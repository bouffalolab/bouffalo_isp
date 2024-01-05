#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "fw_util.h"
#include <sys/stat.h> // Include the header for the mkdir function
#include "isp/mdl_isp.h" //just for LOG macro

#define SHA256_DIGEST_LENGTH 32
#define BFLB_BTHEADER_LEN      176
#define BFLB_FW_LENGTH_OFFSET 120
#define BFLB_FW_HASH_OFFSET   132
#define BFLB_FW_VER_MAGIC	 "BLFBVERF"

#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))
 
#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))
 
static const unsigned int k[64] = {
	0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
	0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
	0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
	0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
	0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
	0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
	0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
	0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};
 
void sha256_transform(SHA256_CTX *ctx, const BYTE databuf[])
{
	unsigned int a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];
 
	for (i = 0, j = 0; i < 16; ++i, j += 4)
		m[i] = (databuf[j] << 24) | (databuf[j + 1] << 16) | (databuf[j + 2] << 8) | (databuf[j + 3]);
	for ( ; i < 64; ++i)
		m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
 
	a = ctx->state[0];
	b = ctx->state[1];
	c = ctx->state[2];
	d = ctx->state[3];
	e = ctx->state[4];
	f = ctx->state[5];
	g = ctx->state[6];
	h = ctx->state[7];
 
	for (i = 0; i < 64; ++i) {
		t1 = h + EP1(e) + CH(e,f,g) + k[i] + m[i];
		t2 = EP0(a) + MAJ(a,b,c);
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}
 
	ctx->state[0] += a;
	ctx->state[1] += b;
	ctx->state[2] += c;
	ctx->state[3] += d;
	ctx->state[4] += e;
	ctx->state[5] += f;
	ctx->state[6] += g;
	ctx->state[7] += h;
}
 
void SHA256_Init(SHA256_CTX *ctx)
{
	ctx->datalen = 0;
	ctx->bitlen = 0;
	ctx->state[0] = 0x6a09e667;
	ctx->state[1] = 0xbb67ae85;
	ctx->state[2] = 0x3c6ef372;
	ctx->state[3] = 0xa54ff53a;
	ctx->state[4] = 0x510e527f;
	ctx->state[5] = 0x9b05688c;
	ctx->state[6] = 0x1f83d9ab;
	ctx->state[7] = 0x5be0cd19;
}
 
void SHA256_Update(SHA256_CTX *ctx, const BYTE databuf[], WORD len)
{
	unsigned int i;
 
	for (i = 0; i < len; ++i) {
		ctx->ctxdata[ctx->datalen] = databuf[i];
		ctx->datalen++;
		if (ctx->datalen == 64) {
			sha256_transform(ctx, ctx->ctxdata);
			ctx->bitlen += 512;
			ctx->datalen = 0;
		}
	}
}
 
void SHA256_Final(SHA256_CTX *ctx, BYTE hash[])
{
	unsigned int i;
    int j;
	i = ctx->datalen;
 
	if (ctx->datalen < 56) {
		ctx->ctxdata[i++] = 0x80;  // pad 10000000 = 0x80
		while (i < 56)
			ctx->ctxdata[i++] = 0x00;
	}
	else {
		ctx->ctxdata[i++] = 0x80;
		while (i < 64)
			ctx->ctxdata[i++] = 0x00;
		sha256_transform(ctx, ctx->ctxdata);
		memset(ctx->ctxdata, 0, 56);
	}
 
	ctx->bitlen += ctx->datalen * 8;
	ctx->ctxdata[63] = ctx->bitlen;
	ctx->ctxdata[62] = ctx->bitlen >> 8;
	ctx->ctxdata[61] = ctx->bitlen >> 16;
	ctx->ctxdata[60] = ctx->bitlen >> 24;
	ctx->ctxdata[59] = ctx->bitlen >> 32;
	ctx->ctxdata[58] = ctx->bitlen >> 40;
	ctx->ctxdata[57] = ctx->bitlen >> 48;
	ctx->ctxdata[56] = ctx->bitlen >> 56;
	sha256_transform(ctx, ctx->ctxdata);
 
	for (i = 0; i < 4; ++i) {
		hash[i]      = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 4]  = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 8]  = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff;
	}
 
}
/******************************************************************************
* Name:    CRC-32  x32+x26+x23+x22+x16+x12+x11+x10+x8+x7+x5+x4+x2+x+1
* Poly:    0x4C11DB7
* Init:    0xFFFFFFF
* Refin:   True
* Refout:  True
* Xorout:  0xFFFFFFF
* Alias:   CRC_32/ADCCP
* Use:     WinRAR,ect.
*****************************************************************************/
uint32_t BFLB_Soft_CRC32_Ex(uint32_t initial, void *dataIn, uint32_t len)
{
    uint8_t i;
    uint32_t crc = ~initial;        // Initial value
    uint8_t *data=(uint8_t *)dataIn;
    
    while(len--){
        crc ^= *data++;                // crc ^= *data; data++;
        for (i = 0; i < 8; ++i){
            if (crc & 1){
                crc = (crc >> 1) ^ 0xEDB88320;// 0xEDB88320= reverse 0x04C11DB7
            }else{
                crc = (crc >> 1);
            }
        }
    }
    return ~crc;
}

