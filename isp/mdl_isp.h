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

#ifndef _MDL_ISP_H
#define _MDL_ISP_H

#include <stdint.h>

#define MDL_ISP_VERSION_BL602 0x06020001
#define MDL_ISP_VERSION_BL702 0x07020001
#define MDL_ISP_VERSION_BL616 0x06160001
#define MDL_ISP_VERSION_BL808 0x08080001

typedef struct
{
    uint32_t version; /*!< bootrom version */

    /*!< bl602 otp info 16bytes */
    /*!< bl702 otp info 20bytes */
    /*!< bl616 otp info 20bytes */
    /*!< bl808 otp info 20bytes */
    uint8_t otp_info[20];
} __attribute__((packed)) mdl_isp_boot_info_generic_t;

typedef struct
{
    uint32_t version; /*!< bootrom version */

    struct
    {
        uint8_t sign_type[1]; /*!< sign type */
        uint8_t encrypted[1]; /*!< aes type  */
        uint8_t : 8;          /*!< reserved  */
        uint8_t : 8;          /*!< reserved  */
    } boot_efuse_hw_cfg;

    uint32_t : 32;
    uint8_t chip_id[8];
} __attribute__((packed)) mdl_isp_boot_info_bl602_t;

typedef struct
{
    uint32_t version; /*!< bootrom version */

    struct
    {
        uint8_t sign_type[1]; /*!< sign type */
        uint8_t encrypted[1]; /*!< aes type  */
        uint8_t : 8;          /*!< reserved  */
        uint8_t : 8;          /*!< reserved  */
    } boot_efuse_hw_cfg;

    uint32_t : 32;
    uint32_t dev_info;
    uint8_t  chip_id[8];
} __attribute__((packed)) mdl_isp_boot_info_bl702_t;

typedef struct
{
    uint32_t version; /*!< bootrom version */

    struct
    {
        uint8_t sign_type[1];               /*!< sign type */
        uint8_t encrypted[1];               /*!< aes type  */
        uint8_t bus_remap[1];               /*!< bus remap */
        uint8_t flash_power_delay_level[1]; /*!< flash power delay level  */
    } boot_efuse_hw_cfg;

    uint32_t sw_cfg0;
    uint8_t  chip_id[8];
    uint32_t sw_cfg1;
} __attribute__((packed)) mdl_isp_boot_info_bl616_t;

typedef struct
{
    uint32_t version; /*!< bootrom version */

    struct
    {
        uint8_t sign_type[2]; /*!< sign type */
        uint8_t encrypted[2]; /*!< aes type  */
    } boot_efuse_hw_cfg;

    uint32_t sw_cfg0;
    uint8_t  chip_id[8];
    uint32_t sw_cfg1;
} __attribute__((packed)) mdl_isp_boot_info_bl808_t;

typedef struct
{
    /*!< bootrom version */
    uint32_t version;
    /*!< eflash loader pointer (eflash in ram, psram, flash) */
    void *eflash_loader_addr;
} mdl_isp_params_bl602_t;

typedef struct
{
    /*!< bootrom version */
    uint32_t version;
    /*!< eflash loader pointer (eflash in ram, psram, flash) */
    void *eflash_loader_addr;
} mdl_isp_params_bl702_t;

