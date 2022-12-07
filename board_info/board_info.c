/***************************************************************************//**
 *   @file    board_info.c
 *   @brief   Hardware board information read and parse module
********************************************************************************
 * Copyright 2022(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "board_info.h"
#include "no_os_error.h"

/******************************************************************************/
/************************ Macros/Constants ************************************/
/******************************************************************************/

/* SDP EEPROM specific macros */
#define SDP_EEPROM_DATA_MAX_LEN			256
#define SDP_EEPROM_LEGACY_BOARD_ID_LEN	18
#define SDP_EEPROM_HEADER_LEN			10
#define SDP_EEPROM_HEADER_INDX			8
#define SDP_EEPROM_DATA_INDX			10
#define SDP_EEPROM_RECORD_FOOTER_LEN	3

/******************************************************************************/
/******************** Variables and User Defined Data Types *******************/
/******************************************************************************/

/******************************************************************************/
/************************** Functions Declarations ****************************/
/******************************************************************************/

static int32_t read_and_parse_sdp_eeprom(struct no_os_eeprom_desc *desc,
		struct board_info *board_info);

/******************************************************************************/
/************************** Functions Definitions *****************************/
/******************************************************************************/

/**
 * @brief 	Read board information from EEPROM
 * @param	desc[in] - EEPROM descriptor
 * @param	board_info[in, out] - Pointer to board info structure
 * @return	0 in case of success, negative error code otherwise
 */
int32_t read_board_info(struct no_os_eeprom_desc *desc,
			struct board_info *board_info)
{
	int32_t ret;

	if (!desc || !board_info) {
		return -EINVAL;
	}

	do {
		/* Read and parse SDP EEPROM format */
		ret = read_and_parse_sdp_eeprom(desc, board_info);
		if (!ret) {
			break;
		}

		// TODO - Read and parse other EEPROM formats

		/* If unable to find valid EEPROM format, return error */
		return ret;
	} while (0);

	return 0;
}

/**
 * @brief 	Read and parse SDP EEPROM data format
 * @param	desc[in] - EEPROM descriptor
 * @param	board_info[in, out] - Pointer to board info structure
 * @return	0 in case of success, negative error code otherwise
 */
static int32_t read_and_parse_sdp_eeprom(struct no_os_eeprom_desc *desc,
		struct board_info *board_info)
{
	int32_t ret;
	uint8_t data_len;
	uint8_t data_parse_len;
	uint8_t record_type;
	uint8_t record_len;
	uint8_t record_data_len = 0;
	uint8_t index = 0;
	uint8_t data_index = 0;
	uint32_t address = 0x0;
	char hw_id[SDP_EEPROM_LEGACY_BOARD_ID_LEN];
	char eeprom_data[SDP_EEPROM_DATA_MAX_LEN];

	if (!desc || !board_info) {
		return -EINVAL;
	}

	/* Read EEPROM header information */
	ret = no_os_eeprom_read(desc, address, (uint8_t *)eeprom_data,
				SDP_EEPROM_HEADER_LEN);
	if (ret) {
		return ret;
	}

	/* Validate if correct SDP EEPROM format */
	if (strcmp(eeprom_data, "ADISDP")) {
		return -EINVAL;
	}

	data_len = eeprom_data[SDP_EEPROM_HEADER_INDX];
	data_parse_len = data_len - SDP_EEPROM_HEADER_LEN;

	/* Read EEPROM data */
	address = SDP_EEPROM_DATA_INDX;
	ret = no_os_eeprom_read(desc, address, (uint8_t *)eeprom_data,
				data_parse_len);
	if (ret) {
		return ret;
	}

	/* Parse the EEPROM data */
	while (index < data_parse_len) {
		record_type = eeprom_data[index];
		record_len = eeprom_data[index + 1] | (eeprom_data[index + 2] << 8);
		record_data_len = record_len - SDP_EEPROM_RECORD_FOOTER_LEN;

		switch (record_type) {
		case 0x01:
			/* Hardware ID for Legacy EVBs */
			hw_id[0] = (eeprom_data[index + SDP_EEPROM_RECORD_FOOTER_LEN + 1]);
			hw_id[1] = (eeprom_data[index + SDP_EEPROM_RECORD_FOOTER_LEN + 0]);
			hw_id[2] = (eeprom_data[index + SDP_EEPROM_RECORD_FOOTER_LEN + 3]);
			hw_id[3] = (eeprom_data[index + SDP_EEPROM_RECORD_FOOTER_LEN + 2]);

			hw_id[4] = (eeprom_data[index + SDP_EEPROM_RECORD_FOOTER_LEN + 7]);
			hw_id[5] = (eeprom_data[index + SDP_EEPROM_RECORD_FOOTER_LEN + 6]);
			hw_id[6] = (eeprom_data[index + SDP_EEPROM_RECORD_FOOTER_LEN + 5]);
			hw_id[7] = (eeprom_data[index + SDP_EEPROM_RECORD_FOOTER_LEN + 4]);

			sprintf(board_info->board_id,
				"0x%02X%02X%02X%02X%02X%02X%02X%02X",
				hw_id[0],hw_id[1],hw_id[2],hw_id[3],
				hw_id[4],hw_id[5],hw_id[6],hw_id[7]);
			break;

		case 0x02:
			/* EVB ID info */
			data_index = 0;
			while (data_index < record_data_len) {
				board_info->board_id[data_index] = eeprom_data[index +
								   SDP_EEPROM_RECORD_FOOTER_LEN
								   + data_index];
				data_index++;
			}
			board_info->board_id[data_index] = '\0';
			break;

		case 0x03:
			/* EVB name info */
			data_index = 0;
			while (data_index < record_data_len) {
				board_info->board_name[data_index] = eeprom_data[index +
								     SDP_EEPROM_RECORD_FOOTER_LEN
								     + data_index];
				data_index++;
			}
			board_info->board_name[data_index] = '\0';
			return 0;

		case 0x04:
		case 0x05:
		case 0x0D:
		case 0x0E:
		case 0x0F:
			/* Valid but unused record types */
			break;

		default:
			/* Invalid record */
			return -EINVAL;
		}

		index += record_len;
	}

	return 0;
}