int fw_check_valid(char *filepath, char version[32])
{    
	unsigned char sha_cal[SHA256_DIGEST_LENGTH];
    unsigned char sha_fw[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256_ctx;
    unsigned char file_rd_buffer[1024]; 
    uint32_t *fw_magic=NULL;
    uint32_t *fw_crc32=NULL;
    uint32_t *fw_len=0;
    uint32_t crc_value = 0;
    struct stat file_stat;

	//LOG_D("Check firmware:%s\r\n",filepath);

	/* do file sha check */
    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        printf("Error opening file:%s\r\n",filepath);
        return 1;
    }
    stat(filepath, &file_stat);

    fread(file_rd_buffer, 1, BFLB_BTHEADER_LEN, file);
    fw_magic=(uint32_t *)file_rd_buffer;
    if(*fw_magic!=0x504e4642){
        remove(filepath);
        printf("Bootheader magic error:%x\r\n",*fw_magic);
        return 1;
    }
    //LOG_D("Bootheader magic :%x\r\n",*fw_magic);
    
    crc_value = BFLB_Soft_CRC32_Ex(crc_value, file_rd_buffer, BFLB_BTHEADER_LEN-4);
    fw_crc32=(uint32_t *)&file_rd_buffer[BFLB_BTHEADER_LEN-4];
    if(crc_value!=*fw_crc32){
        remove(filepath);
        printf("Bootheader crc32 error,cal=%x,fw=%x\r\n",crc_value,*fw_crc32);
        return 1;
    }
    //LOG_D("Bootheader crc32 %x\r\n",*fw_crc32);

    fw_len=(uint32_t *)&file_rd_buffer[BFLB_FW_LENGTH_OFFSET];
    if(file_stat.st_size-8*1024!=*fw_len){
        remove(filepath);
        printf("Bootheader fw len error,header=%d,file=%ld\r\n",*fw_len,file_stat.st_size);
        return 1;
    }
    //printf("Bootheader fw len =%d\r\n",*fw_len);
    memcpy(sha_fw,file_rd_buffer+BFLB_FW_HASH_OFFSET,SHA256_DIGEST_LENGTH);

    // Move the file pointer to 8K position
    fseek(file, 8 * 1024, SEEK_SET);

    SHA256_Init(&sha256_ctx);

    size_t bytes_read;
    while ((bytes_read = fread(file_rd_buffer, 1, sizeof(file_rd_buffer), file)) != 0) {
        SHA256_Update(&sha256_ctx, file_rd_buffer, bytes_read);
    }

    SHA256_Final(&sha256_ctx,sha_cal);
	if(memcmp(sha_cal,sha_fw,SHA256_DIGEST_LENGTH)!=0){
        remove(filepath);
        printf("Bootheader Hash error\r\n");
        return 1;
    }
    //LOG_D("Hash start=%02x,end=%02x\r\n",sha_cal[0],sha_cal[31]);

    // // Move the file pointer to 8+3K position
    // fseek(file, 11 * 1024, SEEK_SET);
    // fread(file_rd_buffer, 1, 32, file);
	// if(memcmp(file_rd_buffer,BFLB_FW_VER_MAGIC,sizeof(BFLB_FW_VER_MAGIC)-1)==0){
    //     memset(version,0,32);
	// 	int max_len=32;
	// 	if(strlen(file_rd_buffer+sizeof(BFLB_FW_VER_MAGIC)-1)<32){
	// 		max_len=strlen(file_rd_buffer+sizeof(BFLB_FW_VER_MAGIC)-1);
	// 	}
    //     memcpy(version,file_rd_buffer+sizeof(BFLB_FW_VER_MAGIC)-1,max_len);
	// }

    fclose(file);
   
	return 0;
}