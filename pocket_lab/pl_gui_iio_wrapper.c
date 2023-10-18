/***************************************************************************//**
 *   @file    pl_gui_iio_wrapper.c
 *   @brief   Pocket lab GUI IIO wrapper interfaces
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
#include <string.h>

#include "pl_gui_iio_wrapper.h"
#include "no_os_error.h"

/******************************************************************************/
/************************ Macros/Constants ************************************/
/******************************************************************************/

/******************************************************************************/
/******************** Variables and User Defined Data Types *******************/
/******************************************************************************/

/* IIO init parameters structure pointer */
static struct iio_init_param *pl_gui_iio_init_params = NULL;

/******************************************************************************/
/************************ Functions Prototypes ********************************/
/******************************************************************************/

/******************************************************************************/
/************************ Functions Definitions *******************************/
/******************************************************************************/

/**
 * @brief 	Save the iio init params descriptor for future use
 * @param	param[in] - IIO init parameters structure pointer
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_save_dev_param_desc(struct iio_init_param *param)
{
	if (!param) {
		return -EINVAL;
	}

	pl_gui_iio_init_params = param;

	return 0;
}

/**
 * @brief 	Get IIO device names
 * @param	dev_names[in,out] - Device names string
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_get_dev_names(char *dev_names)
{
	uint32_t cnt;
	char temp[50];

	if (!dev_names || !pl_gui_iio_init_params) {
		return -EINVAL;
	}

	/* Get IIO device name and store into string array each separated by '\n' */
	for (cnt = 0; cnt < pl_gui_iio_init_params->nb_devs; cnt++) {
		sprintf(temp, "%s\n", pl_gui_iio_init_params->devs[cnt].name);
		strcat(dev_names, temp);
	}

	return 0;
}

/**
 * @brief 	Get IIO channels names
 * @param	chn_names[in,out] - Channel names string
 * @param	nb_of_chn[in,out] - Channels count
 * @param	dev_indx[in] - Current device index
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_get_chn_names(char *chn_names, uint32_t *nb_of_chn,
			     uint32_t dev_indx)
{
	struct iio_device *iio_dev;
	uint32_t chn_indx;
	char temp[50];

	if (!pl_gui_iio_init_params || !chn_names || !nb_of_chn
	    || (dev_indx >= pl_gui_iio_init_params->nb_devs)) {
		return -EINVAL;
	}

	/* Get IIO channel names and store into string array each separated by '\n' */
	iio_dev = pl_gui_iio_init_params->devs[dev_indx].dev_descriptor;
	for (chn_indx = 0; chn_indx < iio_dev->num_ch; chn_indx++) {
		sprintf(temp, "%s\n", iio_dev->channels[chn_indx].name);
		strcat(chn_names, temp);
	}

	*nb_of_chn = iio_dev->num_ch;

	return 0;
}

/**
 * @brief 	Get IIO channels unit type (in string format)
 * @param	chn_unit[in,out] - Channel unit string
 * @param	chn_indx[in] - Current channel index
 * @param	dev_indx[in] - Current device index
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_get_chn_unit(char *chn_unit, uint32_t chn_indx,
			    uint32_t dev_indx)
{
	struct iio_device *iio_dev;

	if (!pl_gui_iio_init_params || !chn_unit
	    || (dev_indx >= pl_gui_iio_init_params->nb_devs)
	    || (chn_indx >=
		pl_gui_iio_init_params->devs[dev_indx].dev_descriptor->num_ch)) {
		return -EINVAL;
	}

	/* Get IIO channel names and store into string array each separated by '\n' */
	iio_dev = pl_gui_iio_init_params->devs[dev_indx].dev_descriptor;

	switch (iio_dev->channels[chn_indx].ch_type) {
	case IIO_VOLTAGE:
		strcpy(chn_unit, "Volt");
		break;

	case IIO_CURRENT:
		strcpy(chn_unit, "mA");
		break;

	case IIO_TEMP:
		strcpy(chn_unit, "degree C");
		break;

	case IIO_ACCEL:
		strcpy(chn_unit, "g");
		break;

	default:
		break;
	}

	return 0;
}

