/***************************************************************************//**
 *   @file    pl_gui_views.c
 *   @brief   Pocket lab GUI views
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
#include <stdint.h>

#include "pl_gui_views.h"
#include "pl_gui_iio_wrapper.h"
#include "adi_fft.h"
#include "no_os_delay.h"
#include "no_os_util.h"
#include "no_os_error.h"
#include "stm32f769i_discovery.h"
#include "tft.h"
#include "touchpad.h"
#include "lvgl/lvgl.h"
#include "lv_conf.h"

/******************************************************************************/
/************************ Macros/Constants ************************************/
/******************************************************************************/

/* Max scale values that can be attached to lvgl chart wizard
 * are less than 2^32 (less than 9 digits), so displaying scale for
 * 32-bit data is not possible. Hence added scale range for
 * supporting 24-bit or lesser resolution part.
 **/
#define PL_GUI_DATA_MAX_RANGE	16777215
#define PL_GUI_DATA_MIN_RANGE	-16777215

/* lvgl can support only upto 4M (4 million) pixels scale range for
 * LCD displays, so actual data needs to be rescaled in this
 * range for displaying correctly.
 **/
#define PL_GUI_CHART_MAX_PXL_RANGE	2000000
#define PL_GUI_CHART_MIN_PXL_RANGE	-2000000

/* DMM update counter. Update time = count value * lvgl tick time (msec) */
#define PL_GUI_DMM_READ_CNT		10

/******************************************************************************/
/******************** Variables and User Defined Data Types *******************/
/******************************************************************************/

/* Hexadecimal button matrix */
static const char *pl_gui_btnm_hex_map[] = {
	"1", "2", "3", "A", "B", "\n",
	"4", "5", "6", "C", "D", "\n",
	"7", "8", "9", "E", "F", "\n",
	LV_SYMBOL_BACKSPACE,
	"0", ".",
	LV_SYMBOL_NEW_LINE,
	""
};

/* Data capture channel series colours (unique for each channel,
 * max 16 channels/colours) */
static uint16_t pl_gui_capture_chn_ser_col[] = {
	LV_PALETTE_RED,
	LV_PALETTE_PURPLE,
	LV_PALETTE_PINK,
	LV_PALETTE_DEEP_PURPLE,
	LV_PALETTE_INDIGO,
	LV_PALETTE_BLUE,
	LV_PALETTE_LIGHT_BLUE,
	LV_PALETTE_CYAN,
	LV_PALETTE_TEAL,
	LV_PALETTE_GREEN,
	LV_PALETTE_LIGHT_GREEN,
	LV_PALETTE_LIME,
	LV_PALETTE_YELLOW,
	LV_PALETTE_AMBER,
	LV_PALETTE_ORANGE,
	LV_PALETTE_DEEP_ORANGE
};

/* Device select dropdown object */
static lv_obj_t *pl_gui_dd_device_select;
/* Channel select dropdown object */
static lv_obj_t *pl_gui_dd_chan_select;
/* Attributes select dropdown object */
static lv_obj_t *pl_gui_dd_attr_select;
/* Available attributes select dropdown object */
static lv_obj_t *pl_gui_dd_avail_attr_select;
/* Attribute r/w text area object */
static lv_obj_t *pl_gui_ta_attr_rw_value;
/* Button matrix keyboard object */
static lv_obj_t *pl_gui_kb_btnmap;
/* Text area common object */
static lv_obj_t *pl_gui_ta_views;

/* Register address text area object */
static lv_obj_t *pl_gui_ta_reg_address;
/* Register write value text area object */
static lv_obj_t *pl_gui_ta_reg_write_value;
/* Register read value text area object */
static lv_obj_t *pl_gui_ta_reg_read_value;

/* DMM start/stop button object */
static lv_obj_t *pl_gui_dmm_btn_start;
/* DMM enable all button object */
static lv_obj_t *pl_gui_dmm_btn_enable_all;
/* DMM disable all button object */
static lv_obj_t *pl_gui_dmm_btn_disable_all;
/* DMM channels checkbox object */
static lv_obj_t **pl_gui_dmm_chn_checkbox;
/* DMM channels text area object */
static lv_obj_t **pl_gui_dmm_chn_ta;
/* Channels count from DMM view */
static uint32_t pl_gui_dmm_chn_cnt;
/* DMM run status */
static bool pl_gui_dmm_is_running;

/* Capture view channels checkbox object */
static lv_obj_t **pl_gui_capture_chn_checkbox;
/* Capture view chart objects */
static lv_obj_t *pl_gui_capture_chart_ovrly, *pl_gui_capture_chart;
/* Capture channels overlay chart series */
static lv_chart_series_t **pl_gui_capture_chn_ser;
/* Channels count from data capture view */
static uint32_t pl_gui_capture_chn_cnt;
/* Capture run status */
bool pl_gui_capture_is_running;

/* FFT processing parameters */
struct adi_fft_processing pl_gui_fft_proc;
/* FFT measurement parameters */
struct adi_fft_measurements pl_gui_fft_meas;
/* FFT channels select dropdown object */
static lv_obj_t *pl_gui_fft_chn_select;
/* FFT channels series for FFT view chart */
static lv_chart_series_t *pl_gui_fft_chn_ser;
/* FFT view chart objects */
static lv_obj_t *pl_gui_fft_chart;
/* Channels count from FFT view */
static uint32_t pl_gui_fft_chn_cnt;
static lv_obj_t *thd_label;
static lv_obj_t *snr_label;
static lv_obj_t *dr_label;
static lv_obj_t *fund_power_label;
static lv_obj_t *fund_freq_label;
static lv_obj_t *rms_noise_label;
/* FFT run status */
bool pl_gui_fft_is_running;

/* Offset value for data representation */
static int32_t **pl_gui_capture_offset;

/* Current selected device dropdown count/index */
static uint32_t pl_gui_device_indx;

/* Channel scan information */
static struct scan_type **pl_gui_capture_chn_info;

/* Number of data samples to be captured for fft analysis */
static uint32_t fft_data_samples;
static uint32_t fft_bins;

/* Local copies of function callbacks to convert input data to voltage */
static adi_fft_data_to_volt_conv data_to_volt_without_vref;
static adi_fft_data_to_volt_conv data_to_volt_wrt_vref;

/* Local copy of function callback to convert input code to straight
 * binary data */
static adi_fft_code_to_straight_bin_conv code_to_straight_binary;

/******************************************************************************/
/************************ Functions Prototypes ********************************/
/******************************************************************************/

/******************************************************************************/
/************************ Functions Definitions *******************************/
/******************************************************************************/

/**
 * @brief 	Replace character in string
 * @param	str[in] - Input string
 * @param	find[in] - Character to be found
 * @param	replace[in] - Character to be replaced
 * @return	None
 */
static void pl_replace_char(char *str, char find, char replace)
{
	char *current_pos = strchr(str, find);
	while (current_pos) {
		*current_pos = replace;
		current_pos = strchr(current_pos, find);
	}
}

/**
 * @brief 	Read the active device index for current view
 * @return	active device index
 */
uint32_t pl_gui_get_active_device_index(void)
{
	return pl_gui_device_indx;
}

/**
 * @brief 	Read attribute and display it's value
 * @return	None
 */
static void pl_gui_read_and_display_attr(void)
{
	int32_t ret;
	char ibuf[100], obuf[100];
	uint32_t chn_pos;

	/* Read channel type */
	ibuf[0] = '\0';
	obuf[0] = '\0';
	lv_dropdown_get_selected_str(pl_gui_dd_chan_select, ibuf, sizeof(ibuf));
	if (ibuf[0] == '\0') {
		return;
	}

	/* Check attribute type (global/channel) */
	if (!strcmp(ibuf, "global")) {
		ibuf[0] = '\0';
		obuf[0] = '\0';

		lv_dropdown_get_selected_str(pl_gui_dd_attr_select, ibuf, sizeof(ibuf));
		if (ibuf[0] == '\0') {
			return;
		}

		/* Read available attribute options */
		ret = pl_gui_get_global_attr_avail_options(ibuf, obuf, pl_gui_device_indx);
		if (!ret) {
			pl_replace_char(obuf, ' ', '\n');
			strcat(obuf, "\n\0");
			lv_dropdown_set_options(pl_gui_dd_avail_attr_select, obuf);
		} else {
			lv_dropdown_clear_options(pl_gui_dd_avail_attr_select);
		}

		/* Read global attribute value */
		ret = pl_gui_read_global_attr(ibuf, obuf, pl_gui_device_indx);
		if (ret) {
			return;
		}
	} else {
		ibuf[0] = '\0';
		obuf[0] = '\0';

		lv_dropdown_get_selected_str(pl_gui_dd_attr_select, ibuf, sizeof(ibuf));
		if (ibuf[0] == '\0') {
			return;
		}

		/* Read available attribute options */
		chn_pos = lv_dropdown_get_selected(pl_gui_dd_chan_select);
		chn_pos -= 1;     // First position is for global attr
		ret = pl_gui_get_chn_attr_avail_options(ibuf, obuf, chn_pos,
							pl_gui_device_indx);
		if (!ret) {
			pl_replace_char(obuf, ' ', '\n');
			strcat(obuf, "\n\0");
			lv_dropdown_set_options(pl_gui_dd_avail_attr_select, obuf);
		} else {
			lv_dropdown_clear_options(pl_gui_dd_avail_attr_select);
		}

		/* Read channel attribute value */
		ret = pl_gui_read_chn_attr(ibuf, obuf, chn_pos, pl_gui_device_indx);
		if (ret) {
			return;
		}
	}

	/* Display attribute value into text field */
	lv_textarea_set_text(pl_gui_ta_attr_rw_value, obuf);
}

