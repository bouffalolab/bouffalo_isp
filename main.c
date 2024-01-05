#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <termios.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

/* include bl616 download lib */
#include "isp/mdl_isp.h"
#include "fw_util.h"
#include "user_config.h"

/* redefine printf */
#define LOG_D(...) printf("\033[0;0m " __VA_ARGS__)
#define LOG_I(...) printf("\033[0;32m " __VA_ARGS__)
#define LOG_W(...) printf("\033[0;33m " __VA_ARGS__)
#define LOG_E(...) printf("\033[0;31m " __VA_ARGS__)

#define bflb_mtimer_delay_ms(ms)  usleep(1000 * ms)
#define bflb_mtimer_get_time_ms() clock()

#define CONFIG_WHOLE_BIN 1

static uint8_t  send_buffer[4096];
static uint8_t  recv_buffer[4096];
static uint8_t  file_buffer[4096];
static uint32_t fnum = 0;

/* fd */
int          fd;
char         tty[20];
static FILE *fp;
static FILE *gpio_fp;

/* set uart config */
int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop)
{
    struct termios newtio, oldtio;
    if (tcgetattr(fd, &oldtio) != 0)
    {
        perror("tcgetattr error");
        return -1;
    }
    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag |= CLOCAL | CREAD;
    newtio.c_cflag &= ~CSIZE;

    switch (nBits)
    {
        case 7:
            newtio.c_cflag |= CS7;
            break;
        case 8:
            newtio.c_cflag |= CS8;
            break;
    }

    switch (nEvent)
    {
        case 'O':
            newtio.c_cflag |= PARENB;
            newtio.c_cflag |= PARODD;
            newtio.c_iflag |= (INPCK | ISTRIP);
            break;
        case 'E':
            newtio.c_iflag |= (INPCK | ISTRIP);
            newtio.c_cflag |= PARENB;
            newtio.c_cflag &= ~PARODD;
            break;
        case 'N':
            newtio.c_cflag &= ~PARENB;
            break;
    }

    switch (nSpeed)
    {
        case 2400:
            cfsetispeed(&newtio, B2400);
            cfsetospeed(&newtio, B2400);
            break;
        case 4800:
            cfsetispeed(&newtio, B4800);
            cfsetospeed(&newtio, B4800);
            break;
        case 9600:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
            break;
        case 115200:
            cfsetispeed(&newtio, B115200);
            cfsetospeed(&newtio, B115200);
            break;
        case 460800:
            cfsetispeed(&newtio, B460800);
            cfsetospeed(&newtio, B460800);
            break;
        case 500000:
            cfsetispeed(&newtio, B500000);
            cfsetospeed(&newtio, B500000);
            break;
        case 1000000:
            cfsetispeed(&newtio, B1000000);
            cfsetospeed(&newtio, B1000000);
            break;
        case 2000000:
            cfsetispeed(&newtio, B2000000);
            cfsetospeed(&newtio, B2000000);
            break;
        default:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
            break;
    }

    if (nStop == 1)
    {
        newtio.c_cflag &= ~CSTOPB;
    }
    else if (nStop == 2)
    {
        newtio.c_cflag |= CSTOPB;
    }
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN]  = 0;
    tcflush(fd, TCIFLUSH);
    if ((tcsetattr(fd, TCSANOW, &newtio)) != 0)
    {
        perror("set error");
        return -1;
    }
    return 0;
}

/* config uart baudrate */
static int config_baudrate(uint32_t baudrate)
{
    int ret;

    ret = set_opt(fd, baudrate, 8, 'N', 1);
    if (ret == -1)
    {
        printf("Set %s failed! Exit!\r\n", tty);
        exit(1);
    }

    return 0;
}