/**
 * @brief 	Get IIO global attributes names
 * @param	attr_names[in,out] - Attribute names string
 * @param	dev_indx[in] - Current device index
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_get_global_attr_names(char *attr_names, uint32_t dev_indx)
{
	struct iio_device *iio_dev;
	uint32_t cnt;
	char temp[50];

	if (!pl_gui_iio_init_params || !attr_names
	    || (dev_indx >= pl_gui_iio_init_params->nb_devs)) {
		return -EINVAL;
	}

	/* Get IIO global attributes names and store into string array each separated by '\n' */
	iio_dev = pl_gui_iio_init_params->devs[dev_indx].dev_descriptor;
	for (cnt = 0; iio_dev->attributes[cnt].name; cnt++) {
		if (!strstr(iio_dev->attributes[cnt].name, "_available")) {
			sprintf(temp, "%s\n", iio_dev->attributes[cnt].name);
			strcat(attr_names, temp);
		}
	}

	return 0;
}

/**
 * @brief 	Get IIO channel attributes names
 * @param	attr_names[in,out] - Attribute names
 * @param	chn_indx[in] - Current channel index
 * @param	dev_indx[in] - Current device index
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_get_chn_attr_names(char *attr_names, uint32_t chn_indx,
				  uint32_t dev_indx)
{
	struct iio_device *iio_dev;
	uint32_t cnt;
	char temp[50];

	if (!pl_gui_iio_init_params || !attr_names
	    || (dev_indx >= pl_gui_iio_init_params->nb_devs)
	    || (chn_indx >=
		pl_gui_iio_init_params->devs[dev_indx].dev_descriptor->num_ch)) {
		return -EINVAL;
	}

	/* Get IIO channel names and store into string array each separated by '\n' */
	iio_dev = pl_gui_iio_init_params->devs[dev_indx].dev_descriptor;
	for (cnt = 0; iio_dev->channels[chn_indx].attributes[cnt].name; cnt++) {
		if (!strstr(iio_dev->channels[chn_indx].attributes[cnt].name, "_available")) {
			sprintf(temp, "%s\n", iio_dev->channels[chn_indx].attributes[cnt].name);
			strcat(attr_names, temp);
		}
	}

	return 0;
}

