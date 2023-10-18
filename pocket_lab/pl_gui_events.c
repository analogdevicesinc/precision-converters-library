/***************************************************************************//**
 *   @file    pl_gui_events.c
 *   @brief   Pocket lab GUI event handling
********************************************************************************
 * Copyright (c) 2023 Analog Devices, Inc.
 *
 * This software is proprietary to Analog Devices, Inc. and its licensors.
 * By using this software you agree to the terms of the associated
 * Analog Devices Software License Agreement.
*******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "pl_gui_events.h"
#include "pl_gui_views.h"
#include "no_os_delay.h"

/******************************************************************************/
/************************ Macros/Constants ************************************/
/******************************************************************************/

/******************************************************************************/
/******************** Variables and User Defined Data Types *******************/
/******************************************************************************/

/* Capture states */
enum pl_gui_capture_state {
	PL_GUI_PREPARE_CAPTURE,
	PL_GUI_START_CAPTURE,
	PL_GUI_END_CAPTURE,
};

/* Flag indicating if event write command is formed */
static bool pl_gui_cmd_formed;
/* Index to event command string */
static uint32_t pl_gui_cmd_str_indx;
/* Command string buffer */
static uint8_t pl_gui_cmd_str[100];
/* Number of new line characters. Newline characters are used as end of
 **command indicator by IIO library */
static uint32_t pl_gui_nb_newln_ch;
/* Current capture state */
static enum pl_gui_capture_state pl_gui_cur_capture_state;
/* Number of data bytes to read */
static uint32_t pl_gui_nb_data_bytes;

/******************************************************************************/
/************************ Functions Prototypes ********************************/
/******************************************************************************/

/******************************************************************************/
/************************ Functions Definitions *******************************/
/******************************************************************************/

/**
 * @brief 	Update lvgl tick timer
 * @param	tick_time[in] - tick time in msec
 * @return	None
 */
void pl_gui_lvgl_tick_update(uint32_t tick_time)
{
	lv_tick_inc(tick_time);
}

/**
 * @brief 	Read the pocket lab GUI event and form (an IIO) command string
 * @param	buf[in] - I/O buffer
 * @param	len[in] - Number of bytes to read
 * @return	Number of bytes read
 */
int32_t pl_gui_event_read(uint8_t *buf, uint32_t len)
{
	uint32_t chn_mask;
	uint32_t chn_mask_temp;
	uint32_t cnt;
	uint32_t dev_indx;
	struct scan_type chn_info;

	if (!buf) {
		return 0;
	}

	/* Wait until capture event is triggered */
	if (!pl_gui_is_capture_running() && !pl_gui_is_fft_running()) {
		if (pl_gui_cur_capture_state == PL_GUI_START_CAPTURE) {
			/* Form end capture command if capture was active previously */
			sprintf(pl_gui_cmd_str, "CLOSE iio:device%d\r\n", dev_indx);
			pl_gui_cur_capture_state = PL_GUI_END_CAPTURE;
		}

		if (pl_gui_cur_capture_state == PL_GUI_END_CAPTURE) {
			/* Copy capture stop command string into input buffer */
			memcpy(buf, &pl_gui_cmd_str[pl_gui_cmd_str_indx], len);
			pl_gui_cmd_str_indx += len;
			return len;
		}

		return 0;
	} else { // Monitor and perform capture until capture stop event occured
		/* Handle capture commands */
		if (!pl_gui_cmd_formed) {
			switch (pl_gui_cur_capture_state) {
			/* Form prepare capture command */
			case PL_GUI_PREPARE_CAPTURE:
				pl_gui_get_capture_chns_mask(&chn_mask);
				dev_indx = pl_gui_get_active_device_index();
				pl_gui_nb_data_bytes = 0;
				cnt = 0;
				chn_mask_temp = chn_mask;
				while (chn_mask_temp) {
					if (chn_mask_temp & 0x1) {
						pl_gui_read_chn_info(&chn_info, cnt, dev_indx);
						pl_gui_nb_data_bytes += (chn_info.storagebits >> 3);
						pl_gui_store_chn_info(&chn_info, cnt);
					}
					chn_mask_temp >>= 1;
					cnt++;
				}

				pl_gui_nb_data_bytes *= get_data_samples_count();

				sprintf(pl_gui_cmd_str, "OPEN iio:device%d %d %08x\r\n",
					dev_indx, get_data_samples_count(), chn_mask);
				pl_gui_cmd_formed = true;
				break;

			case PL_GUI_START_CAPTURE:
				/* Form read buffer command */
				sprintf(pl_gui_cmd_str, "READBUF iio:device%d %d\r\n", dev_indx,
					pl_gui_nb_data_bytes);
				pl_gui_cmd_formed = true;
				pl_gui_nb_newln_ch = 0;
				break;

			default:
				break;
			}
		}

		/* Copy command string into input buffer of size equal to 'len' */
		memcpy(buf, &pl_gui_cmd_str[pl_gui_cmd_str_indx], len);
		pl_gui_cmd_str_indx += len;

		return len;
	}

	return 0;
}

/**
 * @brief 	Form the response for previous pocket lab GUI event (an IIO)
 *			command string
 * @param	buf[in] - I/O buffer
 * @param	len[in] - Number of bytes to write
 * @return	Number of bytes written
 */
int32_t pl_gui_event_write(uint8_t *buf, uint32_t len)
{
	pl_gui_cmd_formed = false;
	pl_gui_cmd_str_indx = 0;

	if (!buf || (!pl_gui_is_capture_running() && !pl_gui_is_fft_running())) {
		pl_gui_cur_capture_state = PL_GUI_PREPARE_CAPTURE;
		return len;
	}

	/* Handle capture state */
	switch (pl_gui_cur_capture_state) {
	case PL_GUI_PREPARE_CAPTURE:
		if (buf[0] == '\n') {
			pl_gui_cur_capture_state = PL_GUI_START_CAPTURE;
		}
		break;

	case PL_GUI_START_CAPTURE:
		if (buf[0] == '\n') {
			pl_gui_nb_newln_ch++;
			break;
		}

		if (pl_gui_nb_newln_ch >= 2) {
			/* Offload the buffer data onto GUI display */
			pl_gui_display_captured_data(buf, len);
		}
		break;

	case PL_GUI_END_CAPTURE:
		if (buf[0] == '\n') {
			pl_gui_cur_capture_state = PL_GUI_PREPARE_CAPTURE;
		}
		break;
	}

	return len;
}

/**
 * @brief 	Handle lvgl GUI events
 * @param	tick_time[in] - tick time in msec
 * @return	None
 */
void pl_gui_event_handle(uint32_t tick_time)
{
	if (pl_gui_is_dmm_running()) {
		pl_gui_perform_dmm_read();
	}

	no_os_mdelay(tick_time);
	lv_task_handler();
}
