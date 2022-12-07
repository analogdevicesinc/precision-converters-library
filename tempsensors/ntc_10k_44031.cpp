/*!
 *****************************************************************************
  @file:  ntc_10k_44031.cpp

  @brief: This file contains functionality for 10K NTC 44021 model

  @details:
 -----------------------------------------------------------------------------
 Copyright (c) 2021 Analog Devices, Inc.  All rights reserved.

 This software is proprietary to Analog Devices, Inc. and its licensors.
 By using this software you agree to the terms of the associated
 Analog Devices Software License Agreement.

*****************************************************************************/

#include <math.h>
#include "thermistor.h"
#include "ntc_10k_44031.h"

/* Convert the temperature using Beta factor specified for 44031 10K NTC */
//#define	NTC_10K_44031_CONVERT_USING_BETA_VALUE

#if defined(NTC_10K_44031_CONVERT_USING_BETA_VALUE)
#define NTC_10K_44031_RESISTANCE_AT_25C		10000	// 10K
#define NTC_10K_44031_ROOM_TEMP_IN_KELVIN	298.15
#define NTC_10K_44031_BETA_VALUE			3694
#endif

#ifdef DEFINE_LOOKUP_TABLES
/* 10K NTC look-up table. Values are resistance in ohm for temperature
 * range from -10 to 80C with +/-1C tolerance.
 * @note This function uses Steinhart-Hart equation for deriving look-up table.
**/
const uint32_t ntc_10k_44031rc::lut[] = {
	47561,	45285,	43131,	41091,	39158,	37327,	35591,	33946,	32385,
	30905,	29500,	28166,	26900,	25697,	24555,	23470,	22438,	21457,
	20524,	19637,	18792,	17989,	17224,	16495,	15801,	15140,	14510,
	13910,	13337,	12791,	12271,	11774,	11299,	10847,	10414,	10002,
	9607,	9231,	8870,	8526,	8197,	7882,	7581,	7293,	7018,
	6754,	6501,	6259,	6028,	5806,	5593,	5389,	5194,	5006,
	4827,	4654,	4489,	4331,	4178,	4032,	3892,	3757,	3628,
	3503,	3384,	3269,	3159,	3053,	2951,	2852,	2758,	2667,
	2580,	2496,	2415,	2337,	2262,	2189,	2120,	2053,	1988,
	1926,	1866,	1808,	1752,	1698,	1646,	1596,	1548,	1501,
	1456
};
#endif


/*!
 * @brief	This is a constructor for ntc_10k_44031rc class
 * @return	none
 */
ntc_10k_44031rc::ntc_10k_44031rc()
{
	/* Coefficients of Steinhart-Hart equation for 10K NTC to convert
	 * NTC resistance into equivalent temperature */
	ntc_10k_44031rc::coeff_A = 1.032*pow(10, -3);
	ntc_10k_44031rc::coeff_B = 2.387*pow(10, -4);
	ntc_10k_44031rc::coeff_C = 1.580*pow(10, -7);

#ifdef DEFINE_LOOKUP_TABLES
	ntc_10k_44031rc::lut_offset = -10;	/* Min temperature obtained through LUT */
	ntc_10k_44031rc::lut_size = 90;		/* Temperature range defined in LUT
										   [lut_offset : lut_size - lut_offset] */
#endif
}


/*!
 * @brief	Convert the thermistor resistance into equivalent temperature using
 *			Steinhart-Hart equation Or Beta value for 10K 44031 NTC
 * @param	resistance[in] - thermistor resistance
 * @return	Thermistor temperature value in Celcius
 */
float ntc_10k_44031rc::convert(const float resistance)
{
#if defined(NTC_10K_44031_CONVERT_USING_BETA_VALUE)
	float temperature;
	temperature = (1 / ((log(resistance / NTC_10K_44031_RESISTANCE_AT_25C) /
			     NTC_10K_44031_BETA_VALUE) + (1 / NTC_10K_44031_ROOM_TEMP_IN_KELVIN))) - 273.15;
	return temperature;
#else
	return thermistor::convert(resistance, coeff_A, coeff_B, coeff_C);
#endif
}


#ifdef DEFINE_LOOKUP_TABLES
/*!
 * @brief	Convert the thermistor resistance into equivalent temperature using
 *			lookup table for 10K 44031 NTC
 * @param	resistance[in] - thermistor resistance
 * @return	Thermistor temperature value
 */
float ntc_10k_44031rc::lookup(const float resistance)
{
	return thermistor::lookup(lut, resistance, lut_size, lut_offset);
}
#endif
