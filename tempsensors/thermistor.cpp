/*!
 *****************************************************************************
  @file:  thermistor.cpp

  @brief: Thermistor sensor module

  @details:
 -----------------------------------------------------------------------------
 Copyright (c) 2021 Analog Devices, Inc.  All rights reserved.

 This software is proprietary to Analog Devices, Inc. and its licensors.
 By using this software you agree to the terms of the associated
 Analog Devices Software License Agreement.

*****************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/

#include <math.h>
#include "thermistor.h"

/******************************************************************************/
/************************** Functions Definitions *****************************/
/******************************************************************************/

thermistor::thermistor() {};
thermistor::~thermistor() {};

/*!
 * @brief	Convert the thermistor resistance into equivalent temperature
 * @param	resistance[in]- Thermistor resistance (Rth)
 * @param	coeff_A[in] - A coefficient for conversion
 * @param	coeff_B[in] - B coefficient for conversion
 * @param	coeff_C[in] - C coefficient for conversion
 * @return	Thermistor temperature value
 * @note	This function uses Steinhart-Hart equation for converting Thermistor
 *			resistance into temperature. Refer below design note for more details:
 *			https://www.analog.com/en/design-center/reference-designs/circuits-from-the-lab/cn0545.html
 */
float thermistor::convert(const float resistance, float coeff_A, float coeff_B,
			  float coeff_C)
{
	float temperature = 25.0;

	/* Get temperature into celcius */
	temperature = 1 / (coeff_A + (coeff_B * log(resistance)) + (coeff_C * pow(log(
				   resistance), 3)));
	temperature -= 273.15;

	return temperature;
}


/*!
 * @brief	Convert the thermistor resistance into equivalent temperature using
 *			lookup table
 * @param	lut[in]- Pointer to look-up table
 * @param	resistance[in] - thermistor resistance
 * @param	size[in] - look-up table size
 * @param	offset[in] - look-up table offset
 * @return	Thermistor temperature value
 */
float thermistor::lookup(const uint32_t *lut, uint32_t resistance,
			 uint16_t size, int16_t offset)
{
	uint16_t first = 0;
	uint16_t last = size - 1;
	uint16_t middle = (first + last) / 2;

	while (first <= last) {
		if (resistance < lut[middle])
			first = middle + 1;
		else if (lut[middle] == resistance) {
			return static_cast<float>(middle + offset);
		} else
			last = middle - 1;

		middle = (first + last) / 2;
	}

	if (first > last)
		return static_cast<float>(first + offset);

	return 0; // should never get here
}
