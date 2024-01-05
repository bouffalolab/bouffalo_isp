/**
 * @file mdl_isp.c
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

#include "mdl_isp.h"
#include <string.h>

#ifdef CONFIG_MDL_ISP_DEBUG
#define _MDL_ISP_CHECK(_expr, _ret) \
    if (!(_expr))                   \
    return _ret
#else
#define _MDL_ISP_CHECK(_expr, _ret) ((void)0)
#endif

#define _MDL_ISP_ALL_CHECK(_isp)                         \
    _MDL_ISP_CHECK(NULL != (_isp), -1);                  \
    _MDL_ISP_CHECK(NULL != (_isp)->config_baudrate, -1); \
    _MDL_ISP_CHECK(NULL != (_isp)->event_callback, -1);  \
    _MDL_ISP_CHECK(NULL != (_isp)->wait, -1);            \
    _MDL_ISP_CHECK(NULL != (_isp)->send_alloc, -1);      \
    _MDL_ISP_CHECK(NULL != (_isp)->send, -1);            \
    _MDL_ISP_CHECK(NULL != (_isp)->send_done, -1);       \
    _MDL_ISP_CHECK(NULL != (_isp)->recv, -1);            \
    _MDL_ISP_CHECK(NULL != (_isp)->recv_done, -1);

#define ISP_SEND_ALLOC_WAIT 100
#define ISP_WAIT            30

#define MDL_ISP_VARIABLE() \
    int _ret;              \
    uint32_t _size;        \
    uint8_t *_data

#define MDL_ISP_ALLOC()                                           \
    if (isp->send_alloc((void *)&(_data), ISP_SEND_ALLOC_WAIT)) { \
        return -1;                                                \
    }

#define MDL_ISP_HEADER_LEN 4
#define MDL_ISP_HEADER(cmd_, size_)     \
    _data[0] = (cmd_);                  \
    _data[1] = (0);                     \
    _data[2] = (uint8_t)((size_) >> 0); \
    _data[3] = (uint8_t)((size_) >> 8)

#define MDL_ISP_CHECKSUM(size_) \
    _data[1] = (uint8_t)(mdl_isp_calc_checksum(&_data[2], 2 + (size_)))

#define MDL_ISP_SENDRECV(send_size_, wait_)               \
    isp->wait(ISP_WAIT);                                  \
    if (isp->send((_data), (send_size_), (wait_))) {      \
        return -1;                                        \
    }                                                     \
    if (isp->recv((void *)&(_data), &(_size), (wait_))) { \
        return -1;                                        \
    }                                                     \
    if (isp->send_done((wait_))) {                        \
        return -1;                                        \
    }

#define MDL_ISP_IF_ACK()                    \
    _ret = mdl_isp_check_ack(_data, _size); \
    if (_ret == 0)

#define MDL_ISP_DONE()             \
    if (isp->recv_done((_data))) { \
        return -1;                 \
    }                              \
    if (_ret) {                    \
        return _ret;               \
    } else {                       \
        return 0;                  \
    }

#define BFLB_EFLASH_LOADER_CMD_GET_BOOTINFO           0x10
#define BFLB_EFLASH_LOADER_CMD_LOAD_BOOTHEADER        0x11
#define BFLB_EFLASH_LOADER_CMD_LOAD_PUBLICKEY         0x12
#define BFLB_EFLASH_LOADER_CMD_LOAD_PUBLICKEY2        0x13
#define BFLB_EFLASH_LOADER_CMD_LOAD_SIGNATURE         0x14
#define BFLB_EFLASH_LOADER_CMD_LOAD_SIGNATURE2        0x15
#define BFLB_EFLASH_LOADER_CMD_LOAD_AESIV             0x16
#define BFLB_EFLASH_LOADER_CMD_LOAD_SEGHEADER         0x17
#define BFLB_EFLASH_LOADER_CMD_LOAD_SEGDATA           0x18
#define BFLB_EFLASH_LOADER_CMD_CHECK_IMG              0x19
#define BFLB_EFLASH_LOADER_CMD_RUN                    0x1A
#define BFLB_EFLASH_LOADER_CMD_CHANGE_RATE            0x20
#define BFLB_EFLASH_LOADER_CMD_RESET                  0x21
#define BFLB_EFLASH_LOADER_CMD_CLK_SET                0x22
#define BFLB_EFLASH_LOADER_CMD_FLASH_ERASE            0x30
#define BFLB_EFLASH_LOADER_CMD_FLASH_WRITE            0x31
#define BFLB_EFLASH_LOADER_CMD_FLASH_READ             0x32
#define BFLB_EFLASH_LOADER_CMD_FLASH_BOOT             0x33
#define BFLB_EFLASH_LOADER_CMD_FLASH_XIP_READ         0x34
#define BFLB_EFLASH_LOADER_CMD_FLASH_SBUS_XIP_READ    0x35
#define BFLB_EFLASH_LOADER_CMD_FLASH_READ_JEDECID     0x36
#define BFLB_EFLASH_LOADER_CMD_FLASH_READ_STATUS_REG  0x37
#define BFLB_EFLASH_LOADER_CMD_FLASH_WRITE_STATUS_REG 0x38

#define BFLB_EFLASH_LOADER_CMD_FLASH_WRITE_CHECK      0x3A
#define BFLB_EFLASH_LOADER_CMD_FLASH_SET_PARA         0x3B
#define BFLB_EFLASH_LOADER_CMD_FLASH_CHIPERASE        0x3C
#define BFLB_EFLASH_LOADER_CMD_FLASH_READSHA          0x3D
#define BFLB_EFLASH_LOADER_CMD_FLASH_XIP_READSHA      0x3E
#define BFLB_EFLASH_LOADER_CMD_EFUSE_WRITE            0x40
#define BFLB_EFLASH_LOADER_CMD_EFUSE_READ             0x41
#define BFLB_EFLASH_LOADER_CMD_EFUSE_READ_MAC_ADDR    0x42

#define BFLB_EFLASH_LOADER_CMD_MEM_WRITE              0x50
#define BFLB_EFLASH_LOADER_CMD_MEM_READ               0x51

#define BFLB_EFLASH_LOADER_CMD_XIP_READ_START         0x60
#define BFLB_EFLASH_LOADER_CMD_XIP_READ_FINISH        0x61

#define BFLB_EFLASH_LOADER_CMD_LOG_READ               0x71

#define BFLB_EFLASH_LOADER_CMD_EFUSE_SECURITY_WRITE   0x80
#define BFLB_EFLASH_LOADER_CMD_EFUSE_SECURITY_READ    0x81

#define BFLB_EFLASH_LOADER_CMD_GET_ECDH_PK            0x90
#define BFLB_EFLASH_LOADER_CMD_ECDH_CHANLLENGE        0x91

#define BFLB_EFLASH_LOADER_CMD_ISP_MODE               0xa0
#define BFLB_EFLASH_LOADER_CMD_IAP_MODE               0xa1

static uint8_t mdl_isp_calc_checksum(uint8_t *data, uint32_t size)
{
    uint32_t sum = 0;

    for (uint32_t i = 0; i < size; ++i) {
        sum += data[i];
    }
    return sum & 0xff;
}

static int mdl_isp_check_ack(uint8_t *data, uint32_t size)
{
    if (data[0] == 'O' && data[1] == 'K') {
        return 0;
    } else if (data[0] == 'F' && data[1] == 'L') {
        return (data[2] << 0) | (data[3] << 8);
    } else if (data[0] == 'P' && data[1] == 'D') {
        return -2;
    } else {
        if (strstr(data, "OK")) {
            return 0;
        } else {
            return -1;
        }
    }
}

static int mdl_isp_reshakehand(mdl_isp_t *isp)
{
    uint8_t *__data;
    uint32_t __size;

    if (isp->config_baudrate(isp->baudrate_high)) {
        return -1;
    }

    MDL_ISP_VARIABLE();
    MDL_ISP_ALLOC();

    memset(_data, 0x55, 256);

    isp->recv((void *)&__data, &__size, 200);
    isp->recv_done(__data);

    isp->wait(300);
    MDL_ISP_SENDRECV(256, 1000);

    MDL_ISP_IF_ACK()
    {
    }

    MDL_ISP_DONE();
}

static int mdl_isp_get_flash_id(mdl_isp_t *isp, uint32_t *jedecid)
{
    MDL_ISP_VARIABLE();
    MDL_ISP_ALLOC();
    MDL_ISP_HEADER(BFLB_EFLASH_LOADER_CMD_FLASH_READ_JEDECID, 0);
    MDL_ISP_SENDRECV(MDL_ISP_HEADER_LEN, 1000);

    MDL_ISP_IF_ACK()
    {
        isp->jedecid = (_data[4] << 24) | (_data[5] << 16) | (_data[6] << 8) | (_data[7] << 0);
        *jedecid = isp->jedecid;
    }
    else
    {
        isp->jedecid = 0xbaadc0de;
        *jedecid = 0xbaadc0de;
    }

    MDL_ISP_DONE();
}

static int bl702_bl602_isp_load_bootheader(mdl_isp_t *isp, uint8_t *data)
{
    MDL_ISP_VARIABLE();
    MDL_ISP_ALLOC();
    MDL_ISP_HEADER(BFLB_EFLASH_LOADER_CMD_LOAD_BOOTHEADER, 176);

    memcpy(_data + MDL_ISP_HEADER_LEN, data, 176);

    MDL_ISP_SENDRECV(MDL_ISP_HEADER_LEN + 176, 500);

    MDL_ISP_IF_ACK()
    {
    }

    MDL_ISP_DONE();
}

static int bl702_bl602_isp_load_sectionheader(mdl_isp_t *isp, uint8_t *data)
{
    MDL_ISP_VARIABLE();
    MDL_ISP_ALLOC();
    MDL_ISP_HEADER(BFLB_EFLASH_LOADER_CMD_LOAD_SEGHEADER, 16);

    memcpy(_data + MDL_ISP_HEADER_LEN, data, 16);

    MDL_ISP_SENDRECV(MDL_ISP_HEADER_LEN + 16, 500);

    MDL_ISP_IF_ACK()
    {
    }

    MDL_ISP_DONE();
}

static uint32_t bl702_bl602_isp_get_sectiondata_size(uint8_t *eflash_loader)
{
    uint8_t *data = eflash_loader + 176 + 4;
    return (data[0] << 0) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

static int bl702_bl602_isp_load_sectiondata(mdl_isp_t *isp, uint8_t *data, uint32_t size)
{
    MDL_ISP_VARIABLE();
    MDL_ISP_ALLOC();
    MDL_ISP_HEADER(BFLB_EFLASH_LOADER_CMD_LOAD_SEGDATA, size);

    memcpy(_data + MDL_ISP_HEADER_LEN, data, size);

    MDL_ISP_SENDRECV(MDL_ISP_HEADER_LEN + size, 3000);

    MDL_ISP_IF_ACK()
    {
    }

    MDL_ISP_DONE();
}

static int bl702_bl602_isp_check_eflash_loader(mdl_isp_t *isp)
{
    MDL_ISP_VARIABLE();
    MDL_ISP_ALLOC();
    MDL_ISP_HEADER(BFLB_EFLASH_LOADER_CMD_CHECK_IMG, 0);
    MDL_ISP_SENDRECV(MDL_ISP_HEADER_LEN, 1000);

    MDL_ISP_IF_ACK()
    {
    }

    MDL_ISP_DONE();
}

static int bl602_isp_run_eflash_loader(mdl_isp_t *isp)
{
    MDL_ISP_VARIABLE();
    MDL_ISP_ALLOC();
    MDL_ISP_HEADER(BFLB_EFLASH_LOADER_CMD_RUN, 0);

    MDL_ISP_SENDRECV(MDL_ISP_HEADER_LEN, 1000);

    MDL_ISP_IF_ACK()
    {
    }

    MDL_ISP_DONE();
}

static int bl702_isp_run_eflash_loader(mdl_isp_t *isp)
{
    MDL_ISP_VARIABLE();
    MDL_ISP_ALLOC();

    _data[0] = 0x50;
    _data[1] = 0x00;
    _data[2] = 0x08;
    _data[3] = 0x00;
    _data[4] = 0x00;
    _data[5] = 0xf1;
    _data[6] = 0x00;
    _data[7] = 0x40;
    _data[8] = 0x45;
    _data[9] = 0x48;
    _data[10] = 0x42;
    _data[11] = 0x4e;

    _data[12] = 0x50;
    _data[13] = 0x00;
    _data[14] = 0x08;
    _data[15] = 0x00;
    _data[16] = 0x04;
    _data[17] = 0xf1;
    _data[18] = 0x00;
    _data[19] = 0x40;
    _data[20] = 0x00;
    _data[21] = 0x00;
    _data[22] = 0x01;
    _data[23] = 0x22;

    _data[24] = 0x50;
    _data[25] = 0x00;
    _data[26] = 0x08;
    _data[27] = 0x00;
    _data[28] = 0x18;
    _data[29] = 0x00;
    _data[30] = 0x00;
    _data[31] = 0x40;
    _data[32] = 0x00;
    _data[33] = 0x00;
    _data[34] = 0x00;
    _data[35] = 0x00;

    _data[36] = 0x50;
    _data[37] = 0x00;
    _data[38] = 0x08;
    _data[39] = 0x00;
    _data[40] = 0x18;
    _data[41] = 0x00;
    _data[42] = 0x00;
    _data[43] = 0x40;
    _data[44] = 0x02;
    _data[45] = 0x00;
    _data[46] = 0x00;
    _data[47] = 0x00;

    isp->wait(ISP_WAIT);
    if (isp->send(_data, 48, 1000)) {
        return -1;
    }
    if (isp->send_done(1000)) {
        return -1;
    }
    isp->wait(3);
    if (isp->recv((void *)&_data, &_size, 1000)) {
        return -1;
    }

    MDL_ISP_IF_ACK()
    {
    }

    MDL_ISP_DONE();
}

static int bl702_isp_set_chip_ready(mdl_isp_t *isp, void *params)
{
    int ret;
    uint32_t size;
    uint32_t write_size;

    uint8_t *data = (uint8_t *)(((mdl_isp_params_bl702_t *)params)->eflash_loader_addr);

    size = bl702_bl602_isp_get_sectiondata_size(data);

    ret = bl702_bl602_isp_load_bootheader(isp, data);
    if (ret) {
        return ret;
    }

    data += 176;

    ret = bl702_bl602_isp_load_sectionheader(isp, data);
    if (ret) {
        return ret;
    }

    data += 16;

    while (size > 0) {
        write_size = size > 4080 ? 4080 : size;

        ret = bl702_bl602_isp_load_sectiondata(isp, data, write_size);
        if (ret) {
            return ret;
        }

        size -= write_size;
        data += write_size;
    }

    ret = bl702_bl602_isp_check_eflash_loader(isp);
    if (ret) {
        return ret;
    }

    ret = bl702_isp_run_eflash_loader(isp);
    if (ret) {
        return ret;
    }

    ret = mdl_isp_reshakehand(isp);
    if (ret) {
        return ret;
    }

    ret = mdl_isp_get_flash_id(isp, &(isp->jedecid));
    if (ret) {
        return ret;
    }

    return 0;
}

static int bl602_isp_set_chip_ready(mdl_isp_t *isp, void *params)
{
    int ret;
    uint32_t size;
    uint32_t write_size;

    uint8_t *data = (uint8_t *)(((mdl_isp_params_bl602_t *)params)->eflash_loader_addr);

    size = bl702_bl602_isp_get_sectiondata_size(data);

    ret = bl702_bl602_isp_load_bootheader(isp, data);
    if (ret) {
        return ret;
    }

    data += 176;

    ret = bl702_bl602_isp_load_sectionheader(isp, data);
    if (ret) {
        return ret;
    }

    data += 16;

    while (size > 0) {
        write_size = size > 4080 ? 4080 : size;

        ret = bl702_bl602_isp_load_sectiondata(isp, data, write_size);
        if (ret) {
            return ret;
        }

        size -= write_size;
        data += write_size;
    }

    ret = bl702_bl602_isp_check_eflash_loader(isp);
    if (ret) {
        return ret;
    }

    ret = bl602_isp_run_eflash_loader(isp);
    if (ret) {
        return ret;
    }

    ret = mdl_isp_reshakehand(isp);
    if (ret) {
        return ret;
    }

    ret = mdl_isp_get_flash_id(isp, &(isp->jedecid));
    if (ret) {
        return ret;
    }

    return 0;
}

static int bl616_isp_set_clock_params(mdl_isp_t *isp, uint32_t baudrate, void *clock_params)
{
    MDL_ISP_VARIABLE();
    MDL_ISP_ALLOC();
    MDL_ISP_HEADER(BFLB_EFLASH_LOADER_CMD_CLK_SET, 28);

    /*!< uart irq enable */
    _data[4] = 1;
    _data[5] = 0;
    _data[6] = 0;
    _data[7] = 0;

    memcpy(&_data[8], &baudrate, 4);
    memcpy(&_data[12], clock_params, 20);

    MDL_ISP_SENDRECV(MDL_ISP_HEADER_LEN + 28, 3000);

    MDL_ISP_IF_ACK()
    {
    }

    MDL_ISP_DONE();
}