/**
 * @brief 	Write and readback attribute value
 * @return	None
 */
static void pl_gui_update_and_readback_attr(void)
{
	int32_t ret;
	char ibuf[100];
	uint32_t chn_pos;
	const char *text;

	/* Read channel type */
	ibuf[0] = '\0';
	lv_dropdown_get_selected_str(pl_gui_dd_chan_select, ibuf, sizeof(ibuf));
	if (ibuf[0] == '\0') {
		return;
	}

	/* Check attribute type (global/channel) */
	if (!strcmp(ibuf, "global")) {
		/* Write global attribute */
		text = lv_textarea_get_text(pl_gui_ta_attr_rw_value);
		ibuf[0] = '\0';
		lv_dropdown_get_selected_str(pl_gui_dd_attr_select, ibuf, sizeof(ibuf));
		if (ibuf[0] == '\0') {
			return;
		}

		ret = pl_gui_write_global_attr(ibuf, (char *)text,
					       pl_gui_device_indx);
		if (ret) {
			return;
		}
	} else {
		/* Write channel attribute */
		chn_pos = lv_dropdown_get_selected(pl_gui_dd_chan_select);
		chn_pos -= 1;      // First position is for global attr
		text = lv_textarea_get_text(pl_gui_ta_attr_rw_value);
		ibuf[0] = '\0';
		lv_dropdown_get_selected_str(pl_gui_dd_attr_select, ibuf, sizeof(ibuf));
		if (ibuf[0] == '\0') {
			return;
		}

		ret = pl_gui_write_chn_attr(ibuf, (char *)text, chn_pos,
					    pl_gui_device_indx);
		if (ret) {
			return;
		}
	}

	/* Perform attribute readback */
	pl_gui_read_and_display_attr();
}

/**
 * @brief 	Read and display the register value
 * @param	reg_addr[in] - Register address in hex
 * @return	None
 */
static void pl_gui_read_and_display_reg_val(uint32_t reg_addr)
{
	uint32_t reg_data = 0;
	char ibuf[100];

	/* Save register address value into it's text area */
	ibuf[0] = '\0';
	sprintf(ibuf, "%X", reg_addr);
	lv_textarea_set_text(pl_gui_ta_reg_address, ibuf);

	/* Read register value and display into value read text area */
	ibuf[0] = '\0';
	pl_gui_read_reg(reg_addr, &reg_data, pl_gui_device_indx);
	sprintf(ibuf, "%X", reg_data);
	lv_textarea_set_text(pl_gui_ta_reg_read_value, ibuf);
}

/**
 * @brief 	Write and readback the register value
 * @param	reg_addr[in] - Register address in hex
 * @param	reg_data[in] - Register data in hex
 * @return	None
 */
static void pl_gui_write_and_readback_reg_val(uint32_t reg_addr,
		uint32_t reg_data)
{
	pl_gui_write_reg(reg_addr, reg_data, pl_gui_device_indx);
	pl_gui_read_and_display_reg_val(reg_addr);
}

/**
 * @brief 	Perform the DMM read operations
 * @return	None
 * @note	DMM results are manipulated based on the raw, offset
 *			and scale attribute values read from the device
 */
void pl_gui_perform_dmm_read(void)
{
	int32_t ret;
	static uint32_t read_cntr;
	uint32_t cnt;
	char ibuf[100];

	read_cntr++;
	if (read_cntr > PL_GUI_DMM_READ_CNT) {
		read_cntr = 0;
		for (cnt = 0; cnt < pl_gui_dmm_chn_cnt; cnt++) {
			ibuf[0] = '\0';
			if (lv_obj_get_state(pl_gui_dmm_chn_checkbox[cnt]) == LV_STATE_CHECKED) {
				/* get DMM reading for current channel and display it into text area */
				ret = pl_gui_get_dmm_reading(ibuf, cnt, pl_gui_device_indx);
				if (ret) {
					return;
				}
				lv_textarea_set_text(pl_gui_dmm_chn_ta[cnt], ibuf);
			}
		}
	}
}

/**
 * @brief 	Rescale the data to fit within display chart pixel range
 * @param	data[in,out] - Rescaled data
 * @return	None
 */
static void pl_gui_rescale_data(int32_t *data)
{
	float scaler;

	scaler = ((float)*data - (PL_GUI_DATA_MIN_RANGE)) / ((
				PL_GUI_DATA_MAX_RANGE -
				(PL_GUI_DATA_MIN_RANGE)));
	*data = ((PL_GUI_CHART_MAX_PXL_RANGE - (PL_GUI_CHART_MIN_PXL_RANGE)) * scaler) +
		(PL_GUI_CHART_MIN_PXL_RANGE);
}

/**
 * @brief 	Display the captured data onto GUI
 * @param	buf[in] - Data buffer
 * @param	rec_bytes[in] - Number of received bytes
 * @return	None
 */
void pl_gui_display_captured_data(uint8_t *buf, uint32_t rec_bytes)
{
	char obuf[100];
	uint32_t chn;
	uint32_t indx = 0;
	uint32_t storage_bytes;
	uint32_t code;
	int32_t data;
	static uint32_t cnt = 0;

	/* Copy data into local buffer */
	while (indx < rec_bytes) {
		if (pl_gui_capture_is_running) {
			for (chn = 0; chn < pl_gui_capture_chn_cnt; chn++) {
				if (lv_obj_get_state(pl_gui_capture_chn_checkbox[chn]) == LV_STATE_CHECKED) {
					storage_bytes = pl_gui_capture_chn_info[chn]->storagebits >> 3;
					memcpy((void *)&code, (void *)&buf[indx], storage_bytes);

					/* Convert code to straight binary */
					data = pl_gui_cnv_code_to_straight_binary(code, chn);

					pl_gui_rescale_data(&data);

					lv_chart_set_next_value(pl_gui_capture_chart_ovrly, pl_gui_capture_chn_ser[chn],
								data);
				}
			}
		} else if (pl_gui_fft_is_running) {
			chn = lv_dropdown_get_selected(pl_gui_fft_chn_select);
			storage_bytes = pl_gui_capture_chn_info[chn]->storagebits >> 3;
			memcpy((void *)&code, (void *)&buf[indx], storage_bytes);

			/* Convert code to straight binary */
			pl_gui_fft_proc.input_data[cnt++] = pl_gui_cnv_code_to_straight_binary(code,
							    chn);

			if (cnt >= fft_data_samples) {
				break;
			}
		} else {
			return;
		}

		indx += storage_bytes;

		if (cnt >= fft_data_samples) {
			break;
		}
	}

	if (pl_gui_fft_is_running && cnt >= fft_data_samples) {
		/* Perform FFT measurements */
		adi_fft_perform(&pl_gui_fft_proc, &pl_gui_fft_meas);

		/* Display FFT results */
		for (cnt = 0; cnt < fft_bins; cnt++) {
			lv_chart_set_next_value(pl_gui_fft_chart,
						pl_gui_fft_chn_ser,
						pl_gui_fft_proc.fft_dB[cnt]);
		}

		obuf[0] = '\0';
		sprintf(obuf, "%.3f dB", pl_gui_fft_meas.THD);
		lv_label_set_text(thd_label, obuf);

		obuf[0] = '\0';
		sprintf(obuf, "%.3f dB", pl_gui_fft_meas.SNR);
		lv_label_set_text(snr_label, obuf);

		obuf[0] = '\0';
		sprintf(obuf, "%.3f dB", pl_gui_fft_meas.DR);
		lv_label_set_text(dr_label, obuf);

		obuf[0] = '\0';
		sprintf(obuf, "%.3f dBFS", pl_gui_fft_meas.harmonics_mag_dbfs[0]);
		lv_label_set_text(fund_power_label, obuf);

		obuf[0] = '\0';
		sprintf(obuf, "%.3f Hz",
			pl_gui_fft_meas.harmonics_freq[0]*pl_gui_fft_proc.bin_width);
		lv_label_set_text(fund_freq_label, obuf);

		obuf[0] = '\0';
		sprintf(obuf, "%.3f uV", pl_gui_fft_meas.RMS_noise * 1000000.0);
		lv_label_set_text(rms_noise_label, obuf);

		cnt = 0;
	}
}

/**
 * @brief 	Get the count for data samples to be captured
 * @return	data samples count
 */
uint32_t get_data_samples_count(void)
{
	if (pl_gui_fft_is_running) {
		return fft_data_samples;
	} else {
		return PL_GUI_REQ_DATA_SAMPLES;
	}
}

/**
 * @brief 	Store the channel info
 * @param	ch_info[in] - Channel information
 * @param	chn_indx[in] - Channel index
 * @return	None
 */
void pl_gui_store_chn_info(struct scan_type *ch_info, uint32_t chn_indx)
{
	memcpy(pl_gui_capture_chn_info[chn_indx], ch_info, sizeof(*ch_info));
}

/**
 * @brief 	Get the channels mask based on enabled channels count
 * @param	chn_mask[in,out] - Channel mask
 * @return	None
 */
