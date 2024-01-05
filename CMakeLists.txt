# set minimum cmake version
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

set(CROSS_COMPILE)
# set(CROSS_COMPILE arm-linux-gnueabihf-)

SET(CMAKE_C_COMPILER ${CROSS_COMPILE}gcc CACHE INTERNAL "")
SET(CMAKE_CXX_COMPILER ${CROSS_COMPILE}g++ CACHE INTERNAL "")

# project name and language
project(bouffalo_isp LANGUAGES C)

add_executable(bouffalo_isp main.c 
isp/mdl_isp.c 
isp/mdl_isp_bl602_eflash_loader_40m_nolog.c 
isp/mdl_isp_bl702_eflash_loader_32m.c
fw_util.c
)