static int bl616_isp_set_flash_set(mdl_isp_t *isp, void *flash_set)
{
    MDL_ISP_VARIABLE();
    MDL_ISP_ALLOC();
    MDL_ISP_HEADER(BFLB_EFLASH_LOADER_CMD_FLASH_SET_PARA, 4);

    memcpy(&_data[4], flash_set, 4);

    MDL_ISP_CHECKSUM(4);
    MDL_ISP_SENDRECV(MDL_ISP_HEADER_LEN + 4, 3000);

    MDL_ISP_IF_ACK()
    {
    }

    MDL_ISP_DONE();
}

static int bl616_isp_set_flash_params(mdl_isp_t *isp, void *flash_set, void *flash_params)
{
    MDL_ISP_VARIABLE();
    MDL_ISP_ALLOC();
    MDL_ISP_HEADER(BFLB_EFLASH_LOADER_CMD_FLASH_SET_PARA, 4 + 84);

    memcpy(&_data[4], flash_set, 4);
    memcpy(&_data[8], flash_params, 84);

    MDL_ISP_CHECKSUM(4 + 84);
    MDL_ISP_SENDRECV(MDL_ISP_HEADER_LEN + 4 + 84, 3000);

    MDL_ISP_IF_ACK()
    {
    }

    MDL_ISP_DONE();
}

