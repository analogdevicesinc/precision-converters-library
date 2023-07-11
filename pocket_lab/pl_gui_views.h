/*************************************************************************//**
 *   @file   pl_gui_views.h
 *   @brief  Pocket lab GUI views
******************************************************************************
* Copyright (c) 2023 Analog Devices, Inc.
* All rights reserved.
*
* This software is proprietary to Analog Devices, Inc. and its licensors.
* By using this software you agree to the terms of the associated
* Analog Devices Software License Agreement.
*****************************************************************************/

#ifndef _PL_GUI_VIEWES_
#define _PL_GUI_VIEWES_

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/

#include <stdint.h>
#include "lvgl/lvgl.h"

/******************************************************************************/
/********************** Macros and Constants Definition ***********************/
/******************************************************************************/

#define PL_GUI_ADD_ATTR_EDIT_DEF_VIEW {\
	.view_name = "Configure", \
	.create_view = &pl_gui_create_attributes_view\
}

#define PL_GUI_ADD_REG_DEBUG_DEF_VIEW {\
	.view_name = "Register", \
	.create_view = &pl_gui_create_register_view\
}

#define PL_GUI_ADD_DMM_DEF_VIEW {\
	.view_name = "DMM", \
	.create_view = &pl_gui_create_dmm_view\
}

#define PL_GUI_ADD_CAPTURE_DEF_VIEW {\
	.view_name = "Capture", \
	.create_view = &pl_gui_create_capture_view\
}

#define PL_GUI_ADD_ABOUT_DEF_VIEW {\
	.view_name = "About", \
	.create_view = &pl_gui_create_about_view\
}

#define PL_GUI_ADD_VIEW(_name, _function) {\
	.view_name = _name,\
	.create_view = _function\
}

/* Requested data samples for capture. These number of samples would
 * be displayed onto display GUI capture tab at single instance */
#define PL_GUI_REQ_DATA_SAMPLES		400

/******************************************************************************/
/************************ Public Declarations *********************************/
/******************************************************************************/

/* Pocket lab GUI view parameters */
struct pl_gui_views {
	/* View name */
	const char *view_name;
	/* View create function */
	int32_t (*create_view)(lv_obj_t *);
};

/* Pocket lab GUI init parameters */
struct pl_gui_init_param {
	/* Pocket lab GUI views */
	struct pl_gui_views *views;
	/* Init parameters extra */
	void *extra;
};

/* Pocket lab GUI run time parameters */
struct pl_gui_desc {
	/* View object */
	lv_obj_t *view_obj;
};

int32_t pl_gui_init(struct pl_gui_desc **desc,
		    struct pl_gui_init_param *param);
int32_t pl_gui_create_attributes_view(lv_obj_t* parent);
int32_t pl_gui_create_register_view(lv_obj_t* parent);
int32_t pl_gui_create_dmm_view(lv_obj_t* parent);
int32_t pl_gui_create_capture_view(lv_obj_t* parent);
int32_t pl_gui_create_about_view(lv_obj_t* parent);
void pl_gui_get_capture_chns_mask(uint32_t *chn_mask);
void pl_gui_display_captured_data(uint8_t *buf, uint32_t rec_bytes);
bool pl_gui_is_dmm_running(void);
bool pl_gui_is_capture_running(void);
void pl_gui_perform_dmm_read(void);
uint32_t pl_gui_get_active_device_index(void);

#endif // _PL_GUI_VIEWES_
