/*!
 *****************************************************************************
  @file:  ptc_ky81_110.h

  @brief: This file contains the global parameters for ptc_ky81_110 module

  @details:
 -----------------------------------------------------------------------------
 Copyright (c) 2021 Analog Devices, Inc.  All rights reserved.

 This software is proprietary to Analog Devices, Inc. and its licensors.
 By using this software you agree to the terms of the associated
 Analog Devices Software License Agreement.

*****************************************************************************/

#include <stdint.h>

#ifndef _PTC_KY81_110_H_
#define _PTC_KY81_110_H_

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/

#include "thermistor.h"

/* This is a child class of thermistor parent class and contains
 * attributes specific to KY81/110 PTC sensor */
class ptc_ky81_110 : thermistor
{
private:
	/* KY81/110 PTC temperature coefficient */
	float temperature_coeff;
#ifdef DEFINE_LOOKUP_TABLES
	int16_t lut_offset;
	int16_t lut_size;
	static const uint32_t lut[];
#endif

public:
	ptc_ky81_110();
	float convert(const float resistance);
#ifdef DEFINE_LOOKUP_TABLES
	float lookup(const float resistance);
#endif
};

#endif	/* _PTC_KY81_110_H_ */