static int bl616_isp_set_chip_ready(mdl_isp_t *isp, void *params)
{
    int ret;

    ret = bl616_isp_set_clock_params(
        isp,
        isp->baudrate_high,
        (void *)&(((mdl_isp_params_bl616_t *)params)->clock_params.magic_code));
    if (ret) {
        return ret;
    }

    if (isp->config_baudrate(isp->baudrate_high)) {
        return -1;
    }

    ret = bl616_isp_set_flash_set(isp,
                                  (void *)&(((mdl_isp_params_bl616_t *)params)->flash_set));
    if (ret) {
        return ret;
    }

    ret = mdl_isp_get_flash_id(isp, &(isp->jedecid));
    if (ret) {
        return ret;
    }

    ret = bl616_isp_set_flash_params(isp,
                                     (void *)&(((mdl_isp_params_bl616_t *)params)->flash_set),
                                     (void *)&(((mdl_isp_params_bl616_t *)params)->flash_params));
    if (ret) {
        return ret;
    }

    return 0;
}

int mdl_isp_shakehand(mdl_isp_t *isp)
{
    _MDL_ISP_ALL_CHECK(isp);

    if (isp->config_baudrate(isp->baudrate_low)) {
        return -1;
    }

    MDL_ISP_VARIABLE();
    MDL_ISP_ALLOC();

    memset(_data, 0x55, 256);

    if (isp->send(_data, 256, 100)) {
        return -1;
    }
    if (isp->send_done(100)) {
        return -1;
    }
    if (isp->recv((void *)&_data, &_size, 100)) {
        return -1;
    }

    MDL_ISP_IF_ACK()
    {
    }

    MDL_ISP_DONE();
}