/**
 * @brief 	Get global available attributes options
 * @param	attr_name[in] - Attribute name
 * @param	attr_val[in,out] - Attribute values/options
 * @param	dev_indx[in] - Current device index
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_get_global_attr_avail_options(const char *attr_name,
		char *attr_val, uint32_t dev_indx)
{
	struct iio_device *iio_dev;
	struct iio_ch_info chn_info;
	char buf[100];
	uint32_t attr_indx;
	bool found = false;

	if (!pl_gui_iio_init_params || !attr_name || !attr_val
	    || (dev_indx >= pl_gui_iio_init_params->nb_devs)) {
		return -EINVAL;
	}

	/* Get IIO channel names and store into string array */
	iio_dev = pl_gui_iio_init_params->devs[dev_indx].dev_descriptor;
	for (attr_indx = 0; iio_dev->attributes[attr_indx].name; attr_indx++) {
		if (strstr(iio_dev->attributes[attr_indx].name, "_available")) {
			if (strstr(iio_dev->attributes[attr_indx].name, attr_name)) {
				iio_dev->attributes[attr_indx].show(pl_gui_iio_init_params->devs[dev_indx].dev,
								    buf,
								    sizeof(buf),
								    &chn_info, iio_dev->attributes[attr_indx].priv);
				strcpy(attr_val, buf);
				found = true;
				break;
			}
		}
	}

	if (!found) {
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief 	Get channel available attributes options
 * @param	attr_name[in] - Attribute name
 * @param	attr_val[in,out] - Attribute values/options
 * @param	chn_indx[in] - Current channel index
 * @param	dev_indx[in] - Current device index
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_get_chn_attr_avail_options(const char *attr_name,
		char *attr_val, uint32_t chn_indx, uint32_t dev_indx)
{
	struct iio_device *iio_dev;
	struct iio_ch_info chn_info;
	char buf[100];
	uint32_t attr_indx;
	bool found = false;

	if (!pl_gui_iio_init_params || !attr_name || !attr_val
	    || (dev_indx >= pl_gui_iio_init_params->nb_devs)
	    || (chn_indx >=
		pl_gui_iio_init_params->devs[dev_indx].dev_descriptor->num_ch)) {
		return -EINVAL;
	}

	/* Get IIO channel names and store into string array */
	iio_dev = pl_gui_iio_init_params->devs[dev_indx].dev_descriptor;
	for (attr_indx = 0; iio_dev->channels[chn_indx].attributes[attr_indx].name;
	     attr_indx++) {
		if (strstr(iio_dev->channels[chn_indx].attributes[attr_indx].name,
			   "_available")) {
			if (strstr(iio_dev->channels[chn_indx].attributes[attr_indx].name, attr_name)) {
				iio_dev->channels[chn_indx].attributes[attr_indx].show(
					pl_gui_iio_init_params->devs[dev_indx].dev, buf,
					sizeof(buf), &chn_info,
					iio_dev->channels[chn_indx].attributes[attr_indx].priv);
				strcpy(attr_val, buf);
				found = true;
				break;
			}
		}
	}

	if (!found) {
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief 	Read global attr value
 * @param	attr_name[in] - Attribute name
 * @param	attr_val[in,out] - Attribute value
 * @param	dev_indx[in] - Current device index
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_read_global_attr(const char *attr_name, char *attr_val,
				uint32_t dev_indx)
{
	struct iio_device *iio_dev;
	struct iio_ch_info chn_info;
	uint32_t attr_indx;
	char buf[50];

	if (!pl_gui_iio_init_params || !attr_name || !attr_val
	    || (dev_indx >= pl_gui_iio_init_params->nb_devs)) {
		return -EINVAL;
	}

	/* Find attribute index from attributes list array */
	iio_dev = pl_gui_iio_init_params->devs[dev_indx].dev_descriptor;
	for (attr_indx = 0; iio_dev->attributes[attr_indx].name;
	     attr_indx++) {
		if (!strcmp(iio_dev->attributes[attr_indx].name, attr_name)) {
			break;
		}
	}

	/* Read the attribute value */
	iio_dev->attributes[attr_indx].show(pl_gui_iio_init_params->devs[dev_indx].dev,
					    buf,
					    sizeof(buf),
					    &chn_info, iio_dev->attributes[attr_indx].priv);

	strcpy(attr_val, buf);

	return 0;
}

/**
 * @brief 	Read channel attr value
 * @param	attr_name[in] - Attribute name
 * @param	attr_val[in,out] - Attribute value
 * @param	chn_indx[in] - Current channel index
 * @param	dev_indx[in] - Current device index
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_read_chn_attr(char *attr_name, char *attr_val,
			     uint32_t chn_indx, uint32_t dev_indx)
{
	struct iio_device *iio_dev;
	struct iio_ch_info chn_info;
	uint32_t attr_indx;
	char buf[50];

	if (!pl_gui_iio_init_params || !attr_name || !attr_val
	    || (dev_indx >= pl_gui_iio_init_params->nb_devs)
	    || (chn_indx >=
		pl_gui_iio_init_params->devs[dev_indx].dev_descriptor->num_ch)) {
		return -EINVAL;
	}

	chn_info.ch_num = chn_indx;

	/* Find attribute index from attributes list array */
	iio_dev = pl_gui_iio_init_params->devs[dev_indx].dev_descriptor;
	for (attr_indx = 0; iio_dev->channels[chn_indx].attributes[attr_indx].name;
	     attr_indx++) {
		if (!strcmp(iio_dev->channels[chn_indx].attributes[attr_indx].name,
			    attr_name)) {
			break;
		}
	}

	/* Read channel attribute */
	iio_dev->channels[chn_indx].attributes[attr_indx].show(
		pl_gui_iio_init_params->devs[dev_indx].dev,
		buf, sizeof(buf), &chn_info,
		iio_dev->channels[chn_indx].attributes[attr_indx].priv);

	strcpy(attr_val, buf);

	return 0;
}

/**
 * @brief 	Write global attr value
 * @param	attr_names[in] - Attribute name
 * @param	attr_val[in] - Attribute value
 * @param	dev_indx[in] - Current device index
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_write_global_attr(const char *attr_name, char *attr_val,
				 uint32_t dev_indx)
{
	struct iio_device *iio_dev;
	struct iio_ch_info chn_info;
	uint32_t attr_indx;
	char buf[50];

	if (!pl_gui_iio_init_params || !attr_name || !attr_val
	    || (dev_indx >= pl_gui_iio_init_params->nb_devs)) {
		return -EINVAL;
	}

	strcpy(buf, attr_val);
	strcat(buf, "\0");

	/* Find attribute index from attributes list array */
	iio_dev = pl_gui_iio_init_params->devs[dev_indx].dev_descriptor;
	for (attr_indx = 0; iio_dev->attributes[attr_indx].name;
	     attr_indx++) {
		if (!strcmp(iio_dev->attributes[attr_indx].name, attr_name)) {
			break;
		}
	}

	/* Write attribute value */
	iio_dev->attributes[attr_indx].store(pl_gui_iio_init_params->devs[dev_indx].dev,
					     buf,
					     strlen(buf),
					     &chn_info, iio_dev->attributes[attr_indx].priv);

	return 0;
}

/**
 * @brief 	Write channel attr value
 * @param	attr_name[in] - Attribute name
 * @param	attr_val[in] - Attribute value
 * @param	chn_indx[in] - Current channel index
 * @param	dev_indx[in] - Current device index
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_write_chn_attr(const char *attr_name, char *attr_val,
			      uint32_t chn_indx, uint32_t dev_indx)
{
	struct iio_device *iio_dev;
	struct iio_ch_info chn_info;
	uint32_t attr_indx;
	char buf[50];

	if (!pl_gui_iio_init_params || !attr_name || !attr_val
	    || (dev_indx >= pl_gui_iio_init_params->nb_devs)
	    || (chn_indx >=
		pl_gui_iio_init_params->devs[dev_indx].dev_descriptor->num_ch)) {
		return -EINVAL;
	}

	chn_info.ch_num = chn_indx;
	strcpy(buf, attr_val);
	strcat(buf, "\0");

	/* Find attribute index from attributes list array */
	iio_dev = pl_gui_iio_init_params->devs[dev_indx].dev_descriptor;
	for (attr_indx = 0; iio_dev->channels[chn_indx].attributes[attr_indx].name;
	     attr_indx++) {
		if (!strcmp(iio_dev->channels[chn_indx].attributes[attr_indx].name,
			    attr_name)) {
			break;
		}
	}

	/* Write attribute value */
	iio_dev->channels[chn_indx].attributes[attr_indx].show(
		pl_gui_iio_init_params->devs[dev_indx].dev,
		buf, strlen(buf), &chn_info,
		iio_dev->channels[chn_indx].attributes[attr_indx].priv);

	return 0;
}