typedef struct
{
    /*!< bootrom version */
    uint32_t version;

    /*!< bl616 clock config params */
    struct __attribute__((packed))
    {
        /*!< 0x47464350 */
        uint32_t magic_code;

        /*!< 0:None,1:24M,2:32M,3:38.4M,4:40M,5:26M,6:RC32M */
        uint8_t xtal_type;

        /*!< mcu_clk 0:RC32M;1:XTAL;2:aupll_div2;3:aupll_div1;4:wifipll_240M;5:wifipll_320M */
        uint8_t mcu_clk;
        uint8_t mcu_clk_div;
        uint8_t mcu_bclk_div;
        uint8_t mcu_pbclk_div;

        /*!< 0:mcu pbclk,1:cpupll 200M,2:wifipll 320M,3:cpupll 400M */
        uint8_t emi_clk;
        uint8_t emi_clk_div;

        /*!< flash_clk_type 0:wifipll_120M;1:xtal;2:aupll_div5;3:muxpll_80M;4:bclk;5:wifipll_96M */
        uint8_t flash_clk_type;
        uint8_t flash_clk_div;

        uint8_t wifipll_pu;
        uint8_t aupll_pu;

        uint8_t rsvd0;

        /*!< 0x00000000 */
        uint32_t crc32;
    } clock_params;

    /*!< bl616 flash set params */
    struct __attribute__((packed))
    {
        /*!< flash_pin value: */
        /*!< bit 7 flash pin autoscan */
        /*!< bit 6 flash select 0: flash1, 1: flash2 */
        /*!< bit 5-0 flash pin cfg: */
        /*!< 0x0: single flash, sf1 internal swap io3 and io0 */
        /*!< 0x1: single flash, sf1 internal swap io3 with io0 and io2 with cs */
        /*!< 0x2: single flash, sf1 internal no swap */
        /*!< 0x3: single flash, sf1 internal swap io2 with cs */
        /*!< 0x4: single flash, sf2 external GPIO4-9 and swap io3 with io0 */
        /*!< 0x8: single flash, sf3 external GPIO10-15 */
        /*!< 0x14:dual flash, sf1 internal swap io3 and io0, sf2 external GPIO4-9 swap io3 with io0 */
        /*!< 0x15:dual flash, sf1 internal swap io3 with io0 and io2 with cs, sf2 external GPIO4-9 swap io3 with io0 */
        /*!< 0x16:dual flash, sf1 internal no swap, sf2 external GPIO4-9 swap io3 with io0 */
        /*!< 0x17:dual flash, sf1 internal swap io2 with cs, sf2 external GPIO4-9 swap io3 with io0 */
        /*!< 0x24:single flash, sf2 external GPIO4-9 */
        /*!< 0x34:dual flash, sf1 internal swap io3 and io0, sf2 external GPIO4-9 no swap */
        /*!< 0x35:dual flash, sf1 internal swap io3 with io0 and io2 with cs, sf2 external GPIO4-9 no swap */
        /*!< 0x36:dual flash, sf1 internal no swap, sf2 external GPIO4-9 no swap */
        /*!< 0x37:dual flash, sf1 internal swap io2 with cs, sf2 external GPIO4-9 no swap */
        uint8_t flash_pin;

        /*!< bit 7-4 flash_clock_type: 0:120M wifipll, 1:xtal, 2:128M cpupll, 3:80M wifipll, 4:bclk, 5:96M wifipll */
        /*!< bit 3-0 flash_clock_div */
        uint8_t flash_clk_cfg;

        /*!< 0:NIO, 1:DO, 2:QO, 3:DIO, 4:QIO */
        uint8_t flash_io_mode;

        /*!< 0:0.5T delay, 1:1T delay, 2:1.5T delay, 3:2T delay */
        uint8_t flash_clk_delay;
    } flash_set;

    /*!< bl616 flash config params */
    struct __attribute__((packed))
    {
        uint8_t  io_mode;                   /*!< Serail flash interface mode,bit0-3:IF mode,bit4:unwrap,bit5:32-bits addr mode support */
        uint8_t  c_read_support;            /*!< Support continuous read mode,bit0:continuous read mode support,bit1:read mode cfg */
        uint8_t  clk_delay;                 /*!< SPI clock delay,bit0-3:delay,bit4-6:pad delay */
        uint8_t  clk_invert;                /*!< SPI clock phase invert,bit0:clck invert,bit1:rx invert,bit2-4:pad delay,bit5-7:pad delay */
        uint8_t  reset_en_cmd;              /*!< Flash enable reset command */
        uint8_t  reset_cmd;                 /*!< Flash reset command */
        uint8_t  reset_c_read_cmd;          /*!< Flash reset continuous read command */
        uint8_t  reset_c_read_cmd_size;     /*!< Flash reset continuous read command size */
        uint8_t  jedec_id_cmd;              /*!< JEDEC ID command */
        uint8_t  jedec_id_cmd_dmy_clk;      /*!< JEDEC ID command dummy clock */
        uint8_t  enter_32bits_addr_cmd;     /*!< Enter 32-bits addr command */
        uint8_t  exit_32bits_addr_cmd;      /*!< Exit 32-bits addr command */
        uint8_t  sector_size;               /*!< *1024bytes */
        uint8_t  mid;                       /*!< Manufacturer ID */
        uint16_t page_size;                 /*!< Page size */
        uint8_t  chip_erase_cmd;            /*!< Chip erase cmd */
        uint8_t  sector_erase_cmd;          /*!< Sector erase command */
        uint8_t  blk32_erase_cmd;           /*!< Block 32K erase command,some Micron not support */
        uint8_t  blk64_erase_cmd;           /*!< Block 64K erase command */
        uint8_t  write_enable_cmd;          /*!< Need before every erase or program */
        uint8_t  page_program_cmd;          /*!< Page program cmd */
        uint8_t  qpage_program_cmd;         /*!< QIO page program cmd */
        uint8_t  qpp_addr_mode;             /*!< QIO page program address mode */
        uint8_t  fast_read_cmd;             /*!< Fast read command */
        uint8_t  fr_dmy_clk;                /*!< Fast read command dummy clock */
        uint8_t  qpi_fast_read_cmd;         /*!< QPI fast read command */
        uint8_t  qpi_fr_dmy_clk;            /*!< QPI fast read command dummy clock */
        uint8_t  fast_read_do_cmd;          /*!< Fast read dual output command */
        uint8_t  fr_do_dmy_clk;             /*!< Fast read dual output command dummy clock */
        uint8_t  fast_read_dio_cmd;         /*!< Fast read dual io comamnd */
        uint8_t  fr_dio_dmy_clk;            /*!< Fast read dual io command dummy clock */
        uint8_t  fast_read_qo_cmd;          /*!< Fast read quad output comamnd */
        uint8_t  fr_qo_dmy_clk;             /*!< Fast read quad output comamnd dummy clock */
        uint8_t  fast_read_qio_cmd;         /*!< Fast read quad io comamnd */
        uint8_t  fr_qio_dmy_clk;            /*!< Fast read quad io comamnd dummy clock */
        uint8_t  qpi_fast_read_qio_cmd;     /*!< QPI fast read quad io comamnd */
        uint8_t  qpi_fr_qio_dmy_clk;        /*!< QPI fast read QIO dummy clock */
        uint8_t  qpi_page_program_cmd;      /*!< QPI program command */
        uint8_t  write_vreg_enable_cmd;     /*!< Enable write reg */
        uint8_t  wr_enable_index;           /*!< Write enable register index */
        uint8_t  qe_index;                  /*!< Quad mode enable register index */
        uint8_t  busy_index;                /*!< Busy status register index */
        uint8_t  wr_enable_bit;             /*!< Write enable bit pos */
        uint8_t  qe_bit;                    /*!< Quad enable bit pos */
        uint8_t  busy_bit;                  /*!< Busy status bit pos */
        uint8_t  wr_enable_write_reg_len;   /*!< Register length of write enable */
        uint8_t  wr_enable_read_reg_len;    /*!< Register length of write enable status */
        uint8_t  qe_write_reg_len;          /*!< Register length of contain quad enable */
        uint8_t  qe_read_reg_len;           /*!< Register length of contain quad enable status */
        uint8_t  release_powerdown;         /*!< Release power down command */
        uint8_t  busy_read_reg_len;         /*!< Register length of contain busy status */
        uint8_t  read_reg_cmd[4];           /*!< Read register command buffer */
        uint8_t  write_reg_cmd[4];          /*!< Write register command buffer */
        uint8_t  enter_qpi;                 /*!< Enter qpi command */
        uint8_t  exit_qpi;                  /*!< Exit qpi command */
        uint8_t  c_read_mode;               /*!< Config data for continuous read mode */
        uint8_t  c_rexit;                   /*!< Config data for exit continuous read mode */
        uint8_t  burst_wrap_cmd;            /*!< Enable burst wrap command */
        uint8_t  burst_wrap_cmd_dmy_clk;    /*!< Enable burst wrap command dummy clock */
        uint8_t  burst_wrap_data_mode;      /*!< Data and address mode for this command */
        uint8_t  burst_wrap_data;           /*!< Data to enable burst wrap */
        uint8_t  de_burst_wrap_cmd;         /*!< Disable burst wrap command */
        uint8_t  de_burst_wrap_cmd_dmy_clk; /*!< Disable burst wrap command dummy clock */
        uint8_t  de_burst_wrap_data_mode;   /*!< Data and address mode for this command */
        uint8_t  de_burst_wrap_data;        /*!< Data to disable burst wrap */
        uint16_t time_e_sector;             /*!< 4K erase time */
        uint16_t time_e_32k;                /*!< 32K erase time */
        uint16_t time_e_64k;                /*!< 64K erase time */
        uint16_t time_page_pgm;             /*!< Page program time */
        uint16_t time_ce;                   /*!< Chip erase time in ms */
        uint8_t  pd_delay;                  /*!< Release power down command delay time for wake up */
        uint8_t  qe_data;                   /*!< QE set data */
    } flash_params;

} mdl_isp_params_bl616_t;