int mdl_isp_get_bootinfo(mdl_isp_t *isp, void *boot_info)
{
    _MDL_ISP_ALL_CHECK(isp);

    MDL_ISP_VARIABLE();
    MDL_ISP_ALLOC();
    MDL_ISP_HEADER(BFLB_EFLASH_LOADER_CMD_GET_BOOTINFO, 0);
    MDL_ISP_SENDRECV(MDL_ISP_HEADER_LEN, 500);

    MDL_ISP_IF_ACK()
    {
        memcpy(boot_info, _data + MDL_ISP_HEADER_LEN, _size - MDL_ISP_HEADER_LEN);
        memcpy((void *)isp, _data + MDL_ISP_HEADER_LEN, _size - MDL_ISP_HEADER_LEN);
    }

    MDL_ISP_DONE();
}

int mdl_isp_set_chip_ready(mdl_isp_t *isp, void *params)
{
    _MDL_ISP_ALL_CHECK(isp);

    if (isp->boot_info_generic.version != *((uint32_t *)params)) {
        if ((isp->boot_info_generic.version == 0x00000001) &&
            (*((uint32_t *)params) == 0x06020001)) {
        } else {
            return -1;
        }
    }

    switch (isp->boot_info_generic.version) {
        case 0x00000001:
        case MDL_ISP_VERSION_BL602:
            return bl602_isp_set_chip_ready(isp, params);
        case MDL_ISP_VERSION_BL702:
            return bl702_isp_set_chip_ready(isp, params);
        case MDL_ISP_VERSION_BL616:
            return bl616_isp_set_chip_ready(isp, params);
        case MDL_ISP_VERSION_BL808:
            return -1;
        default:
            return -1;
    }
}

