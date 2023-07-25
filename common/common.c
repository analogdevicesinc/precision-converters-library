/***************************************************************************//**
 * @file    common.c
 * @brief   Precision converters firmware common functions
********************************************************************************
* Copyright (c) 2023 Analog Devices, Inc.
* All rights reserved.
*
* This software is proprietary to Analog Devices, Inc. and its licensors.
* By using this software you agree to the terms of the associated
* Analog Devices Software License Agreement.
*******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/

#include "common.h"
#include "board_info.h"
#if defined (TARGET_SDP_K1)
#include "sdp_k1_sdram.h"
#endif

/******************************************************************************/
/********************* Macros and Constants Definition ************************/
/******************************************************************************/

/* This value is calculated for SDP-K1 eval board (STM32F469NI MCU)
 * at 180Mhz core clock frequency */
#define EEPROM_OPS_START_DELAY		0xfffff

/******************************************************************************/
/******************** Variables and User Defined Data Types *******************/
/******************************************************************************/

/* Context attributes ID */
enum context_attr_ids {
	HW_MEZZANINE_ID,
	HW_CARRIER_ID,
	HW_NAME_ID,
	DEF_NUM_OF_CONTXT_ATTRS
};

/* Hardware board information */
static struct board_info board_info;

/* Valid EEPROM device address detected by firmware */
static uint8_t eeprom_detected_dev_addr;
static bool valid_eeprom_addr_detected;

/******************************************************************************/
/************************** Functions Definitions *****************************/
/******************************************************************************/

/**
 * @brief 	Initialize the EEPROM
 * @param	eeprom_desc[in] - EEPROM descriptor
 * @param	extra[in] - EEPROM extra init param structure pointer
 *			(interface type such as i2c)
 * @return	0 in case of success, negative error code otherwise
 */
int32_t eeprom_init(struct no_os_eeprom_desc **eeprom_desc,
		    struct no_os_eeprom_init_param *eeprom_init_params)
{
	int32_t ret;
	static volatile uint32_t cnt;

	if (!eeprom_desc || !eeprom_init_params) {
		return -EINVAL;
	}

#if defined (TARGET_SDP_K1)
	/* ~100msec Delay before starting EEPROM operations for SDP-K1.
	 * This delay makes sure that MCU is stable after power on
	 * cycle before doing any EEPROM operations */
	for (cnt = 0; cnt < EEPROM_OPS_START_DELAY; cnt++) ;
#endif

	ret = no_os_eeprom_init(eeprom_desc, eeprom_init_params);
	if (ret) {
		return ret;
	}

	return 0;
}

/**
 * @brief 	Store the EEPROM device address
 * @param	eeprom_desc[in] - EEPROM descriptor
 * @param	dev_addr[in] - EEPROM device address
 * @return	0 in case of success, negative error code otherwise
 */
int32_t load_eeprom_dev_address(struct no_os_eeprom_desc *eeprom_desc,
				uint8_t dev_addr)
{
	// TODO Add this function as a part of eeprom drivers layer
	struct eeprom_24xx32a_dev *eeprom_dev;

	if (!eeprom_desc) {
		return -EINVAL;
	}

	eeprom_dev = eeprom_desc->extra;

#if (ACTIVE_PLATFORM == MBED_PLATFORM)
	/* Left shift by 1 to get 7-bit address (7 MSBs)
	 * The LSB (0th bit) acts as R/W bit */
	eeprom_dev->i2c_desc->slave_address = dev_addr << 1;
#else
	eeprom_dev->i2c_desc->slave_address = dev_addr;
#endif

	return 0;
}

/**
 * @brief 	Validate the EEPROM device address
 * @param	eeprom_desc[in] - EEPROM descriptor
 * @return	0 in case of success, negative error code otherwise
 */
static int32_t validate_eeprom(struct no_os_eeprom_desc *eeprom_desc)
{
	int32_t ret;
	static volatile uint8_t eeprom_addr;
	static volatile uint8_t dummy_data;

	if (!eeprom_desc) {
		return -EINVAL;
	}

	/* Detect valid EEPROM */
	valid_eeprom_addr_detected = false;
	for (eeprom_addr = EEPROM_DEV_ADDR_START;
	     eeprom_addr <= EEPROM_DEV_ADDR_END; eeprom_addr++) {
		ret = load_eeprom_dev_address(eeprom_desc, eeprom_addr);
		if (ret) {
			return ret;
		}

		ret = no_os_eeprom_read(eeprom_desc, 0, (uint8_t *)&dummy_data, 1);
		if (!ret) {
			/* Valid EEPROM address detected */
			eeprom_detected_dev_addr = eeprom_addr;
			valid_eeprom_addr_detected = true;
			break;
		}
	}

	if (!valid_eeprom_addr_detected) {
		printf("No valid EEPROM address detected\r\n");
	} else {
		printf("Valid EEPROM address detected: 0x%x\r\n", eeprom_addr);
	}

	return 0;
}