void pl_gui_get_capture_chns_mask(uint32_t *chn_mask)
{
	uint32_t cnt;
	uint32_t mask = 0x1;
	uint32_t chn_pos;

	if (!chn_mask) {
		return;
	}

	*chn_mask = 0;

	if (pl_gui_capture_is_running) {
		for (cnt = 0; cnt < pl_gui_capture_chn_cnt; cnt++) {
			if (lv_obj_get_state(pl_gui_capture_chn_checkbox[cnt]) == LV_STATE_CHECKED) {
				*chn_mask |= mask;
			}
			mask <<= 1;
		}
	} else if (pl_gui_fft_is_running) {
		chn_pos = lv_dropdown_get_selected(pl_gui_fft_chn_select);
		mask <<= chn_pos;
		*chn_mask = mask;
	} else {
		return;
	}
}

/**
 * @brief 	DMM running status check
 * @return	DMM running status
 */
bool pl_gui_is_dmm_running(void)
{
	return pl_gui_dmm_is_running;
}

/**
 * @brief 	Capture running status check
 * @return	Capture running status
 */
bool pl_gui_is_capture_running(void)
{
	return pl_gui_capture_is_running;
}

/**
 * @brief 	FFT running status check
 * @return	FFT running status
 */
bool pl_gui_is_fft_running(void)
{
	return pl_gui_fft_is_running;
}

/**
 * @brief 	Handle button matrix keyboard events
 * @param	event[in] - Button matrix event
 * @return	None
 */
static void pl_gui_btnmp_event_cb(lv_event_t *event)
{
	lv_obj_t *obj = lv_event_get_target(event);
	const char *txt = lv_btnmatrix_get_button_text(obj,
			  lv_btnmatrix_get_selected_btn(obj));

	if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
		lv_textarea_del_char(pl_gui_ta_views);
	} else if (strcmp(txt, LV_SYMBOL_NEW_LINE) == 0) {
		lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
		lv_event_send(pl_gui_ta_views, LV_EVENT_READY, NULL);
	} else {
		lv_textarea_add_text(pl_gui_ta_views, txt);
	}
}

/**
 * @brief 	Add button matrix keyboard
 * @return	None
 */
static void pl_gui_add_btnmap_kb(void)
{
	/* Setup button matrix for hex values */
	pl_gui_kb_btnmap = lv_btnmatrix_create(lv_scr_act());
	lv_obj_set_size(pl_gui_kb_btnmap, 500, 200);
	lv_obj_align(pl_gui_kb_btnmap, LV_ALIGN_BOTTOM_MID, 0, -50);
	lv_obj_clear_flag(pl_gui_kb_btnmap, LV_OBJ_FLAG_CLICK_FOCUSABLE);
	lv_btnmatrix_set_map(pl_gui_kb_btnmap, pl_gui_btnm_hex_map);
	lv_obj_add_flag(pl_gui_kb_btnmap, LV_OBJ_FLAG_HIDDEN);

	/* Add button matrix event */
	lv_obj_add_event(pl_gui_kb_btnmap, pl_gui_btnmp_event_cb,
			 LV_EVENT_VALUE_CHANGED,
			 NULL);
}

/**
 * @brief 	Manage the button matrix keyboard visibility
 * @param	event[in] - Button matrix event
 * @return	None
 */
static void pl_gui_manage_btnmap_kb(lv_event_t *event)
{
	lv_event_code_t evt = lv_event_get_code(event);

	switch (evt) {
	case LV_EVENT_FOCUSED:
		lv_obj_clear_flag(pl_gui_kb_btnmap, LV_OBJ_FLAG_HIDDEN);
		break;

	case LV_EVENT_DEFOCUSED:
		lv_obj_add_flag(pl_gui_kb_btnmap, LV_OBJ_FLAG_HIDDEN);
		break;

	default:
		LV_LOG_USER("unexpected options %d", evt);
	}
}

/**
 * @brief 	Handle device list dropdown select events for current view
 * @param	event[in] - device list dropdown event
 * @return	None
 */
static void pl_gui_device_select_event_cb(lv_event_t *event)
{
	lv_event_code_t code = lv_event_get_code(event);
	lv_obj_t *obj = lv_event_get_target(event);

	if (code == LV_EVENT_VALUE_CHANGED) {
		/* Store the current device index */
		pl_gui_device_indx = lv_dropdown_get_selected(obj);
	}
}

/**
 * @brief 	Handle channel list dropdown select events for current view
 * @param	event[in] - channel list dropdown event
 * @return	None
 */
static void pl_gui_chn_select_event_cb(lv_event_t *event)
{
	lv_event_code_t code = lv_event_get_code(event);
	lv_obj_t *obj = lv_event_get_target(event);
	char ibuf[100], obuf[100];
	uint32_t chn_pos;

	if (code == LV_EVENT_VALUE_CHANGED) {
		ibuf[0] = '\0';
		obuf[0] = '\0';
		lv_dropdown_get_selected_str(obj, ibuf, sizeof(ibuf));
		if (ibuf[0] == '\0') {
			return;
		}

		if (!strcmp(ibuf, "global")) {
			/* Read global attribute names and store into attribute
			 * selection dropdown */
			pl_gui_get_global_attr_names(obuf, pl_gui_device_indx);
		} else {
			/* Read channel attribute names and store into attribute
			 * selection dropdown*/
			chn_pos = lv_dropdown_get_selected(pl_gui_dd_chan_select);
			chn_pos -= 1;  // First position is for global attr
			pl_gui_get_chn_attr_names(obuf, chn_pos, pl_gui_device_indx);
		}

		lv_dropdown_set_options(pl_gui_dd_attr_select, obuf);
		pl_gui_read_and_display_attr();
	}
}

/**
 * @brief 	Handle attribute list dropdown select events
 * @param	event[in] - attribute list dropdown event
 * @return	None
 */
static void pl_gui_attr_select_event_cb(lv_event_t *event)
{
	lv_event_code_t code = lv_event_get_code(event);

	if (code == LV_EVENT_VALUE_CHANGED) {
		pl_gui_read_and_display_attr();
	}
}

/**
 * @brief 	Handle attribute read button events
 * @param	event[in] - attribute read button event
 * @return	None
 */
static void pl_gui_attr_read_btn_event_cb(lv_event_t *event)
{
	lv_event_code_t code = lv_event_get_code(event);

	if (code == LV_EVENT_CLICKED) {
		pl_gui_read_and_display_attr();
	}
}

/**
 * @brief 	Handle attribute write button events
 * @param	event[in] - attribute write button event
 * @return	None
 */
static void pl_gui_attr_write_btn_event_cb(lv_event_t *event)
{
	lv_event_code_t code = lv_event_get_code(event);

	if (code == LV_EVENT_CLICKED) {
		/* Update attribute and readback it's value */
		pl_gui_update_and_readback_attr();
	}
}

/**
 * @brief 	Handle attribute available list dropdown select events
 * @param	event[in] - attribute available list dropdown event
 * @return	None
 */
static void pl_gui_attr_avl_select_event_cb(lv_event_t *event)
{
	char ibuf[100];
	lv_event_code_t code = lv_event_get_code(event);
	lv_obj_t *obj = lv_event_get_target(event);

	if (code == LV_EVENT_VALUE_CHANGED) {
		/* Read available attribute option select string */
		ibuf[0] = '\0';
		lv_dropdown_get_selected_str(obj, ibuf, sizeof(ibuf));
		if (ibuf[0] == '\0') {
			return;
		}

		/* Display select option value into text field */
		lv_textarea_set_text(pl_gui_ta_attr_rw_value, ibuf);

		/* Write to attribute and readback it's value */
		pl_gui_update_and_readback_attr();
	}
}

/**
 * @brief 	Handle register view button events
 * @param	event[in] - register view button event
 * @return	None
 */
static void pl_gui_reg_btn_event_cb(lv_event_t *event)
{
	lv_event_code_t code = lv_event_get_code(event);
	lv_obj_t *btn = lv_event_get_target(event);
	static uint32_t reg_addr = 0;
	uint32_t reg_data;
	const char *addr, *data;

	if (code == LV_EVENT_CLICKED) {
		lv_obj_t *label = lv_obj_get_child(btn, 0);
		char *text = lv_label_get_text(label);

		if (!strcmp(text, "+")) {
			reg_addr += 1;
			if (reg_addr > 0xffff) {
				reg_addr = 0;
			}
			pl_gui_read_and_display_reg_val(reg_addr);
		} else if (!strcmp(text, "-")) {
			if (reg_addr > 0) {
				reg_addr -= 1;
			}
			pl_gui_read_and_display_reg_val(reg_addr);
		} else if (!strcmp(text, "Read")) {
			addr = lv_textarea_get_text(pl_gui_ta_reg_address);
			sscanf(addr, "%X", &reg_addr);
			pl_gui_read_and_display_reg_val(reg_addr);
		} else if (!strcmp(text, "Write")) {
			addr = lv_textarea_get_text(pl_gui_ta_reg_address);
			sscanf(addr, "%X", &reg_addr);
			data = lv_textarea_get_text(pl_gui_ta_reg_write_value);
			sscanf(data, "%X", &reg_data);
			pl_gui_write_and_readback_reg_val(reg_addr, reg_data);
		}
	}
}

/**
 * @brief 	Handle text area select events for current view
 * @param	event[in] - text area select event
 * @return	None
 */