int mdl_isp_flash_erase(mdl_isp_t *isp, uint32_t start_addr, uint32_t end_addr, uint32_t wait)
{
    _MDL_ISP_ALL_CHECK(isp);

    MDL_ISP_VARIABLE();
    MDL_ISP_ALLOC();
    MDL_ISP_HEADER(BFLB_EFLASH_LOADER_CMD_FLASH_ERASE, 8);

    _data[4] = (uint8_t)(start_addr >> 0);
    _data[5] = (uint8_t)(start_addr >> 8);
    _data[6] = (uint8_t)(start_addr >> 16);
    _data[7] = (uint8_t)(start_addr >> 24);

    _data[8] = (uint8_t)(end_addr >> 0);
    _data[9] = (uint8_t)(end_addr >> 8);
    _data[10] = (uint8_t)(end_addr >> 16);
    _data[11] = (uint8_t)(end_addr >> 24);

    MDL_ISP_SENDRECV(MDL_ISP_HEADER_LEN + 8, wait);

    while (1) {
        MDL_ISP_IF_ACK()
        {
            break;
        }
        else if (_ret == -2)
        {
            if (isp->recv((void *)&(_data), &(_size), (wait))) {
                return -1;
            }
        }
        else
        {
            break;
        }
    }

    MDL_ISP_DONE();
}

