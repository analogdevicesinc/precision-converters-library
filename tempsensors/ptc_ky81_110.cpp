/*!
 *****************************************************************************
  @file:  ptc_ky81_110.cpp

  @brief: This file contains functionality for PTC KY81/110 model

  @details:
 -----------------------------------------------------------------------------
 Copyright (c) 2021 Analog Devices, Inc.  All rights reserved.

 This software is proprietary to Analog Devices, Inc. and its licensors.
 By using this software you agree to the terms of the associated
 Analog Devices Software License Agreement.

*****************************************************************************/

#include "thermistor.h"
#include "ptc_ky81_110.h"

#ifdef DEFINE_LOOKUP_TABLES
/* PTC look-up table. Values are resistance in ohm for temperature
 * range from -10 to 80C with +/-1C tolerance.
 * @note This table is derived based on the look-up table specified
 *		 in the datasheet of KY81/110 part. The linear interpolation
 *		 has been used to obtain 1C step size.
**/
const uint32_t ptc_ky81_110::lut[] = {
	747, 753, 760, 767, 774, 781, 787, 794, 801, 808, 815, 822, 829, 836,
	843, 850, 857, 864, 871, 878, 886, 893, 901, 908, 916, 923, 931, 938,
	946, 953, 961, 968, 976, 984, 992, 1000, 1008, 1016, 1024, 1032, 1040,
	1048, 1056, 1064, 1072, 1081, 1089, 1097, 1105, 1113, 1122, 1130, 1139,
	1148, 1156, 1165, 1174, 1182, 1191, 1200, 1209, 1218, 1227, 1236, 1245,
	1254, 1263, 1272, 1281, 1290, 1299, 1308, 1317, 1326, 1336, 1345, 1354,
	1364, 1373, 1382, 1392, 1401, 1411, 1421, 1431, 1441, 1450, 1460, 1470,
	1480, 1490
};
#endif


/*!
 * @brief	This is a constructor for ptc_ky81_110 class
 * @return	none
 */
ptc_ky81_110::ptc_ky81_110()
{
	temperature_coeff = 0.79;		/* Temperature coefficient for KY81/110 */

#ifdef DEFINE_LOOKUP_TABLES
	ptc_ky81_110::lut_offset = -10; /* Min temperature obtained through LUT */
	ptc_ky81_110::lut_size = 90;	/* Temperature range defined in LUT
									   [lut_offset : lut_size - lut_offset] */
#endif
}


/*!
 * @brief	Convert the thermistor resistance into equivalent temperature
 * @param	resistance[in] - thermistor resistance
 * @return	Thermistor temperature value
 * @note	Resistance at room temperature is 1000 ohms (1K)
 */
float ptc_ky81_110::convert(const float resistance)
{
	return (((resistance - 1000) / 1000) * (100.0 / temperature_coeff)) + 25.0;
}


#ifdef DEFINE_LOOKUP_TABLES
/*!
 * @brief	Convert the thermistor resistance into equivalent temperature using
 *			lookup table for KY81/110 NTC
 * @param	resistance[in] - thermistor resistance
 * @return	Thermistor temperature value
 */
float ptc_ky81_110::lookup(const float resistance)
{
	return thermistor::lookup(lut, resistance, lut_size, lut_offset);
}
#endif