static int send_alloc(void **data, uint32_t wait)
{
    *data = send_buffer;
    return 0;
}
/* uart send */
static int send(void *data, uint32_t size, uint32_t wait)
{
    write(fd, data, size);
    return 0;
}
/* uart send done */
static int send_done(uint32_t wait)
{
    return 0;
}
/* uart recv */
static int recv(void **data, uint32_t *size, uint32_t wait)
{
    clock_t  begin;
    clock_t  end;
    uint32_t nread = 0;
    uint32_t total = 0;

    usleep(50000);

    memset(recv_buffer, 0, 1024);

    *data = recv_buffer;

    begin = clock();

    while (1)
    {
        end = clock();
        nread += read(fd, &recv_buffer[nread], 1024);

        if ((((end - begin) * 1000) / CLOCKS_PER_SEC) > wait)
        {
            *size = 0;
            //LOG_E("wait time out!!,wait time:%d\r\n", wait);
            return -1;
        }

        if (nread == 0)
        {
            continue;
        }

        if (recv_buffer[2] > 0)
        {
            if (nread < (recv_buffer[2] + 2))
            {
                //printf("read continue\r\n");
                continue;
            }
        }

        *size = nread;
        break;
    }

    //printf("len:%d\r\n", nread);

    // for (uint32_t i = 0; i < nread; i++)
    //     LOG_D(" 0x%02x", recv_buffer[i]);
    // printf("\r\n");
    // LOG_D("\r\nRead data: %c%c\r\n", recv_buffer[0], recv_buffer[1]);

    return 0;
}

static int recv_done(void *data)
{
    return 0;
}
/* wait some time */
static void wait(uint32_t wait)
{
    usleep((wait * 2000));
}

static void event_callback(uint32_t event){};