int mdl_isp_flash_erase_chip(mdl_isp_t *isp, uint32_t wait)
{
    _MDL_ISP_ALL_CHECK(isp);

    MDL_ISP_VARIABLE();
    MDL_ISP_ALLOC();
    MDL_ISP_HEADER(BFLB_EFLASH_LOADER_CMD_FLASH_CHIPERASE, 0);

    MDL_ISP_SENDRECV(MDL_ISP_HEADER_LEN, wait);

    while (1) {
        MDL_ISP_IF_ACK()
        {
            break;
        }
        else if (_ret == -2)
        {
            if (isp->recv((void *)&(_data), &(_size), (wait))) {
                return -1;
            }
        }
        else
        {
            break;
        }
    }

    MDL_ISP_DONE();
}

int mdl_isp_flash_program(mdl_isp_t *isp, uint32_t addr, void *data, uint32_t size)
{
    _MDL_ISP_ALL_CHECK(isp);

    MDL_ISP_VARIABLE();
    MDL_ISP_ALLOC();
    MDL_ISP_HEADER(BFLB_EFLASH_LOADER_CMD_FLASH_WRITE, size + 4);

    _data[4] = (uint8_t)(addr >> 0);
    _data[5] = (uint8_t)(addr >> 8);
    _data[6] = (uint8_t)(addr >> 16);
    _data[7] = (uint8_t)(addr >> 24);

    memcpy(&_data[8], data, size);

    MDL_ISP_CHECKSUM(size + 4);
    MDL_ISP_SENDRECV(MDL_ISP_HEADER_LEN + size + 4, 3000);

    MDL_ISP_IF_ACK()
    {
    }

    MDL_ISP_DONE();
}

