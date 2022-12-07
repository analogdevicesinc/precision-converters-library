/*!
 *****************************************************************************
  @file:  ntc_10k_44031.h

  @brief:

  @details:
 -----------------------------------------------------------------------------
 Copyright (c) 2021 Analog Devices, Inc.  All rights reserved.

 This software is proprietary to Analog Devices, Inc. and its licensors.
 By using this software you agree to the terms of the associated
 Analog Devices Software License Agreement.

*****************************************************************************/

#include <stdint.h>

#ifndef _NTC_10K_44031_H_
#define _NTC_10K_44031_H_

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/

#include "thermistor.h"

/* This is a child class of thermistor parent class and contains
 * attributes specific to 10K 44031 NTC sensor */
class ntc_10k_44031rc : thermistor
{
private:
	/* NTC coefficients for Steinhart-Hart equation */
	float coeff_A;
	float coeff_B;
	float coeff_C;
#ifdef DEFINE_LOOKUP_TABLES
	int16_t lut_offset;
	int16_t lut_size;
	static const uint32_t lut[];
#endif

public:
	ntc_10k_44031rc();
	float convert(const float resistance);
#ifdef DEFINE_LOOKUP_TABLES
	float lookup(const float resistance);
#endif
};

#endif	/* _NTC_10K_44031_H_ */