/**
 * @brief 	Return the flag indicating if valid EEPROM address is detected
 * @return	EEPROM valid address detect flag (true/false)
 */
bool is_eeprom_valid_dev_addr_detected(void)
{
	return valid_eeprom_addr_detected;
}

/**
 * @brief 	Get the EEPROM device address detected by firmware
 * @return	EEPROM device address
 */
uint8_t get_eeprom_detected_dev_addr(void)
{
	return eeprom_detected_dev_addr;
}

/**
 * @brief	Read IIO context attributes
 * @param 	ctx_attr[in,out] - Pointer to IIO context attributes init param
 * @param	attrs_cnt[in,out] - IIO contxt attributes count
 * @param	eeprom_desc[in] - EEPROM descriptor
 * @param	hw_mezzanine[in,out] - HW Mezzanine ID string
 * @param	hw_carrier[in,out] - HW Carrier ID string
 * @param	hw_mezzanine_is_valid[in,out] - HW Mezzanine valid status
 * @return	0 in case of success, negative error code otherwise
 */
int32_t get_iio_context_attributes(struct iio_ctx_attr **ctx_attr,
				   uint32_t *attrs_cnt,
				   struct no_os_eeprom_desc *eeprom_desc,
				   const char *hw_mezzanine, const char *hw_carrier,
				   bool *hw_mezzanine_is_valid)
{
	int32_t ret;
	struct iio_ctx_attr *context_attributes;
	const char *board_status;
	uint8_t num_of_context_attributes = DEF_NUM_OF_CONTXT_ATTRS;
	uint8_t cnt = 0;
	bool board_detect_error = false;

	if (!ctx_attr || !attrs_cnt || !hw_carrier || !hw_mezzanine_is_valid) {
		return -EINVAL;
	}

	ret = validate_eeprom(eeprom_desc);
	if (ret) {
		return ret;
	}

	if (is_eeprom_valid_dev_addr_detected()) {
		/* Read the board information from EEPROM */
		ret = read_board_info(eeprom_desc, &board_info);
		if (ret) {
			board_detect_error = true;
		} else {
			if (hw_mezzanine == NULL) {
				if (board_info.board_id[0] != '\0') {
					*hw_mezzanine_is_valid = true;
				} else {
					board_detect_error = true;
				}
			} else {
				if (!strcmp(board_info.board_id, hw_mezzanine)) {
					*hw_mezzanine_is_valid = true;
				} else {
					*hw_mezzanine_is_valid = false;
					board_status = "mismatch";
					num_of_context_attributes++;
				}
			}
		}
	} else {
		board_detect_error = true;
	}

	if (board_detect_error) {
		*hw_mezzanine_is_valid = false;
		board_status = "not_detected";
		num_of_context_attributes++;
	}

#if defined(FIRMWARE_VERSION)
	num_of_context_attributes++;
#endif

	/* Allocate dynamic memory for context attributes based on number of attributes
	 * detected/available */
	context_attributes = (struct iio_ctx_attr *)calloc(
				     num_of_context_attributes,
				     sizeof(*context_attributes));
	if (!context_attributes) {
		return -ENOMEM;
	}

#if defined(FIRMWARE_VERSION)
	(context_attributes + cnt)->name = "fw_version";
	(context_attributes + cnt)->value = STR(FIRMWARE_VERSION);
	cnt++;
#endif

	(context_attributes + cnt)->name = "hw_carrier";
	(context_attributes + cnt)->value = hw_carrier;
	cnt++;

	if (board_info.board_id[0] != '\0') {
		(context_attributes + cnt)->name = "hw_mezzanine";
		(context_attributes + cnt)->value = board_info.board_id;
		cnt++;
	}

	if (board_info.board_name[0] != '\0') {
		(context_attributes + cnt)->name = "hw_name";
		(context_attributes + cnt)->value = board_info.board_name;
		cnt++;
	}

	if (!*hw_mezzanine_is_valid) {
		(context_attributes + cnt)->name = "hw_mezzanine_status";
		(context_attributes + cnt)->value = board_status;
		cnt++;
	}

	num_of_context_attributes = cnt;
	*ctx_attr = context_attributes;
	*attrs_cnt = num_of_context_attributes;

	return 0;
}

/**
 * @brief 	Initialize the SDP-K1 SDRAM
 * @return	0 in case of success, negative error code otherwise
 */
int32_t sdram_init(void)
{
#if defined (TARGET_SDP_K1)
	if (SDP_SDRAM_Init() != SDRAM_OK) {
		return -EIO;
	}
#endif

	return 0;
}