static void pl_gui_ta_event_handler(lv_event_t *event)
{
	lv_event_code_t code = lv_event_get_code(event);
	lv_obj_t *ta = lv_event_get_target(event);

	if (code == LV_EVENT_CLICKED) {
		/* Store the selected text area object */
		pl_gui_ta_views = ta;
	}
}

/**
 * @brief 	Handle dmm view buttons events
 * @param	event[in] - dmm view buttons event
 * @return	None
 */
static void pl_gui_dmm_btn_event_cb(lv_event_t *event)
{
	char *text;
	uint32_t cnt;
	lv_event_code_t code = lv_event_get_code(event);
	lv_obj_t *btn = lv_event_get_target(event);

	if (code == LV_EVENT_CLICKED) {
		/*Get the first child of the button which is the label and change its text*/
		lv_obj_t *label = lv_obj_get_child(btn, 0);
		text = lv_label_get_text(label);

		if (!strcmp(text, "Enable All")) {
			if (!pl_gui_dmm_is_running) {
				for (cnt = 0; cnt < pl_gui_dmm_chn_cnt; cnt++) {
					lv_obj_add_state(pl_gui_dmm_chn_checkbox[cnt], LV_STATE_CHECKED);
				}
			}
		} else if (!strcmp(text, "Disable All")) {
			if (!pl_gui_dmm_is_running) {
				for (cnt = 0; cnt < pl_gui_dmm_chn_cnt; cnt++) {
					lv_obj_clear_state(pl_gui_dmm_chn_checkbox[cnt], LV_STATE_CHECKED);
				}
			}
		} else { // Start/Stop label
			if (pl_gui_dmm_is_running) {
				lv_label_set_text(label, "Start");
				lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
			} else {
				lv_label_set_text(label, "Stop");
				lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
			}
			pl_gui_dmm_is_running = !pl_gui_dmm_is_running;
		}
	}
}

/**
 * @brief 	Handle capture view buttons events
 * @param	event[in] - capture view buttons event
 * @return	None
 */
static void pl_gui_capture_btn_event_cb(lv_event_t *event)
{
	char *text;
	uint32_t cnt;
	lv_event_code_t code = lv_event_get_code(event);
	lv_obj_t *btn = lv_event_get_target(event);

	if (code == LV_EVENT_CLICKED) {
		lv_obj_t *label = lv_obj_get_child(btn, 0);
		text = lv_label_get_text(label);

		if (!strcmp(text, "Enable All")) {
			if (!pl_gui_dmm_is_running) {
				for (cnt = 0; cnt < pl_gui_capture_chn_cnt; cnt++) {
					lv_obj_add_state(pl_gui_capture_chn_checkbox[cnt], LV_STATE_CHECKED);
				}
			}
		} else if (!strcmp(text, "Disable All")) {
			if (!pl_gui_dmm_is_running) {
				for (cnt = 0; cnt < pl_gui_capture_chn_cnt; cnt++) {
					lv_obj_clear_state(pl_gui_capture_chn_checkbox[cnt], LV_STATE_CHECKED);
				}
			}
		} else { // Start/Stop label
			if (pl_gui_capture_is_running) {
				lv_label_set_text(label, "Start");
				lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);

				/* Remove previously enabled channels from pl_gui_capture_chart_ovrly series */
				for (cnt = 0; cnt < pl_gui_capture_chn_cnt; cnt++) {
					if (lv_obj_get_state(pl_gui_capture_chn_checkbox[cnt]) == LV_STATE_CHECKED) {
						lv_chart_remove_series(pl_gui_capture_chart_ovrly, pl_gui_capture_chn_ser[cnt]);
						pl_gui_capture_chn_ser[cnt] = NULL;
					}
				}
			} else {
				lv_label_set_text(label, "Stop");
				lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);

				/* Add enabled channels to pl_gui_capture_chart_ovrly series */
				for (cnt = 0; cnt < pl_gui_capture_chn_cnt; cnt++) {
					if (lv_obj_get_state(pl_gui_capture_chn_checkbox[cnt]) == LV_STATE_CHECKED) {
						pl_gui_capture_chn_ser[cnt] = lv_chart_add_series(pl_gui_capture_chart_ovrly,
									      lv_palette_main(pl_gui_capture_chn_ser_col[cnt]),
									      LV_CHART_AXIS_PRIMARY_Y);
						lv_chart_set_point_count(pl_gui_capture_chart_ovrly, 400);
					}
				}
			}
			pl_gui_capture_is_running = !pl_gui_capture_is_running;
		}
	}
}

/**
 * @brief 	Handle FFT view buttons events
 * @param	event[in] - FFT view buttons event
 * @return	None
 */
static void pl_gui_fft_btn_event_cb(lv_event_t *event)
{
	char *text;
	lv_event_code_t code = lv_event_get_code(event);
	lv_obj_t *btn = lv_event_get_target(event);

	if (code == LV_EVENT_CLICKED) {
		lv_obj_t *label = lv_obj_get_child(btn, 0);
		text = lv_label_get_text(label);

		if (pl_gui_fft_is_running) {
			lv_label_set_text(label, "Start");
			lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);

			/* Remove previously enabled channel from series */
			lv_chart_remove_series(pl_gui_fft_chart, pl_gui_fft_chn_ser);
			pl_gui_fft_chn_ser = NULL;
		} else {
			lv_label_set_text(label, "Stop");
			lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);

			/* Add enabled channels to series */
			pl_gui_fft_chn_ser = lv_chart_add_series(pl_gui_fft_chart,
					     lv_palette_main(LV_PALETTE_RED),
					     LV_CHART_AXIS_PRIMARY_Y);
			lv_chart_set_point_count(pl_gui_fft_chart, fft_bins);
		}
		pl_gui_fft_is_running = !pl_gui_fft_is_running;
	}
}

/**
 * @brief 	Convert input data to voltage without Vref
 * @param	data[in] - Input data (signed, straight binary)
 * @param	chn[in] - Input channel
 * @return	voltage
 */
float pl_gui_cnv_data_to_volt_without_vref(int32_t data, uint8_t chn)
{
	if (data_to_volt_without_vref != NULL) {
		return data_to_volt_without_vref(data, chn);
	} else {
		return 0;
	}
}

/**
 * @brief 	Convert input data code to voltage w.r.t Vref
 * @param	data[in] - Input data (signed, straight binary)
 * @param	chn[in] - Input channel
 * @return	voltage
 */
float pl_gui_cnv_data_to_volt_wrt_vref(int32_t data, uint8_t chn)
{
	if (data_to_volt_wrt_vref != NULL) {
		return data_to_volt_wrt_vref(data, chn);
	} else {
		return 0;
	}
}

/**
 * @brief 	Convert input code to straight binary data
 * @param	code[in] - Input code (unsigned)
 * @param	chn[in] - Input channel
 * @return	straight binary data
 */
int32_t pl_gui_cnv_code_to_straight_binary(uint32_t code, uint8_t chn)
{
	if (code_to_straight_binary != NULL) {
		return code_to_straight_binary(code, chn);
	} else {
		return (code + *pl_gui_capture_offset[chn]);
	}
}