/* other api ------------------------------------------------------------------*/
static void print_chip_id(uint8_t *chipid)
{
    char chip_id[32];
    snprintf(chip_id, 32, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", chipid[0], chipid[1], chipid[2], chipid[3], chipid[4], chipid[5], chipid[6], chipid[7]);
    LOG_I("chip id %s \r\n", chip_id);
}

static void print_sha256(uint8_t *sha256)
{
    char sha[65];
    for (uint8_t i = 0; i < 32; i++)
    {
        snprintf(sha + (i * 2), 65, "%02x", sha256[i]);
    }
    LOG_D("%s\r\n", sha);
}

/* mdl isp ------------------------------------------------------------------*/
static mdl_isp_t isp = {
    .baudrate_low    = 500000,
    .baudrate_high   = 500000,
    .event_callback  = event_callback,
    .config_baudrate = config_baudrate,
    .wait            = wait,
    .send_alloc      = send_alloc,
    .send            = send,
    .send_done       = send_done,
    .recv            = recv,
    .recv_done       = recv_done
};

/* mdl isp params ------------------------------------------------------------------*/

mdl_isp_params_bl616_t bl616_params = {
    .version = MDL_ISP_VERSION_BL616,

    .clock_params = {
        .magic_code     = 0x47464350,
        .xtal_type      = 7,
        .mcu_clk        = 5,
        .mcu_clk_div    = 0,
        .mcu_bclk_div   = 0,
        .mcu_pbclk_div  = 3,
        .emi_clk        = 2,
        .emi_clk_div    = 1,
        .flash_clk_type = 1,
        .flash_clk_div  = 0,
        .wifipll_pu     = 1,
        .aupll_pu       = 1,
        .rsvd0          = 0,
        .crc32          = 0x89ef340b },

    .flash_set = {
        .flash_pin       = 0x80,
        .flash_clk_cfg   = 0x10,
        .flash_io_mode   = 0x01,
        .flash_clk_delay = 0x01,
    },
    .flash_params = {
        .io_mode                   = 0x04,
        .c_read_support            = 0x01,
        .clk_delay                 = 0x00,
        .clk_invert                = 0x00,
        .reset_en_cmd              = 0x66,
        .reset_cmd                 = 0x99,
        .reset_c_read_cmd          = 0xff,
        .reset_c_read_cmd_size     = 0x03,
        .jedec_id_cmd              = 0x9f,
        .jedec_id_cmd_dmy_clk      = 0x00,
        .enter_32bits_addr_cmd     = 0xb7,
        .exit_32bits_addr_cmd      = 0xe9,
        .sector_size               = 4,
        .mid                       = 0xef,
        .page_size                 = 256,
        .chip_erase_cmd            = 0xc7,
        .sector_erase_cmd          = 0x20,
        .blk32_erase_cmd           = 0x52,
        .blk64_erase_cmd           = 0xd8,
        .write_enable_cmd          = 0x06,
        .page_program_cmd          = 0x02,
        .qpage_program_cmd         = 0x32,
        .qpp_addr_mode             = 0x00,
        .fast_read_cmd             = 0x0b,
        .fr_dmy_clk                = 1,
        .qpi_fast_read_cmd         = 0x0b,
        .qpi_fr_dmy_clk            = 1,
        .fast_read_do_cmd          = 0x3b,
        .fr_do_dmy_clk             = 1,
        .fast_read_dio_cmd         = 0xbb,
        .fr_dio_dmy_clk            = 0,
        .fast_read_qo_cmd          = 0x6b,
        .fr_qo_dmy_clk             = 1,
        .fast_read_qio_cmd         = 0xeb,
        .fr_qio_dmy_clk            = 2,
        .qpi_fast_read_qio_cmd     = 0xeb,
        .qpi_fr_qio_dmy_clk        = 2,
        .qpi_page_program_cmd      = 0x02,
        .write_vreg_enable_cmd     = 0x50,
        .wr_enable_index           = 0,
        .qe_index                  = 1,
        .busy_index                = 0,
        .wr_enable_bit             = 1,
        .qe_bit                    = 1,
        .busy_bit                  = 0,
        .wr_enable_write_reg_len   = 2,
        .wr_enable_read_reg_len    = 1,
        .qe_write_reg_len          = 1,
        .qe_read_reg_len           = 1,
        .release_powerdown         = 0xab,
        .busy_read_reg_len         = 1,
        .read_reg_cmd              = { 0x05, 0x35, 0x00, 0x00 },
        .write_reg_cmd             = { 0x01, 0x31, 0x00, 0x00 },
        .enter_qpi                 = 0x38,
        .exit_qpi                  = 0xff,
        .c_read_mode               = 0xf0,
        .c_rexit                   = 0xff,
        .burst_wrap_cmd            = 0x77,
        .burst_wrap_cmd_dmy_clk    = 0x03,
        .burst_wrap_data_mode      = 2,
        .burst_wrap_data           = 0x40,
        .de_burst_wrap_cmd         = 0x77,
        .de_burst_wrap_cmd_dmy_clk = 0x03,
        .de_burst_wrap_data_mode   = 2,
        .de_burst_wrap_data        = 0xf0,
        .time_e_sector             = 300,
        .time_e_32k                = 1200,
        .time_e_64k                = 1200,
        .time_page_pgm             = 5,
        .time_ce                   = 33000,
        .pd_delay                  = 8,
        .qe_data                   = 0,
    }
};

extern const uint8_t   mdl_isp_bl702_eflash_loader_32m[59328];
mdl_isp_params_bl702_t bl702_params = {
    .version            = MDL_ISP_VERSION_BL702,
    .eflash_loader_addr = (void *)mdl_isp_bl702_eflash_loader_32m
};

extern const uint8_t   mdl_isp_bl602_eflash_loader_40m_nolog[38592];
mdl_isp_params_bl602_t bl602_params = {
    .version            = MDL_ISP_VERSION_BL602,
    .eflash_loader_addr = (void *)mdl_isp_bl602_eflash_loader_40m_nolog
};

int main(int argc, char *argv[])
{
    int     ret;
    uint8_t cnt = 4;

    mdl_isp_boot_info_generic_t boot_info;

    printf("bouffalo isp for linux\r\n");

    if (argc < 4)
    {
        printf("Usage: bouffalo_isp [tty node] [baud rate] [download file path]\r\n");
        printf("       Sample: bouffalo_isp /dev/ttyACM0 2000000 ./helloworld.bin\r\n");
        return 0;
    }

    printf("check Firmware integrity\r\n");
    if (fw_check_valid(argv[3], NULL) != 0)
    {
        printf("invalid firmware %s\r\n", argv[3]);
        return -1;
    }

    boot_pin_init();
    reset_pin_init();

    memcpy(tty, argv[1], strlen(argv[1]));
    isp.baudrate_low = atoi(argv[2]);
    LOG_I("tty:%s, Baudrate: %s\r\n", argv[1], argv[2]);

    while (1)
    {
        fd = open(tty, O_RDWR);
        if (fd == -1)
        {
            LOG_E("Open %s failed! Exit!\r\n", tty);
            printf("\r\n");
            exit(1);
        }
        LOG_I("open %s successfully!\r\n", tty);

#if 1
    burn_check:

        /*!< enter boot mode */
        reset_pin_low();
        bflb_mtimer_delay_ms(10);
        boot_pin_high();
        bflb_mtimer_delay_ms(10);
        reset_pin_high();
        bflb_mtimer_delay_ms(100);
        boot_pin_low();
        bflb_mtimer_delay_ms(10);
        cnt = 4;
        while (--cnt)
        {
            /*!< discard chip start data */
            // recv_size      = 0;
            // recv_done_flag = 0;
            /*!< shake hand */
            if (0 == mdl_isp_shakehand(&isp))
            {
                LOG_I("========  [step.1] shake hand ok\r\n");
                break;
            }
            else
            {
                LOG_E("========  [step.1] shake hand fail\r\n");
                bflb_mtimer_delay_ms(2000);
                continue;
            }
        }

        if (cnt == 0)
        {
            goto burn_error;
        }
#else

        new_chip_attach_info = true;
        old_chip_remove_info = true;
        is_burn_ok           = false;
        is_new_chip_attach   = false;
        is_old_chip_remove   = true;

    burn_check:

        /*!< enter boot mode */
        mdl_pin_reset(DFTIO_RESET);
        bflb_mtimer_delay_ms(10);
        mdl_pin_set(DFTIO_BOOT);
        bflb_mtimer_delay_ms(10);
        mdl_pin_set(DFTIO_RESET);
        bflb_mtimer_delay_ms(100);
        mdl_pin_reset(DFTIO_BOOT);
        bflb_mtimer_delay_ms(10);

        /*!< discard chip start data */
        recv_size = 0;
        recv_done_flag = 0;

        /*!< shake hand */
        if (0 == mdl_isp_shakehand(&isp))
        {
            if (!is_burn_ok)
            {
                is_new_chip_attach = true;
            }

            if (is_new_chip_attach && is_old_chip_remove)
            {
                LOG_I("========  [step.0] new chip attach\r\n");
                LOG_I("========  [step.1] shake hand ok\r\n");
                goto burn_start;
            }
            else if (!is_old_chip_remove)
            {
                if (new_chip_attach_info)
                {
                    LOG_I("You can now safety remove old chip\r\n");
                    new_chip_attach_info = false;
                }
            }

            goto burn_check;
        }
        else
        {
            if (is_new_chip_attach)
            {
                LOG_E("========  [step.1] shake hand fail\r\n");
                is_new_chip_attach = false;
                goto burn_check;
            }
            else
            {
                if (old_chip_remove_info)
                {
                    new_chip_attach_info = true;
                    LOG_I("You can now safety attach new chip\r\n");
                    old_chip_remove_info = false;
                }
            }

            if (is_burn_ok)
            {
                is_old_chip_remove = true;
                is_burn_ok = false;
                goto burn_check;
            }
            else
            {
                is_old_chip_remove = true;
                goto burn_check;
            }
        }

#endif
    burn_start:

        /*!< get boot info */
        if (0 == mdl_isp_get_bootinfo(&isp, &boot_info))
        {
            LOG_I("========  [step.2] get boot info ok\r\n");
            LOG_I("version %08x\r\n", boot_info.version);
        }
        else
        {
            LOG_E("========  [step.2] get boot info fail\r\n");
            goto burn_error;
        }

        char *path = NULL;

        switch (boot_info.version)
        {
            case 0x00000001:
            case MDL_ISP_VERSION_BL602:
                path = argv[3];
                print_chip_id(isp.boot_info_bl602.chip_id);

                /*!< set chip ready */
                if (0 == mdl_isp_set_chip_ready(&isp, (void *)&bl602_params))
                {
                    LOG_I("========  [step.3] set chip ready ok\r\n");
                    LOG_I("flash id %08x\r\n", isp.jedecid);
                }
                else
                {
                    LOG_E("========  [step.3] set chip ready fail\r\n");
                    goto burn_error;
                }

                break;

            case MDL_ISP_VERSION_BL702:
                path = argv[3];
                print_chip_id(isp.boot_info_bl702.chip_id);

                /*!< set chip ready */
                if (0 == mdl_isp_set_chip_ready(&isp, (void *)&bl702_params))
                {
                    LOG_I("========  [step.3] set chip ready ok\r\n");
                    LOG_I("flash id %08x\r\n", isp.jedecid);
                }
                else
                {
                    LOG_E("========  [step.3] set chip ready fail\r\n");
                    goto burn_error;
                }

                break;

            case MDL_ISP_VERSION_BL616:
                path = argv[3];
                print_chip_id(isp.boot_info_bl616.chip_id);

                /*!< set chip ready */
                if (0 == mdl_isp_set_chip_ready(&isp, (void *)&bl616_params))
                {
                    LOG_I("========  [step.3] set chip ready ok\r\n");
                    LOG_I("flash id %08x\r\n", isp.jedecid);
                }
                else
                {
                    LOG_E("========  [step.3] set chip ready fail\r\n");
                    goto burn_error;
                }
                break;

            case MDL_ISP_VERSION_BL808:
                path = argv[3];
                print_chip_id(isp.boot_info_bl808.chip_id);
                LOG_W("bl808 not support yet\r\n");

                goto burn_error;
                break;

            default:
                LOG_E("unknown soc\r\n");
                goto burn_error;
        }

        if (isp.jedecid == 0xbaadc0de)
        {
            LOG_E("error flash id\r\n");
            goto burn_error;
        }

        uint64_t begin;
        uint64_t end;

        uint32_t addr_offset;

#if CONFIG_WHOLE_BIN
        LOG_D("whole bin enable\r\n");
        addr_offset = 0;
#else
        LOG_D("whole bin disable\r\n");
        addr_offset = IMG_ADDRESS_OFFSET;

        LOG_I("========  [step.4.0] program bootinfo\r\n");

        /*!< erase for bootinfo */
        begin = bflb_mtimer_get_time_ms();
        LOG_I("erase 0x%06x ~ 0x%06x\r\n", 0, IMG_ADDRESS_OFFSET);
        if (0 == mdl_isp_flash_erase(&isp, 0, IMG_ADDRESS_OFFSET, 2000))
        {
            end = bflb_mtimer_get_time_ms();
            LOG_I("========  [step.4.0.1] erase done, use %d ms\r\n", (uint32_t)(end - begin));
        }
        else
        {
            LOG_E("========  [step.4.0.1] erase fail\r\n");
            goto burn_error;
        }

        begin = bflb_mtimer_get_time_ms();
        uint8_t retry = 0;
        while (mdl_isp_flash_program(&isp, 0, (void *)bootinfo, 0x1000))
        {
            retry++;

            if (retry > 3)
            {
                LOG_E("========  [step.4.0.2] program retry timeout\r\n");
                goto burn_error;
            }
            else
            {
                LOG_W("retry %d\r\n", retry);
            }
        }
        retry = 0;
        while (mdl_isp_flash_program(&isp, 0x1000, (void *)(bootinfo + 0x1000), 0x1000))
        {
            retry++;

            if (retry > 3)
            {
                LOG_E("========  [step.4.0.2] program retry timeout\r\n");
                goto burn_error;
            }
            else
            {
                end
                    LOG_W("retry %d\r\n", retry);
            }
        }

        LOG_I("========  [step.4.0.2] program bootinfo done\r\n");
        end = bflb_mtimer_get_time_ms();
        if (mdl_isp_flash_program_check(&isp))
        {
            LOG_E("========  [step.4.0.3] program check fail\r\n");
            goto burn_error;
        }

        LOG_I("========  [step.4.0.3] program check ok, use %d ms\r\n", (uint32_t)(end - begin));

#endif

        /*!< open bin file */
        fp = fopen(path, "r");
        if (fp == NULL)
        {
            LOG_E("not find file %s\r\n", path);
            bflb_mtimer_delay_ms(1000);
            goto burn_error;
        }

        /* get file size */
        fseek(fp, 0, SEEK_END);
        uint32_t size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        LOG_D("find file ok\r\n");
        LOG_D("img size %d KBytes\r\n", size / 1024);

        /*!< erase for img */
        begin = bflb_mtimer_get_time_ms();
        LOG_D("erase 0x%06x ~ 0x%06x\r\n", addr_offset, addr_offset + size - 1);
        if (0 == mdl_isp_flash_erase(&isp, addr_offset, addr_offset + size - 1, 15000))
        {
            end = bflb_mtimer_get_time_ms();
            LOG_I("========  [step.4.1] erase done, use %d ms\r\n", (uint32_t)(end - begin));
        }
        else
        {
            LOG_E("========  [step.4.1] erase fail\r\n");
            goto burn_error;
        }

        LOG_D("program 0x%06x ~ 0x%06x\r\n", addr_offset, addr_offset + size - 1);
        LOG_D("program [  0%%][]\r\n");

        /*!< program for img */
        char     progressbar[41] = { 0 };
        uint32_t addr            = addr_offset;
        begin                    = bflb_mtimer_get_time_ms();
        while (1)
        {
            fnum = fread(file_buffer, 1, 4096, fp);
            if (fnum == 0)
            {
                fclose(fp);
                break;
            }

            uint8_t retry = 0;
            while (mdl_isp_flash_program(&isp, addr, file_buffer, fnum))
            {
                retry++;

                if (retry > 3)
                {
                    LOG_E("========  [step.4.2] program retry timeout\r\n");
                    goto burn_error;
                }
                else
                {
                    LOG_W("retry %d\r\n", retry);
                }
            }

            addr += fnum;
            for (uint8_t i = 0; i < ((addr - addr_offset) * 40) / size; i++)
            {
                progressbar[i] = '=';
            }
            LOG_D("program [%3d%%][%s]\r\n", ((addr - addr_offset) * 100) / size, progressbar);
        }
        LOG_I("========  [step.4.2] program done\r\n");
        end = bflb_mtimer_get_time_ms();
        if (mdl_isp_flash_program_check(&isp))
        {
            LOG_E("========  [step.4.3] program check fail\r\n");
            goto burn_error;
        }

        LOG_I("========  [step.4.3] program check ok, use %d ms\r\n", (uint32_t)(end - begin));

#if 0
        /*!< use spi mode read sha */
        uint8_t sha256[32];
        if (0 == mdl_isp_flash_get_sha256(&isp, addr_offset, size, sha256))
        {
            LOG_I("========  [step.5.1] read sha256\r\n");
            print_sha256(sha256);
        }
        else
        {
            LOG_E("========  [step.5.1] read sha256 fail\r\n");
            break;
        }
#else
        /*!< use xip mode read sha */
        uint8_t sha256[32];
        if (0 == mdl_isp_flash_xip_enter(&isp))
        {
            LOG_I("========  [step.5.0] enter xip mode\r\n");
        }
        else
        {
            LOG_E("========  [step.5.0] enter xip mode fail\r\n");
            goto burn_error;
        }
        if (0 == mdl_isp_flash_get_sha256(&isp, addr_offset, size, sha256))
        {
            LOG_I("========  [step.5.1] read sha256\r\n");
            print_sha256(sha256);
        }
        else
        {
            LOG_E("========  [step.5.1] read sha256 fail\r\n");
            goto burn_error;
        }

        if (0 == mdl_isp_flash_xip_exit(&isp))
        {
            LOG_I("========  [step.5.2] exit xip mode\r\n");
        }
        else
        {
            LOG_E("========  [step.5.2] exit xip mode fail\r\n");
            goto burn_error;
        }
#endif
        goto burn_success;
    }

burn_error:
    LOG_E("========  [step.6] burn fail\r\n");
    LOG_D("\r\n");
    close(fd);
    return -1;

burn_success:
    LOG_I("========  [step.6] burn success\r\n");
    LOG_D("\r\n");
    close(fd);

    boot_pin_low();
    bflb_mtimer_delay_ms(10);
    reset_pin_low();
    bflb_mtimer_delay_ms(10);
    reset_pin_high();
    return 0;
}