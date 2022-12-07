/*!
 *****************************************************************************
  @file:  thermistor.h

  @brief:

  @details:
 -----------------------------------------------------------------------------
 Copyright (c) 2021 Analog Devices, Inc.  All rights reserved.

 This software is proprietary to Analog Devices, Inc. and its licensors.
 By using this software you agree to the terms of the associated
 Analog Devices Software License Agreement.

*****************************************************************************/

#include <stdint.h>

#ifndef _THERMISTOR_H_
#define _THERMISTOR_H_

/* Enable this macro to use look-up tables for temperature conversion */
#define DEFINE_LOOKUP_TABLES

class thermistor
{
public:
	thermistor();
	~thermistor();
	static float lookup(const uint32_t *lut,
			    uint32_t resistance,
			    uint16_t size,
			    int16_t offset);
	static float convert(const float resistance, float coeff_A, float coeff_B,
			     float coeff_C);
	virtual float convert(const float resistance) = 0;
	virtual float lookup(const float resistance) = 0;
};

#endif	/* _THERMISTOR_H_ */