/**
 * @brief 	Create pocket lab GUI attributes view
 * @param	parent[in] - pointer to attributes view instance
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_create_attributes_view(lv_obj_t *parent,
				      struct pl_gui_init_param *param)
{
	int32_t ret;
	char dropdown_list[100];
	lv_obj_t *wr_btn;
	lv_obj_t *rd_btn;
	uint32_t nb_of_chn;

	/* Create view */
	lv_obj_t *label = lv_label_create(parent);

	/*** Device names dropdown ***/

	/* Get device names */
	dropdown_list[0] = '\0';
	ret = pl_gui_get_dev_names(dropdown_list);
	if (ret) {
		return ret;
	}

	/* Create device select dropdown and insert device names list */
	lv_label_set_text(label, "Device");
	lv_obj_align(label, LV_ALIGN_TOP_LEFT, 5, 20);
	pl_gui_dd_device_select = lv_dropdown_create(parent);
	lv_obj_align_to(pl_gui_dd_device_select, label, LV_ALIGN_OUT_BOTTOM_LEFT, 0,
			10);
	lv_dropdown_set_options(pl_gui_dd_device_select, dropdown_list);
	lv_obj_add_event(pl_gui_dd_device_select, pl_gui_device_select_event_cb,
			 LV_EVENT_ALL,
			 NULL);

	/*** Global and channels attributes dropdown ***/

	/* Insert 'global' attribute label as first dropdown option */
	dropdown_list[0] = '\0';
	strcat(dropdown_list, "global\n");

	/* Get list of all channels for selected device */
	ret = pl_gui_get_chn_names(dropdown_list, &nb_of_chn, pl_gui_device_indx);
	if (ret) {
		return ret;
	}

	/* Create channel select dropdown and insert channel names */
	pl_gui_dd_chan_select = lv_dropdown_create(parent);
	lv_obj_align_to(pl_gui_dd_chan_select, pl_gui_dd_device_select,
			LV_ALIGN_OUT_RIGHT_MID,
			20,
			0);
	lv_dropdown_set_options(pl_gui_dd_chan_select, dropdown_list);
	lv_obj_add_event(pl_gui_dd_chan_select, pl_gui_chn_select_event_cb,
			 LV_EVENT_ALL,
			 NULL);

	label = lv_label_create(parent);
	lv_label_set_text(label, "Channel");
	lv_obj_align_to(label, pl_gui_dd_chan_select, LV_ALIGN_OUT_TOP_LEFT, 0, -5);

	/* Get global attributes names (global attr is default) */
	dropdown_list[0] = '\0';
	ret = pl_gui_get_global_attr_names(dropdown_list, pl_gui_device_indx);
	if (ret) {
		return ret;
	}

	/* Create attribute select dropdown and insert attribute names */
	pl_gui_dd_attr_select = lv_dropdown_create(parent);
	lv_obj_align_to(pl_gui_dd_attr_select, pl_gui_dd_chan_select,
			LV_ALIGN_OUT_RIGHT_TOP,
			20, 0);
	lv_dropdown_set_options(pl_gui_dd_attr_select, dropdown_list);
	lv_obj_add_event(pl_gui_dd_attr_select, pl_gui_attr_select_event_cb,
			 LV_EVENT_ALL,
			 NULL);
	lv_obj_set_width(pl_gui_dd_attr_select, 300);

	label = lv_label_create(parent);
	lv_label_set_text(label, "Attributes");
	lv_obj_align_to(label, pl_gui_dd_attr_select, LV_ALIGN_OUT_TOP_LEFT, 0, -5);

	/* Create Read/Write attribute value widgets */
	pl_gui_ta_attr_rw_value = lv_textarea_create(parent);
	lv_textarea_set_one_line(pl_gui_ta_attr_rw_value, true);
	lv_textarea_set_text(pl_gui_ta_attr_rw_value, "0");
	lv_textarea_set_max_length(pl_gui_ta_attr_rw_value, 30);
	lv_obj_set_width(pl_gui_ta_attr_rw_value, 300);
	lv_obj_align_to(pl_gui_ta_attr_rw_value, pl_gui_dd_attr_select,
			LV_ALIGN_OUT_BOTTOM_LEFT, 0,
			20);
	lv_obj_add_event(pl_gui_ta_attr_rw_value, pl_gui_ta_event_handler,
			 LV_EVENT_CLICKED,
			 NULL);

	label = lv_label_create(parent);
	lv_label_set_text(label, "Read/Write Value ");
	lv_obj_align_to(label, pl_gui_ta_attr_rw_value, LV_ALIGN_OUT_LEFT_MID, -15, 0);

	rd_btn = lv_btn_create(parent);
	lv_obj_set_size(rd_btn, 100, 50);
	lv_obj_align_to(rd_btn, pl_gui_ta_attr_rw_value, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
	label = lv_label_create(rd_btn);
	lv_label_set_text(label, "Read");
	lv_obj_center(label);
	lv_obj_add_event(rd_btn, pl_gui_attr_read_btn_event_cb, LV_EVENT_ALL, NULL);
	lv_obj_set_style_bg_color(rd_btn, lv_palette_main(LV_PALETTE_PURPLE),
				  LV_PART_MAIN);

	wr_btn = lv_btn_create(parent);
	lv_obj_set_size(wr_btn, 100, 50);
	lv_obj_align_to(wr_btn, pl_gui_ta_attr_rw_value, LV_ALIGN_OUT_RIGHT_MID, 10,
			60);
	label = lv_label_create(wr_btn);
	lv_label_set_text(label, "Write");
	lv_obj_center(label);
	lv_obj_add_event(wr_btn, pl_gui_attr_write_btn_event_cb, LV_EVENT_ALL, NULL);
	lv_obj_set_style_bg_color(wr_btn,
				  lv_palette_main(LV_PALETTE_GREEN),
				  LV_PART_MAIN);

	/* Attribute value write menu (for available/dropdown type attributes) */
	pl_gui_dd_avail_attr_select = lv_dropdown_create(parent);
	lv_obj_align_to(pl_gui_dd_avail_attr_select,
			pl_gui_ta_attr_rw_value,
			LV_ALIGN_OUT_BOTTOM_LEFT,
			0,
			70);
	lv_dropdown_set_options(pl_gui_dd_avail_attr_select, "\n\n\n\n\n\n\n\n\n\n");
	lv_obj_add_event(pl_gui_dd_avail_attr_select,
			 pl_gui_attr_avl_select_event_cb,
			 LV_EVENT_ALL,
			 NULL);
	lv_obj_set_width(pl_gui_dd_avail_attr_select, 300);

	label = lv_label_create(parent);
	lv_label_set_text(label, "available options");
	lv_obj_align_to(label, pl_gui_dd_avail_attr_select, LV_ALIGN_OUT_RIGHT_MID, 5,
			0);

	/* Add event for keyboard visibility management */
	lv_obj_add_event(pl_gui_ta_attr_rw_value, pl_gui_manage_btnmap_kb,
			 LV_EVENT_FOCUSED,
			 NULL);
	lv_obj_add_event(pl_gui_ta_attr_rw_value, pl_gui_manage_btnmap_kb,
			 LV_EVENT_DEFOCUSED,
			 NULL);

	return 0;
}

/**
 * @brief 	Create pocket lab GUI register view
 * @param	parent[in] - pointer to register view instance
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_create_register_view(lv_obj_t *parent,
				    struct pl_gui_init_param *param)
{
	char dropdown_list[100];
	int32_t ret;

	/* Get device names */
	dropdown_list[0] = '\0';
	ret = pl_gui_get_dev_names(dropdown_list);
	if (ret) {
		return ret;
	}

	/* Device select dropdown options */
	pl_gui_dd_device_select = lv_dropdown_create(parent);
	lv_obj_align(pl_gui_dd_device_select, LV_ALIGN_TOP_MID, 0, 0);
	lv_dropdown_set_options(pl_gui_dd_device_select, dropdown_list);
	lv_obj_add_event(pl_gui_dd_device_select, pl_gui_device_select_event_cb,
			 LV_EVENT_ALL,
			 NULL);

	/* Register Address text area */
	pl_gui_ta_reg_address = lv_textarea_create(parent);
	lv_textarea_set_one_line(pl_gui_ta_reg_address, true);
	lv_textarea_set_text(pl_gui_ta_reg_address, "0");
	lv_textarea_set_accepted_chars(pl_gui_ta_reg_address, "0123456789ABCDEFabcdef");
	lv_textarea_set_max_length(pl_gui_ta_reg_address, 8);
	lv_obj_align(pl_gui_ta_reg_address, LV_ALIGN_TOP_MID, 0, 80);
	lv_obj_add_event(pl_gui_ta_reg_address, pl_gui_ta_event_handler,
			 LV_EVENT_CLICKED,
			 NULL);

	/* Register address label */
	lv_obj_t *label = lv_label_create(parent);
	lv_label_set_text(label, "Register Address (hex)");
	lv_obj_align_to(label, pl_gui_ta_reg_address, LV_ALIGN_OUT_LEFT_MID, -10, 0);

	/* Register address increment/decrement buttons and labels */
	lv_obj_t *btn = lv_btn_create(parent);
	lv_obj_set_size(btn, 90, 50);
	lv_obj_align_to(btn, pl_gui_ta_reg_address, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
	label = lv_label_create(btn);
	lv_label_set_text(label, "-");
	lv_obj_center(label);
	lv_obj_add_event(btn, pl_gui_reg_btn_event_cb, LV_EVENT_ALL, NULL);
	lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_ORANGE),
				  LV_PART_MAIN);

	btn = lv_btn_create(parent);
	lv_obj_set_size(btn, 90, 50);
	lv_obj_align_to(btn, pl_gui_ta_reg_address, LV_ALIGN_OUT_RIGHT_MID, 110, 0);
	label = lv_label_create(btn);
	lv_label_set_text(label, "+");
	lv_obj_center(label);
	lv_obj_add_event(btn, pl_gui_reg_btn_event_cb, LV_EVENT_ALL, NULL);
	lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);

	/* Register read value text area */
	pl_gui_ta_reg_read_value = lv_textarea_create(parent);
	lv_textarea_set_one_line(pl_gui_ta_reg_read_value, true);
	lv_textarea_set_text(pl_gui_ta_reg_read_value, "0");
	lv_obj_clear_flag(pl_gui_ta_reg_read_value, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_align_to(pl_gui_ta_reg_read_value, pl_gui_ta_reg_address,
			LV_ALIGN_OUT_BOTTOM_MID, 0,
			30);
	lv_obj_add_event(pl_gui_ta_reg_read_value, pl_gui_ta_event_handler,
			 LV_EVENT_CLICKED,
			 NULL);

	/* Register read value label */
	label = lv_label_create(parent);
	lv_label_set_text(label, "Read Value (hex)");
	lv_obj_align_to(label, pl_gui_ta_reg_read_value, LV_ALIGN_OUT_LEFT_MID, -10, 0);

	/* Register read value button */
	btn = lv_btn_create(parent);
	lv_obj_set_size(btn, 90, 50);
	lv_obj_align_to(btn, pl_gui_ta_reg_read_value, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

	/* Register read value button */
	label = lv_label_create(btn);
	lv_label_set_text(label, "Read");
	lv_obj_center(label);

	lv_obj_add_event(btn, pl_gui_reg_btn_event_cb, LV_EVENT_ALL, NULL);
	lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_PURPLE),
				  LV_PART_MAIN);

	/* Register write value text area */
	pl_gui_ta_reg_write_value = lv_textarea_create(parent);
	lv_textarea_set_one_line(pl_gui_ta_reg_write_value, true);
	lv_textarea_set_text(pl_gui_ta_reg_write_value, "0");
	lv_textarea_set_accepted_chars(pl_gui_ta_reg_write_value,
				       "0123456789ABCDEFabcdef");
	lv_textarea_set_max_length(pl_gui_ta_reg_write_value, 8);
	lv_obj_align_to(pl_gui_ta_reg_write_value, pl_gui_ta_reg_read_value,
			LV_ALIGN_OUT_BOTTOM_MID,
			0, 30);
	lv_obj_add_event(pl_gui_ta_reg_write_value, pl_gui_ta_event_handler,
			 LV_EVENT_CLICKED,
			 NULL);

	/* Register write value label */
	label = lv_label_create(parent);
	lv_label_set_text(label, "Write Value (hex)");
	lv_obj_align_to(label, pl_gui_ta_reg_write_value, LV_ALIGN_OUT_LEFT_MID, -10,
			0);

	/* Register write value button */
	btn = lv_btn_create(parent);
	lv_obj_set_size(btn, 90, 50);
	lv_obj_align_to(btn, pl_gui_ta_reg_write_value, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

	/* Register write value button label */
	label = lv_label_create(btn);
	lv_label_set_text(label, "Write");
	lv_obj_center(label);

	lv_obj_add_event(btn, pl_gui_reg_btn_event_cb, LV_EVENT_ALL, NULL);
	lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);

	/* Callbacks to manage visibilty of the hex keypad */
	lv_obj_add_event(pl_gui_ta_reg_address, pl_gui_manage_btnmap_kb,
			 LV_EVENT_FOCUSED,
			 NULL);
	lv_obj_add_event(pl_gui_ta_reg_address, pl_gui_manage_btnmap_kb,
			 LV_EVENT_DEFOCUSED,
			 NULL);
	lv_obj_add_event(pl_gui_ta_reg_write_value, pl_gui_manage_btnmap_kb,
			 LV_EVENT_FOCUSED,
			 NULL);
	lv_obj_add_event(pl_gui_ta_reg_write_value, pl_gui_manage_btnmap_kb,
			 LV_EVENT_DEFOCUSED,
			 NULL);

	return 0;
}

