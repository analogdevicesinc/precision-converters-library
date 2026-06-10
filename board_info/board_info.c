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
#include "no_os_util.h"

/* lib-hatplus headers */
#include "hatplus.h"
#include "atom.h"
#include "head.h"
#include "vendor_info.h"
#include "tlv.h"

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

/* lib-hatplus configuration */
#define MAX_ATOM_SIZE   1024


/******************************************************************************/
/******************** Variables and User Defined Data Types *******************/
/******************************************************************************/

/**
 * @struct 	hatplus_no_os_context
 * @brief 	Platform context for lib-hatplus - embeds struct hatplus
 * @note 	Uses container pattern to link lib-hatplus context with no_os_eeprom
 */
struct hatplus_no_os_context {
	struct no_os_eeprom_desc *eeprom_desc;	/**< no-OS EEPROM descriptor */
	struct hatplus hatplus;			/**< lib-hatplus context (embedded) */
};

/******************************************************************************/
/************************** Functions Declarations ****************************/
/******************************************************************************/

static int32_t read_and_parse_sdp_eeprom(struct no_os_eeprom_desc *desc,
		struct board_info *board_info);

static int32_t read_and_parse_rpi_hat_plus_eeprom(struct no_os_eeprom_desc *desc,
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

		/* Read and parse RPI HAT+ EEPROM format */
		ret = read_and_parse_rpi_hat_plus_eeprom(desc, board_info);
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
			/* EVB name info */
			data_index = 0;
			while (data_index < record_data_len) {
				board_info->board_name[data_index] = eeprom_data[index +
								     SDP_EEPROM_RECORD_FOOTER_LEN
								     + data_index];
				data_index++;
			}
			board_info->board_name[data_index] = '\0';
			break;

		case 0x03:
		case 0x04:
		case 0x05:
		case 0x0D:
			break;
		case 0x0E:
			// SAP code ID
			data_index = 0;
			while (data_index < record_data_len) {
				board_info->board_id[data_index] = eeprom_data[index +
								   SDP_EEPROM_RECORD_FOOTER_LEN + data_index];
				data_index++;
			}
			board_info->board_id[data_index] = '\0';
			break;
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

/**
 * @brief 	Platform ops callback - sequential read from no_os_eeprom
 * @param 	hp[in] - lib-hatplus context pointer
 * @param 	address[in] - EEPROM address to read from
 * @param 	data[out] - Buffer to store read data
 * @param 	count[in] - Number of bytes to read
 * @return 	Number of bytes read on success, negative error code otherwise
 * @note 	Uses container pattern to upcast and access no_os_eeprom_desc
 */
static int hatplus_no_os_seq_read(struct hatplus *hp,
				   const unsigned long address,
				   uint8_t *data,
				   const size_t count)
{
	struct hatplus_no_os_context *ctx;
	int32_t ret;

	if (!hp || !data)
		return -EINVAL;

	ctx = hatplus_container(hp, struct hatplus_no_os_context, hatplus);

	ret = no_os_eeprom_read(ctx->eeprom_desc, address, data, count);

	/* API conversion: no_os_eeprom_read returns 0 on success,
	 * but lib-hatplus expects number of bytes read (POSIX-style) */
	if (ret < 0)
		return ret;  /* Return error as-is */

	return count;  /* Success: return number of bytes read */
}

/**
 * @brief 	Platform ops structure for lib-hatplus
 * @note 	Only seq_read is implemented; write operations not needed for reading
 */
static struct hatplus_eep_ops hatplus_no_os_ops = {
	.seq_read = hatplus_no_os_seq_read,
	.write8 = NULL,
	.write = NULL,
	.write_protect = NULL,
	.write_unprotect = NULL,
};

/**
 * @brief 	Process vendor info atom to extract board name and product name
 * @param 	atom[in] - Vendor info atom from HAT+ EEPROM
 * @param 	board_info[in, out] - Board info structure to populate
 * @return 	0 in case of success, negative error code otherwise
 * @note 	Extracts product name and copies to both board_id and board_name
 */
static int32_t process_vendor_info_atom(struct hatplus_atom *atom,
		struct board_info *board_info)
{
	char product_str[BOARD_NAME_MAX_LEN];
	int ret;

	ret = hatplus_vinfo_atom_get_product(atom, product_str,
					     sizeof(product_str));
	if (ret < 0)
		return ret;

	strncpy(board_info->board_id, product_str, BOARD_ID_MAX_LEN - 1);
	board_info->board_id[BOARD_ID_MAX_LEN - 1] = '\0';

	strncpy(board_info->board_name, product_str, BOARD_NAME_MAX_LEN - 1);
	board_info->board_name[BOARD_NAME_MAX_LEN - 1] = '\0';

	return 0;
}

/**
 * @brief 	Process custom data atom to extract dongle features
 * @param 	atom[in] - Custom data atom from HAT+ EEPROM
 * @param 	board_info[in, out] - Board info structure to populate
 * @return 	0 in case of success, negative error code otherwise
 * @note 	Parses TLV data to extract board name override (tag 0x01) and
 * 		comma-separated feature list (tag 0x02)
 */
static int32_t process_custom_data_atom(struct hatplus_atom *atom,
		struct board_info *board_info)
{
	struct hatplus_tlv *tlv;
	int tag;
	int len;
	char *value_str;
	char *token_start;
	char *token_end;
	size_t token_len;
	uint32_t feature_count = 0;

	tlv = hatplus_atom_payload(atom);
	if (!tlv)
		return -EINVAL;

	while (tlv && feature_count < MAX_DONGLE_FEATURE) {
		tag = hatplus_tlv_tag(tlv);
		len = hatplus_tlv_len(tlv);

		if (tag == HATPLUS_TLV_TAG_INVALID)
			break;

		switch (tag) {
		case 0x01: {
			value_str = (char *)hatplus_tlv_value(tlv);
			if (value_str && len < BOARD_NAME_MAX_LEN) {
				memcpy(board_info->board_name, value_str, len);
				board_info->board_name[len] = '\0';
			}
			break;
		}

		case 0x02: {
			value_str = (char *)hatplus_tlv_value(tlv);
			if (!value_str || len == 0)
				break;

			char feature_buf[256];
			if (len >= sizeof(feature_buf))
				break;

			memcpy(feature_buf, value_str, len);
			feature_buf[len] = '\0';

			token_start = feature_buf;
			while (feature_count < MAX_DONGLE_FEATURE && token_start && *token_start) {
				token_end = strchr(token_start, ',');
				if (token_end)
					token_len = (size_t)(token_end - token_start);
				else
					token_len = strlen(token_start);

				if (token_len > 0) {
					board_info->dongle_features[feature_count] = malloc(token_len + 1);
					if (board_info->dongle_features[feature_count]) {
						memcpy(board_info->dongle_features[feature_count],
						       token_start, token_len);
						board_info->dongle_features[feature_count][token_len] = '\0';
						feature_count++;
					}
				}

				if (!token_end)
					break;

				token_start = token_end + 1;
			}
			break;
		}

		default:
			break;
		}

		tlv = hatplus_tlv_next(tlv);
	}

	return 0;
}

/**
 * @brief 	Read and parse RPI HAT+ EEPROM using lib-hatplus
 * @param	desc[in] - EEPROM descriptor
 * @param	board_info[in, out] - Pointer to board info structure
 * @return	0 in case of success, negative error code otherwise
 */
static int32_t read_and_parse_rpi_hat_plus_eeprom(struct no_os_eeprom_desc *desc,
		struct board_info *board_info)
{
	struct hatplus_no_os_context ctx;
	HATPLUS_DECLARE_HEADER(header_buffer);
	struct hatplus_eep_header *header;
	HATPLUS_DECLARE_ATOM(atom_buffer, MAX_ATOM_SIZE);
	struct hatplus_atom *atom;
	int32_t ret;
	int num_atoms;
	int atom_type;
	int i;

	if (!desc || !board_info)
		return -EINVAL;

	ctx.eeprom_desc = desc;

	ret = hatplus_init(&ctx.hatplus, &hatplus_no_os_ops);
	if (ret)
		return ret;

	header = hatplus_header_init(header_buffer);
	ret = hatplus_get_head(&ctx.hatplus, header);
	if (ret)
		goto exit;

	num_atoms = hatplus_header_get_numatoms(header);

	if (num_atoms <= 0) {
		ret = -EINVAL;
		goto exit;
	}

	atom = hatplus_atom_init(atom_buffer);

	for (i = 0; i < num_atoms; i++) {
		ret = hatplus_get_atom(&ctx.hatplus, atom, sizeof(atom_buffer), i);
		if (ret)
			continue;

		if (hatplus_atom_integrity_check(atom) != 0) {
			continue;
		}

		atom_type = hatplus_atom_get_type(atom);

		switch (atom_type) {
		case HATPLUS_ATOM_VENDOR:
			ret = process_vendor_info_atom(atom, board_info);
			break;

		case HATPLUS_ATOM_CUSTOM_DATA:
			ret = process_custom_data_atom(atom, board_info);
			break;

		case HATPLUS_ATOM_DT_OVERLAY:
			/* Device tree overlay - not used for board_info */
			break;

		default:
			break;
		}
	}

	ret = 0;

exit:
	hatplus_exit(&ctx.hatplus);
	return ret;
}
