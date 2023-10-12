/*************************************************************************//**
 *   @file   pl_gui_iio_wrapper.h
 *   @brief  Pocket lab GUI IIO wrapper interfaces
******************************************************************************
* Copyright (c) 2023 Analog Devices, Inc.
* All rights reserved.
*
* This software is proprietary to Analog Devices, Inc. and its licensors.
* By using this software you agree to the terms of the associated
* Analog Devices Software License Agreement.
*****************************************************************************/

#ifndef _PL_GUI_IIO_WRAPPER_
#define _PL_GUI_IIO_WRAPPER_

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/

#include <stdint.h>
#include "iio.h"

/******************************************************************************/
/********************** Macros and Constants Definition ***********************/
/******************************************************************************/

/******************************************************************************/
/************************ Public Declarations *********************************/
/******************************************************************************/

int32_t pl_gui_save_dev_param_desc(struct iio_init_param *param);
int32_t pl_gui_get_dev_names(char *dev_names);
int32_t pl_gui_get_chn_names(char *chn_names, uint32_t *nb_of_chn,
			     uint32_t dev_indx);
int32_t pl_gui_get_chn_unit(char *chn_unit, uint32_t chn_indx,
			    uint32_t dev_indx);
int32_t pl_gui_get_global_attr_names(char *attr_names, uint32_t dev_indx);
int32_t pl_gui_get_chn_attr_names(char *attr_names, uint32_t chn_indx,
				  uint32_t dev_indx);
int32_t pl_gui_get_global_attr_avail_options(const char *attr_name,
		char *attr_val, uint32_t dev_indx);
int32_t pl_gui_get_chn_attr_avail_options(const char *attr_name,
		char *attr_val, uint32_t chn_indx, uint32_t dev_indx);
int32_t pl_gui_read_global_attr(const char *attr_name, char *attr_val,
				uint32_t dev_indx);
int32_t pl_gui_read_chn_attr(char *attr_name, char *attr_val,
			     uint32_t chn_indx, uint32_t dev_indx);
int32_t pl_gui_write_global_attr(const char *attr_name, char *attr_val,
				 uint32_t dev_indx);
int32_t pl_gui_write_chn_attr(const char *attr_name, char *attr_val,
			      uint32_t chn_indx, uint32_t dev_indx);
int32_t pl_gui_read_reg(uint32_t addr, uint32_t *data, uint32_t dev_indx);
int32_t pl_gui_write_reg(uint32_t addr, uint32_t data, uint32_t dev_indx);
int32_t pl_gui_get_dmm_reading(char *val, uint32_t chn_indx, uint32_t dev_indx);
int32_t pl_gui_read_chn_info(struct scan_type *chn_info, uint32_t chn_indx,
			     uint32_t dev_indx);

#endif // _PL_GUI_IIO_WRAPPER_