/**
 * @brief 	Create pocket lab GUI DMM view
 * @param	parent[in] - pointer to DMM view instance
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_create_dmm_view(lv_obj_t *parent,
			       struct pl_gui_init_param *param)
{
	int32_t ret;
	char dropdown_list[100];
	char chn_name[10];
	char chn_unit[10];
	char label_str[50];
	uint32_t cnt;
	uint32_t i=0, j=0;
	uint32_t row_height;
	lv_obj_t *obj;

	/* Get device names */
	dropdown_list[0] = '\0';
	ret = pl_gui_get_dev_names(dropdown_list);
	if (ret) {
		return ret;
	}

	/* Device select menu */
	pl_gui_dd_device_select = lv_dropdown_create(parent);
	lv_obj_align(pl_gui_dd_device_select, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
	lv_dropdown_set_options(pl_gui_dd_device_select, dropdown_list);
	lv_obj_add_event(pl_gui_dd_device_select, pl_gui_device_select_event_cb,
			 LV_EVENT_ALL,
			 NULL);

	/* DMM start button */
	pl_gui_dmm_btn_start = lv_btn_create(parent);
	lv_obj_set_size(pl_gui_dmm_btn_start, 120, 50);
	lv_obj_align_to(pl_gui_dmm_btn_start, pl_gui_dmm_btn_start,
			LV_ALIGN_OUT_BOTTOM_LEFT, 0,
			30);
	lv_obj_add_event(pl_gui_dmm_btn_start, pl_gui_dmm_btn_event_cb, LV_EVENT_ALL,
			 NULL);
	lv_obj_set_style_bg_color(pl_gui_dmm_btn_start,
				  lv_palette_main(LV_PALETTE_GREEN),
				  LV_PART_MAIN);

	/* DMM start button label */
	lv_obj_t *label = lv_label_create(pl_gui_dmm_btn_start);
	lv_label_set_text(label, "Start");
	lv_obj_center(label);

	/* DMM enable all button */
	pl_gui_dmm_btn_enable_all = lv_btn_create(parent);
	lv_obj_set_size(pl_gui_dmm_btn_enable_all, 120, 50);
	lv_obj_align_to(pl_gui_dmm_btn_enable_all, pl_gui_dmm_btn_start,
			LV_ALIGN_OUT_BOTTOM_LEFT, 0,
			30);
	lv_obj_add_event(pl_gui_dmm_btn_enable_all, pl_gui_dmm_btn_event_cb,
			 LV_EVENT_ALL,
			 NULL);

	/* DMM enable all button label */
	label = lv_label_create(pl_gui_dmm_btn_enable_all);
	lv_label_set_text(label, "Enable All");
	lv_obj_center(label);

	/* DMM disable all button */
	pl_gui_dmm_btn_disable_all = lv_btn_create(parent);
	lv_obj_set_size(pl_gui_dmm_btn_disable_all, 120, 50);
	lv_obj_align_to(pl_gui_dmm_btn_disable_all, pl_gui_dmm_btn_enable_all,
			LV_ALIGN_OUT_BOTTOM_LEFT, 0,
			30);
	lv_obj_add_event(pl_gui_dmm_btn_disable_all, pl_gui_dmm_btn_event_cb,
			 LV_EVENT_ALL,
			 NULL);

	/* DMM disable all button label */
	label = lv_label_create(pl_gui_dmm_btn_disable_all);
	lv_label_set_text(label, "Disable All");
	lv_obj_center(label);

	/* Create the scrolling view container */
	lv_obj_t *cont_col = lv_obj_create(parent);
	lv_obj_set_size(cont_col, 600, 400);
	lv_obj_align_to(cont_col, pl_gui_dd_device_select, LV_ALIGN_OUT_RIGHT_TOP, 20,
			0);

	/* Get the name of all channels for selected device */
	dropdown_list[0] = '\0';
	ret = pl_gui_get_chn_names(dropdown_list, &pl_gui_dmm_chn_cnt,
				   pl_gui_device_indx);
	if (ret) {
		return ret;
	}

	pl_gui_dmm_chn_checkbox = calloc(pl_gui_dmm_chn_cnt, sizeof(lv_obj_t));
	if (!pl_gui_dmm_chn_checkbox) {
		return -ENOMEM;
	}

	pl_gui_dmm_chn_ta = calloc(pl_gui_dmm_chn_cnt, sizeof(lv_obj_t));
	if (!pl_gui_dmm_chn_ta) {
		ret = -ENOMEM;
		goto error_dmm_chn_checkbox;
	}

	/* Display checkboxes for channels */
	for (cnt = 0; cnt < pl_gui_dmm_chn_cnt; cnt++) {
		label_str[0] = '\0';
		chn_unit[0] = '\0';
		chn_name[0] = '\0';
		row_height = 60;
		j = 0;

		while (dropdown_list[i] != '\n') {
			chn_name[j++] = dropdown_list[i++];
		}
		chn_name[j] = '\0';
		i++;

		/* Add channel enable checkboxs */
		obj = lv_checkbox_create(cont_col);
		sprintf(label_str, "%s", chn_name);
		lv_checkbox_set_text(obj, label_str);
		lv_obj_set_pos(obj, 10, cnt * row_height + 15);
		pl_gui_dmm_chn_checkbox[cnt] = obj;

		/* DMM channel value text area */
		obj = lv_textarea_create(cont_col);
		lv_textarea_set_one_line(obj, true);
		lv_textarea_set_text(obj, " ");
		lv_obj_set_size(obj, 150, 50);
		lv_obj_set_pos(obj, 150, cnt * row_height + 10);
		pl_gui_dmm_chn_ta[cnt] = obj;

		/* Add channel unit string */
		ret = pl_gui_get_chn_unit(chn_unit, cnt, pl_gui_device_indx);
		if (ret) {
			return ret;
		}
		lv_obj_t *label = lv_label_create(cont_col);
		sprintf(label_str, "%s", chn_unit);
		lv_label_set_text(label, label_str);
		lv_obj_set_pos(label, 320, cnt * row_height + 15);
	}

	return 0;

error_dmm_chn_checkbox:
	free(pl_gui_dmm_chn_checkbox);

	return ret;
}

