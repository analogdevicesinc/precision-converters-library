/*************************************************************************//**
 *   @file   pl_gui_events.h
 *   @brief  Pocket lab GUI event handling headers
******************************************************************************
* Copyright (c) 2023 Analog Devices, Inc.
* All rights reserved.
*
* This software is proprietary to Analog Devices, Inc. and its licensors.
* By using this software you agree to the terms of the associated
* Analog Devices Software License Agreement.
*****************************************************************************/

#ifndef _PL_GUI_EVENTS_
#define _PL_GUI_EVENTS_

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/

#include <stdint.h>
#include "pl_gui_iio_wrapper.h"

/******************************************************************************/
/********************** Macros and Constants Definition ***********************/
/******************************************************************************/

/******************************************************************************/
/************************ Public Declarations *********************************/
/******************************************************************************/

void pl_gui_lvgl_tick_update(uint32_t tick_time);
void pl_gui_event_handle(uint32_t tick_time);
int32_t pl_gui_event_read(uint8_t *buf, uint32_t len);
int32_t pl_gui_event_write(uint8_t *buf, uint32_t len);
void pl_gui_store_chn_info(struct scan_type *ch_info, uint32_t chn_indx);

#endif // _PL_GUI_EVENTS_
