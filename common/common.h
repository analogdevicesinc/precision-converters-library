/***************************************************************************//*
 * @file    common.h
 * @brief   Precision converters firmware common headers
******************************************************************************
 * Copyright (c) 2023 Analog Devices, Inc.
 * All rights reserved.
 *
 * This software is proprietary to Analog Devices, Inc. and its licensors.
 * By using this software you agree to the terms of the associated
 * Analog Devices Software License Agreement.
******************************************************************************/

#ifndef COMMON_H_
#define COMMON_H_

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "iio.h"
#include "24xx32a.h"
#include "no_os_eeprom.h"
#include "no_os_error.h"

/******************************************************************************/
/********************** Macros and Constants Definition ***********************/
/******************************************************************************/

#if defined (TARGET_SDP_K1)
/* SDRAM configs for SDP-K1 */
#define SDRAM_START_ADDRESS		(volatile int8_t *)0xC0000000
#define SDRAM_SIZE_BYTES		(16777216)	// 16MBytes
#endif

/* EEPROM valid device address range */
#define EEPROM_DEV_ADDR_START	0x50
#define EEPROM_DEV_ADDR_END		0x57

/* Last accessible EEPROM address/location (default 32Kbit EEPROM size ) */
#define MAX_REGISTER_ADDRESS	0xFFF

/* Macros for stringification */
#define XSTR(s)		#s
#define STR(s)		XSTR(s)

/******************************************************************************/
/********************** Public/Extern Declarations ****************************/
/******************************************************************************/

int32_t get_iio_context_attributes(struct iio_ctx_attr **ctx_attr,
				   uint32_t *attrs_cnt,
				   struct no_os_eeprom_desc *eeprom_desc,
				   const char *hw_mezzanine, const char *hw_carrier,
				   bool *hw_mezzanine_is_valid);
int32_t eeprom_init(struct no_os_eeprom_desc **eeprom_desc,
		    struct no_os_eeprom_init_param *eeprom_init_params);
uint8_t get_eeprom_detected_dev_addr(void);
bool is_eeprom_valid_dev_addr_detected(void);
int32_t sdram_init(void);

#endif /* COMMON_H_ */