/**
 * @brief 	Create pocket lab GUI data capture view
 * @param	parent[in] - pointer to data capture view instance
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_create_capture_view(lv_obj_t *parent,
				   struct pl_gui_init_param *param)
{
	int32_t ret;
	uint32_t cnt;
	char dropdown_list[100];
	char chn_name[10];
	char label_str[50];
	char ibuf[100];
	uint32_t i = 0, j = 0;
	lv_obj_t *obj;
	lv_obj_t *start_btn, *enable_all_btn, *disable_all_btn;
	lv_obj_t *label;

	/* Get device names */
	dropdown_list[0] = '\0';
	ret = pl_gui_get_dev_names(dropdown_list);
	if (ret) {
		return ret;
	}

	/* Device select menu */
	pl_gui_dd_device_select = lv_dropdown_create(parent);
	lv_obj_align(pl_gui_dd_device_select, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
	lv_dropdown_set_options(pl_gui_dd_device_select, dropdown_list);
	lv_obj_add_event(pl_gui_dd_device_select, pl_gui_device_select_event_cb,
			 LV_EVENT_ALL,
			 NULL);

	/* Create the capture Start/Stop Button */
	start_btn = lv_btn_create(parent);
	lv_obj_set_size(start_btn, 100, 45);
	lv_obj_align_to(start_btn, pl_gui_dd_device_select, LV_ALIGN_OUT_RIGHT_TOP, 10,
			0);
	lv_obj_add_event(start_btn, pl_gui_capture_btn_event_cb, LV_EVENT_ALL,
			 NULL);
	lv_obj_set_style_bg_color(start_btn, lv_palette_main(LV_PALETTE_GREEN),
				  LV_PART_MAIN);

	label = lv_label_create(start_btn);
	lv_label_set_text(label, "Start");
	lv_obj_center(label);

	/* Create the all channels enable button */
	enable_all_btn = lv_btn_create(parent);
	lv_obj_set_size(enable_all_btn, 120, 45);
	lv_obj_align_to(enable_all_btn, start_btn, LV_ALIGN_RIGHT_MID, 150,
			0);
	lv_obj_add_event(enable_all_btn, pl_gui_capture_btn_event_cb, LV_EVENT_ALL,
			 NULL);
	lv_obj_set_style_bg_color(enable_all_btn, lv_palette_main(LV_PALETTE_BLUE),
				  LV_PART_MAIN);

	label = lv_label_create(enable_all_btn);
	lv_label_set_text(label, "Enable All");
	lv_obj_center(label);

	/* Create the all channels disable button */
	disable_all_btn = lv_btn_create(parent);
	lv_obj_set_size(disable_all_btn, 120, 45);
	lv_obj_align_to(disable_all_btn, enable_all_btn, LV_ALIGN_RIGHT_MID, 150,
			0);
	lv_obj_add_event(disable_all_btn, pl_gui_capture_btn_event_cb, LV_EVENT_ALL,
			 NULL);
	lv_obj_set_style_bg_color(disable_all_btn, lv_palette_main(LV_PALETTE_BLUE),
				  LV_PART_MAIN);

	label = lv_label_create(disable_all_btn);
	lv_label_set_text(label, "Disable All");
	lv_obj_center(label);

	/* Create check boxes to enable display of channels */
	lv_obj_t *cont_col = lv_obj_create(parent);
	lv_obj_set_size(cont_col, 125, 340);
	lv_obj_align_to(cont_col, pl_gui_dd_device_select, LV_ALIGN_OUT_BOTTOM_MID, 0,
			10);
	lv_obj_set_flex_flow(cont_col, LV_FLEX_FLOW_COLUMN);

	/* Create a pl_gui_capture_chart for displaying axises (not for actual data display) */
	pl_gui_capture_chart = lv_chart_create(parent);
	lv_obj_set_size(pl_gui_capture_chart, 540, 340);
	lv_obj_align_to(pl_gui_capture_chart, cont_col, LV_ALIGN_OUT_RIGHT_MID, 100, 0);

	/* Display labels on x and y axises */
	lv_chart_set_axis_tick(pl_gui_capture_chart, LV_CHART_AXIS_PRIMARY_Y, 5, 0, 9,
			       1, true,
			       100);
	lv_chart_set_axis_tick(pl_gui_capture_chart, LV_CHART_AXIS_PRIMARY_X, 5, 0, 9,
			       1, true,
			       20);

	/* Set the x and y axises range (Input data range) */
	lv_chart_set_range(pl_gui_capture_chart, LV_CHART_AXIS_PRIMARY_Y,
			   PL_GUI_DATA_MIN_RANGE,
			   PL_GUI_DATA_MAX_RANGE);
	lv_chart_set_range(pl_gui_capture_chart, LV_CHART_AXIS_PRIMARY_X, 0,
			   PL_GUI_REQ_DATA_SAMPLES);

	/* Create an overlay pl_gui_capture_chart for displaying actual data */
	pl_gui_capture_chart_ovrly = lv_chart_create(parent);
	lv_obj_set_size(pl_gui_capture_chart_ovrly, 540, 340);
	lv_obj_align_to(pl_gui_capture_chart_ovrly, cont_col, LV_ALIGN_OUT_RIGHT_MID,
			100, 0);
	lv_chart_set_type(pl_gui_capture_chart_ovrly, LV_CHART_TYPE_LINE);
	lv_chart_set_update_mode(pl_gui_capture_chart_ovrly,
				 LV_CHART_UPDATE_MODE_CIRCULAR);

	/* Set the x and y axises range (rescaled from actual input data range) */
	lv_chart_set_range(pl_gui_capture_chart_ovrly, LV_CHART_AXIS_PRIMARY_Y,
			   PL_GUI_CHART_MIN_PXL_RANGE,
			   PL_GUI_CHART_MAX_PXL_RANGE);
	lv_chart_set_range(pl_gui_capture_chart_ovrly, LV_CHART_AXIS_PRIMARY_X, 0,
			   PL_GUI_REQ_DATA_SAMPLES);

	/* Do not display points on the data */
#if LV_VERSION_CHECK(9,0,0)
	lv_obj_set_style_size(pl_gui_capture_chart_ovrly, 0, 0, LV_PART_INDICATOR);
#else
	lv_obj_set_style_size(pl_gui_capture_chart_ovrly, 0, LV_PART_INDICATOR);
#endif

	/* Get the name of all channels and channel count */
	dropdown_list[0] = '\0';
	ret = pl_gui_get_chn_names(dropdown_list, &pl_gui_capture_chn_cnt,
				   pl_gui_device_indx);
	if (ret) {
		return ret;
	}

	pl_gui_capture_chn_checkbox = calloc(pl_gui_capture_chn_cnt, sizeof(lv_obj_t));
	if (!pl_gui_capture_chn_checkbox) {
		return -ENOMEM;
	}

	pl_gui_capture_chn_ser = calloc(pl_gui_capture_chn_cnt, sizeof(lv_obj_t));
	if (!pl_gui_capture_chn_ser) {
		ret = -ENOMEM;
		goto error_capture_chn_ser;
	}

	pl_gui_capture_chn_info = calloc(pl_gui_capture_chn_cnt,
					 sizeof(struct scan_type));
	if (!pl_gui_capture_chn_info) {
		ret = -ENOMEM;
		goto error_chn_info;
	}

	pl_gui_capture_offset = calloc(pl_gui_capture_chn_cnt, sizeof(int32_t));
	if (!pl_gui_capture_offset) {
		ret = -ENOMEM;
		goto error_offset;
	}

	/* Display checkboxes for capture view channels */
	for (cnt = 0; cnt < pl_gui_capture_chn_cnt; cnt++) {
		ibuf[0] = '\0';
		label_str[0] = '\0';
		chn_name[0] = '\0';
		j = 0;

		while (dropdown_list[i] != '\n') {
			chn_name[j++] = dropdown_list[i++];
		}
		chn_name[j] = '\0';
		i++;

		/* Add channel enable checkboxs */
		obj = lv_checkbox_create(cont_col);
		lv_obj_set_height(obj, 40);
		sprintf(label_str, "%s", chn_name);
		lv_checkbox_set_text(obj, label_str);
		pl_gui_capture_chn_checkbox[cnt] = obj;

		pl_gui_capture_chn_info[cnt] = malloc(sizeof(struct scan_type));
		if (!pl_gui_capture_chn_info[cnt]) {
			return -ENOMEM;
		}

		pl_gui_capture_offset[cnt] = malloc(sizeof(int32_t));
		if (!pl_gui_capture_offset[cnt]) {
			return -ENOMEM;
		}

		ret = pl_gui_read_chn_attr("offset", ibuf, cnt, pl_gui_device_indx);
		if (ret) {
			return ret;
		}

		sscanf(ibuf, "%d", pl_gui_capture_offset[cnt]);
	}

	return 0;

error_offset:
	free(pl_gui_capture_chn_info);
error_chn_info:
	free(pl_gui_capture_chn_ser);
error_capture_chn_ser:
	free(pl_gui_capture_chn_checkbox);

	return ret;
}

