 
/**
 * @file mdl_isp.h
 * @brief
 *
 * Copyright (c) 2021 Bouffalolab team
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 */

#ifndef _SHA256_H
#define _SHA256_H

typedef unsigned char BYTE;
typedef unsigned int  WORD;
 
typedef struct {
	BYTE ctxdata[64];
	WORD datalen;
	unsigned long long bitlen;
	WORD state[8];
} SHA256_CTX;
 
void SHA256_Init(SHA256_CTX *ctx);
void SHA256_Update(SHA256_CTX *ctx, const BYTE data[], WORD len);
void SHA256_Final(SHA256_CTX *ctx, BYTE hash[]);
uint32_t BFLB_Soft_CRC32_Ex(uint32_t initial, void *dataIn, uint32_t len);
int fw_check_valid(char *filepath, char version[32]);

#endif