/**
 * @brief 	Read register value
 * @param	addr[in] - Register address
 * @param	data[in,out] - Register value
 * @param	dev_indx[in] - Current device index
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_read_reg(uint32_t addr, uint32_t *data, uint32_t dev_indx)
{
	struct iio_device *iio_dev;

	if (!pl_gui_iio_init_params || !data
	    || (dev_indx >= pl_gui_iio_init_params->nb_devs)) {
		return -EINVAL;
	}

	iio_dev = pl_gui_iio_init_params->devs[dev_indx].dev_descriptor;
	iio_dev->debug_reg_read(pl_gui_iio_init_params->devs[dev_indx].dev, addr, data);

	return 0;
}

/**
 * @brief 	Write register value
 * @param	addr[in] - Register address
 * @param	data[in] - Register value
 * @param	dev_indx[in] - Current device index
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_write_reg(uint32_t addr, uint32_t data, uint32_t dev_indx)
{
	struct iio_device *iio_dev;

	if (!pl_gui_iio_init_params || dev_indx >= pl_gui_iio_init_params->nb_devs) {
		return -EINVAL;
	}

	iio_dev = pl_gui_iio_init_params->devs[dev_indx].dev_descriptor;
	iio_dev->debug_reg_write(pl_gui_iio_init_params->devs[dev_indx].dev, addr,
				 data);

	return 0;
}

/**
 * @brief 	Get DMM reading
 * @param	val[in, out] - DMM reading
 * @param	chn_indx[in] - Current channel index
 * @param	dev_indx[in] - Current device index
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_get_dmm_reading(char *val, uint32_t chn_indx, uint32_t dev_indx)
{
	struct iio_device *iio_dev;
	struct iio_ch_info chn_info;
	char buf[50];
	int32_t offset = 0;
	uint32_t raw = 0;
	uint32_t attr_indx;
	float scale = 0;
	float dmm_reading;
	bool found = false;

	if (!pl_gui_iio_init_params || !val
	    || (dev_indx >= pl_gui_iio_init_params->nb_devs)
	    || (chn_indx >=
		pl_gui_iio_init_params->devs[dev_indx].dev_descriptor->num_ch)) {
		return -EINVAL;
	}

	chn_info.ch_num = chn_indx;

	/* Find and read the raw attribute value */
	iio_dev = pl_gui_iio_init_params->devs[dev_indx].dev_descriptor;
	for (attr_indx = 0; iio_dev->channels[chn_indx].attributes[attr_indx].name;
	     attr_indx++) {
		if (!strcmp(iio_dev->channels[chn_indx].attributes[attr_indx].name, "raw")) {
			iio_dev->channels[chn_indx].attributes[attr_indx].show(
				pl_gui_iio_init_params->devs[dev_indx].dev,
				buf, strlen(buf), &chn_info,
				iio_dev->channels[chn_indx].attributes[attr_indx].priv);

			sscanf(buf, "%d", &raw);
			found = true;
			break;
		}
	}

	if (!found) {
		return -EIO;
	}

	/* Find and read the scale attribute value */
	for (attr_indx = 0; iio_dev->channels[chn_indx].attributes[attr_indx].name;
	     attr_indx++) {
		if (!strcmp(iio_dev->channels[chn_indx].attributes[attr_indx].name, "scale")) {
			iio_dev->channels[chn_indx].attributes[attr_indx].show(
				pl_gui_iio_init_params->devs[dev_indx].dev,
				buf, strlen(buf), &chn_info,
				iio_dev->channels[chn_indx].attributes[attr_indx].priv);

			sscanf(buf, "%f", &scale);
			found = true;
			break;
		}
	}

	if (!found) {
		return -EIO;
	}

	/* Find and read the offset attribute value */
	for (attr_indx = 0; iio_dev->channels[chn_indx].attributes[attr_indx].name;
	     attr_indx++) {
		if (!strcmp(iio_dev->channels[chn_indx].attributes[attr_indx].name, "offset")) {
			iio_dev->channels[chn_indx].attributes[attr_indx].show(
				pl_gui_iio_init_params->devs[dev_indx].dev,
				buf, strlen(buf), &chn_info,
				iio_dev->channels[chn_indx].attributes[attr_indx].priv);

			sscanf(buf, "%d", &offset);
			found = true;
			break;
		}
	}

	if (!found) {
		return -EIO;
	}

	dmm_reading = ((int32_t)(raw + offset) * scale) / 1000.0;
	sprintf(val, "%f", dmm_reading);

	return 0;
}

/**
 * @brief 	Read channel scan info
 * @param	chn_info[in, out] - Channel scan info
 * @param	chn_indx[in] - Current channel index
 * @param	dev_indx[in] - Current device index
 * @return	0 in case of success, negative error code otherwise
 */
int32_t pl_gui_read_chn_info(struct scan_type *chn_info, uint32_t chn_indx,
			     uint32_t dev_indx)
{
	struct iio_device *iio_dev;

	if (!pl_gui_iio_init_params || !chn_info ||
	    (dev_indx >= pl_gui_iio_init_params->nb_devs) ||
	    (chn_indx >= pl_gui_iio_init_params->devs[dev_indx].dev_descriptor->num_ch)) {
		return -EINVAL;
	}

	/* Get IIO channel names and store into string array */
	iio_dev = pl_gui_iio_init_params->devs[dev_indx].dev_descriptor;

	chn_info->storagebits = iio_dev->channels[chn_indx].scan_type->storagebits;
	chn_info->realbits = iio_dev->channels[chn_indx].scan_type->realbits;
	chn_info->sign = iio_dev->channels[chn_indx].scan_type->sign;
	chn_info->shift = iio_dev->channels[chn_indx].scan_type->shift;

	return 0;
}