typedef struct
{
    /*!< bootrom version */
    uint32_t version;
} mdl_isp_params_bl808_t;

typedef struct
{
    union
    {
        mdl_isp_boot_info_generic_t boot_info_generic;
        mdl_isp_boot_info_bl602_t   boot_info_bl602;
        mdl_isp_boot_info_bl702_t   boot_info_bl702;
        mdl_isp_boot_info_bl616_t   boot_info_bl616;
        mdl_isp_boot_info_bl808_t   boot_info_bl808;
    };

    uint32_t jedecid;
    uint32_t baudrate_low;
    uint32_t baudrate_high;

    int (*config_baudrate)(uint32_t baudrate);
    void (*wait)(uint32_t wait);
    
    void (*event_callback)(uint32_t event);

    int (*send_alloc)(void **data, uint32_t wait);
    int (*send)(void *data, uint32_t size, uint32_t wait);
    int (*send_done)(uint32_t wait);
    int (*recv)(void **data, uint32_t *size, uint32_t wait);
    int (*recv_done)(void *data);
} mdl_isp_t;

extern int mdl_isp_shakehand(mdl_isp_t *isp);
extern int mdl_isp_get_bootinfo(mdl_isp_t *isp, void *boot_info);
extern int mdl_isp_set_chip_ready(mdl_isp_t *isp, void *params);

extern int mdl_isp_flash_erase(mdl_isp_t *isp, uint32_t start_addr, uint32_t end_addr, uint32_t wait);
extern int mdl_isp_flash_erase_chip(mdl_isp_t *isp, uint32_t wait);
extern int mdl_isp_flash_program(mdl_isp_t *isp, uint32_t addr, void *data, uint32_t size);
extern int mdl_isp_flash_program_check(mdl_isp_t *isp);
extern int mdl_isp_flash_get_sha256(mdl_isp_t *isp, uint32_t addr, uint32_t size, uint8_t *sha256);

extern int mdl_isp_flash_xip_enter(mdl_isp_t *isp);
extern int mdl_isp_flash_xip_get_sha256(mdl_isp_t *isp, uint32_t addr, uint32_t size, uint8_t *sha256);
extern int mdl_isp_flash_xip_exit(mdl_isp_t *isp);

#endif