int mdl_isp_flash_program_check(mdl_isp_t *isp)
{
    _MDL_ISP_ALL_CHECK(isp);

    MDL_ISP_VARIABLE();
    MDL_ISP_ALLOC();
    MDL_ISP_HEADER(BFLB_EFLASH_LOADER_CMD_FLASH_WRITE_CHECK, 0);
    MDL_ISP_SENDRECV(MDL_ISP_HEADER_LEN, 3000);

    MDL_ISP_IF_ACK()
    {
    }

    MDL_ISP_DONE();
}

int mdl_isp_flash_get_sha256(mdl_isp_t *isp, uint32_t addr, uint32_t size, uint8_t *sha256)
{
    _MDL_ISP_ALL_CHECK(isp);

    MDL_ISP_VARIABLE();
    MDL_ISP_ALLOC();
    MDL_ISP_HEADER(BFLB_EFLASH_LOADER_CMD_FLASH_READSHA, 8);

    _data[4] = (uint8_t)(addr >> 0);
    _data[5] = (uint8_t)(addr >> 8);
    _data[6] = (uint8_t)(addr >> 16);
    _data[7] = (uint8_t)(addr >> 24);

    _data[8] = (uint8_t)(size >> 0);
    _data[9] = (uint8_t)(size >> 8);
    _data[10] = (uint8_t)(size >> 16);
    _data[11] = (uint8_t)(size >> 24);

    MDL_ISP_SENDRECV(MDL_ISP_HEADER_LEN + 8, 3000);

    MDL_ISP_IF_ACK()
    {
        memcpy(sha256, _data + MDL_ISP_HEADER_LEN, _size - MDL_ISP_HEADER_LEN);
    }

    MDL_ISP_DONE();
}

int mdl_isp_flash_xip_enter(mdl_isp_t *isp)
{
    _MDL_ISP_ALL_CHECK(isp);

    MDL_ISP_VARIABLE();
    MDL_ISP_ALLOC();
    MDL_ISP_HEADER(BFLB_EFLASH_LOADER_CMD_XIP_READ_START, 0);

    MDL_ISP_SENDRECV(MDL_ISP_HEADER_LEN, 500);

    MDL_ISP_IF_ACK()
    {
    }

    MDL_ISP_DONE();
}

int mdl_isp_flash_xip_get_sha256(mdl_isp_t *isp, uint32_t addr, uint32_t size, uint8_t *sha256)
{
    _MDL_ISP_ALL_CHECK(isp);

    MDL_ISP_VARIABLE();
    MDL_ISP_ALLOC();
    MDL_ISP_HEADER(BFLB_EFLASH_LOADER_CMD_FLASH_XIP_READSHA, 8);

    _data[4] = (uint8_t)(addr >> 0);
    _data[5] = (uint8_t)(addr >> 8);
    _data[6] = (uint8_t)(addr >> 16);
    _data[7] = (uint8_t)(addr >> 24);

    _data[8] = (uint8_t)(size >> 0);
    _data[9] = (uint8_t)(size >> 8);
    _data[10] = (uint8_t)(size >> 16);
    _data[11] = (uint8_t)(size >> 24);

    MDL_ISP_SENDRECV(MDL_ISP_HEADER_LEN + 8, 2000);

    MDL_ISP_IF_ACK()
    {
        memcpy(sha256, _data + MDL_ISP_HEADER_LEN, _size - MDL_ISP_HEADER_LEN);
    }

    MDL_ISP_DONE();
}

int mdl_isp_flash_xip_exit(mdl_isp_t *isp)
{
    _MDL_ISP_ALL_CHECK(isp);

    MDL_ISP_VARIABLE();
    MDL_ISP_ALLOC();
    MDL_ISP_HEADER(BFLB_EFLASH_LOADER_CMD_XIP_READ_FINISH, 0);

    MDL_ISP_SENDRECV(MDL_ISP_HEADER_LEN, 500);

    MDL_ISP_IF_ACK()
    {
    }

    MDL_ISP_DONE();
}