/**
 * @brief 	Create pocket lab GUI analysis view
 * @param	parent[in] - pointer to analysis view instance
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_create_analysis_view(lv_obj_t *parent,
				    struct pl_gui_init_param *param)
{
	int32_t ret;
	char dropdown_list[100];
	lv_obj_t *start_btn;
	lv_obj_t *label;

	/* Get device names */
	dropdown_list[0] = '\0';
	ret = pl_gui_get_dev_names(dropdown_list);
	if (ret) {
		return ret;
	}

	/* Device select menu */
	pl_gui_dd_device_select = lv_dropdown_create(parent);
	lv_obj_align(pl_gui_dd_device_select, LV_ALIGN_OUT_RIGHT_TOP, 10, 0);
	lv_dropdown_set_options(pl_gui_dd_device_select, dropdown_list);
	lv_obj_add_event(pl_gui_dd_device_select, pl_gui_device_select_event_cb,
			 LV_EVENT_ALL,
			 NULL);

	/* Get the name of all channels and channel count */
	dropdown_list[0] = '\0';
	ret = pl_gui_get_chn_names(dropdown_list, &pl_gui_fft_chn_cnt,
				   pl_gui_device_indx);
	if (ret) {
		return ret;
	}

	/* Create drop-down to select channel */
	pl_gui_fft_chn_select = lv_dropdown_create(parent);
	lv_obj_align_to(pl_gui_fft_chn_select, pl_gui_dd_device_select,
			LV_ALIGN_OUT_RIGHT_TOP,
			10, 0);
	lv_dropdown_set_options(pl_gui_fft_chn_select, dropdown_list);
	lv_obj_add_event(pl_gui_fft_chn_select, pl_gui_chn_select_event_cb,
			 LV_EVENT_ALL,
			 NULL);
	lv_obj_set_width(pl_gui_fft_chn_select, 150);

	/* Create the FFT Start/Stop Button */
	start_btn = lv_btn_create(parent);
	lv_obj_set_size(start_btn, 100, 40);
	lv_obj_align_to(start_btn, pl_gui_fft_chn_select,
			LV_ALIGN_OUT_RIGHT_TOP,
			10, 0);
	lv_obj_add_event(start_btn, pl_gui_fft_btn_event_cb, LV_EVENT_ALL,
			 NULL);
	lv_obj_set_style_bg_color(start_btn, lv_palette_main(LV_PALETTE_GREEN),
				  LV_PART_MAIN);

	label = lv_label_create(start_btn);
	lv_label_set_text(label, "Start");
	lv_obj_center(label);

	/* Create a pl_gui_fft_chart for displaying axises (not for actual data display) */
	pl_gui_fft_chart = lv_chart_create(parent);
	lv_obj_set_size(pl_gui_fft_chart, 600, 340);
	lv_obj_set_pos(pl_gui_fft_chart, 30, 50);

	/* Display labels on x and y axises */
	lv_chart_set_axis_tick(pl_gui_fft_chart, LV_CHART_AXIS_PRIMARY_Y, 5, 0, 9, 1,
			       true,
			       100);
	// TODO- Check if scale can be displayed in terms of sampling frequency (Fs)
	// Range: 0 to Fs/2. Resolution = Fs/FFT length
	//lv_chart_set_axis_tick(pl_gui_fft_chart, LV_CHART_AXIS_PRIMARY_X, 5, 0, 10, 1,
	//		       true,
	//		       20);

	/* Set the x and y axises range (input data range) */
	lv_chart_set_range(pl_gui_fft_chart, LV_CHART_AXIS_PRIMARY_Y,
			   -200,
			   0);
	lv_chart_set_range(pl_gui_fft_chart, LV_CHART_AXIS_PRIMARY_X, 0,
			   fft_bins);

	/* Do not display points on the data */
#if LV_VERSION_CHECK(9,0,0)
	lv_obj_set_style_size(pl_gui_fft_chart, 0, 0, LV_PART_INDICATOR);
#else
	lv_obj_set_style_size(pl_gui_fft_chart, 0, LV_PART_INDICATOR);
#endif

	/* Create other FFT parameters */
	label = lv_label_create(parent);
	lv_label_set_text(label, "THD:");
	lv_obj_set_pos(label, 640, 40);
	thd_label = lv_label_create(parent);
	lv_label_set_text(thd_label, "");
	lv_obj_set_pos(thd_label, 640, 60);

	label = lv_label_create(parent);
	lv_label_set_text(label, "SNR:");
	lv_obj_set_pos(label, 640, 100);
	snr_label = lv_label_create(parent);
	lv_label_set_text(snr_label, "");
	lv_obj_set_pos(snr_label, 640, 120);

	label = lv_label_create(parent);
	lv_label_set_text(label, "DR:");
	lv_obj_set_pos(label, 640, 160);
	dr_label = lv_label_create(parent);
	lv_label_set_text(dr_label, "");
	lv_obj_set_pos(dr_label, 640, 180);

	label = lv_label_create(parent);
	lv_label_set_text(label, "Fund Power:");
	lv_obj_set_pos(label, 640, 220);
	fund_power_label = lv_label_create(parent);
	lv_label_set_text(fund_power_label, "");
	lv_obj_set_pos(fund_power_label, 640, 240);

	label = lv_label_create(parent);
	lv_label_set_text(label, "Fund Frequency:");
	lv_obj_set_pos(label, 640, 280);
	fund_freq_label = lv_label_create(parent);
	lv_label_set_text(fund_freq_label, "");
	lv_obj_set_pos(fund_freq_label, 640, 300);

	label = lv_label_create(parent);
	lv_label_set_text(label, "RMS Noise:");
	lv_obj_set_pos(label, 640, 340);
	rms_noise_label = lv_label_create(parent);
	lv_label_set_text(rms_noise_label, "");
	lv_obj_set_pos(rms_noise_label, 640, 360);

	pl_gui_fft_chn_ser = calloc(pl_gui_fft_chn_cnt, sizeof(lv_obj_t));
	if (!pl_gui_capture_chn_ser) {
		return -ENOMEM;
	}

	/* Initialize the FFT parameters */
	return adi_fft_init(param->device_params->fft_params, &pl_gui_fft_proc,
			    &pl_gui_fft_meas);
}

/**
 * @brief 	Create pocket lab GUI about view
 * @param	parent[in] - pointer to about view instance
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_create_about_view(lv_obj_t *parent,
				 struct pl_gui_init_param *param)
{
	lv_obj_t *obj;
	lv_obj_t *label;

	/* Display ADI logo */
	LV_IMG_DECLARE(adi_logo);
	obj = lv_img_create(parent);
	lv_img_set_src(obj, &adi_logo);
	lv_img_set_size_mode(obj, LV_IMAGE_SIZE_MODE_REAL);
	lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, -5);

	/* Display labels */

	label = lv_label_create(parent);
	lv_label_set_text(label,
			  "Analog Devices Inc. Pocket Lab v0.1");
	lv_obj_align(label, LV_ALIGN_CENTER, 0, -50);

	label = lv_label_create(parent);
	lv_label_set_text(label,
			  "\n\n\n\n\n\n"
			  "Pocket Lab\n"
			  "A GUI for IIO devices\n\n"
			  "Pocket Lab is a GUI based embedded\n"
			  "application, developed for demoing\n"
			  "and evaluating the IIO devices.\n"
			  "The application supports device\n"
			  "configuration, registers r/w,\n"
			  "time/freq domain data plot, etc");
	lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 30);

	label = lv_label_create(parent);
	lv_label_set_text(label,
			  "\n\n\n\n\n\n"
			  "Industrial I/O Devices (IIO)\n\n\n"
			  "IIO subsytem is intended to provide\n"
			  "support for devices that in some\n"
			  "sense are analog to digital and\n"
			  "digital to analog converters\n"
			  "(ADCs and DACs).");
	lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 20);

	return 0;
}

/**
 * @brief 	Create pocket lab GUI views
 * @param	desc[in, out] - Pocket lab GUI descriptor
 * @param	param[in] - Pocket lab GUI init parameters
 * @return	0 in case of success, negative error code otherwise
 */
static int32_t pl_gui_create_views(struct pl_gui_desc **desc,
				   struct pl_gui_init_param *param)
{
	int32_t ret;
	uint32_t cnt;
	uint32_t nb_of_views = 0;
	struct pl_gui_desc *gui_desc;
	lv_obj_t *tabview;

	if (!param || !desc) {
		return -EINVAL;
	}

	/* Store the device init param descriptor */
	ret = pl_gui_save_dev_param_desc(param->extra);
	if (ret) {
		return ret;
	}

	/* Find the number of views */
	for (cnt = 0; param->views[cnt].view_name; cnt++) {
		nb_of_views++;
	}

	/* Allocate memory for GUI views descriptor */
	gui_desc = calloc(nb_of_views, sizeof(*gui_desc));
	if (!gui_desc) {
		return -ENOMEM;
	}

	/* Create a Tab view object, and the views within it */
	tabview = lv_tabview_create(lv_scr_act(), LV_DIR_BOTTOM, 50);

	/* Button matrix creation and mapping */
	pl_gui_add_btnmap_kb();

	/* Create pocket lab GUI views */
	for (cnt = 0; cnt < nb_of_views; cnt++) {
		gui_desc[cnt].view_obj = lv_tabview_add_tab(tabview,
					 param->views[cnt].view_name);
		param->views[cnt].create_view(gui_desc[cnt].view_obj, param);
	}

	/* Add content to the tab views */
	lv_tabview_set_act(tabview, 0, LV_ANIM_ON);

	*desc = gui_desc;

	return 0;
}

/**
 * @brief 	Init pocket lab GUI
 * @param	desc[in] - Pocket lab GUI descriptor
 * @param	param[in] - Pocket lab GUI init parameters
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_init(struct pl_gui_desc **desc,
		    struct pl_gui_init_param *param)
{
	int32_t ret;

	if (!param || !desc) {
		return -EINVAL;
	}

	/* Initialize the lvgl library */
	lv_init();

	/* Initialize the tft display and touchpad */
	tft_init();
	touchpad_init();

	ret = pl_gui_create_views(desc, param);
	if (ret) {
		return ret;
	}

	/* Save device specific callbacks called into application layer */
	data_to_volt_without_vref =
		param->device_params->fft_params->convert_data_to_volt_without_vref;
	data_to_volt_wrt_vref =
		param->device_params->fft_params->convert_data_to_volt_wrt_vref;
	code_to_straight_binary =
		param->device_params->fft_params->convert_code_to_straight_binary;

	fft_data_samples = param->device_params->fft_params->samples_count;
	fft_bins = fft_data_samples / 2;

	return 0;
}
