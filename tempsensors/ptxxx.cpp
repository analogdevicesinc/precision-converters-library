/*!
 *****************************************************************************
  @file:  ptxxx.cpp

  @brief:

  @details:
 -----------------------------------------------------------------------------
 Copyright (c) 2018, 2020 Analog Devices, Inc.  All rights reserved.

 This software is proprietary to Analog Devices, Inc. and its licensors.
 By using this software you agree to the terms of the associated
 Analog Devices Software License Agreement.

*****************************************************************************/

#include <math.h>
#include "ptxxx.h"

#define PT1000_RESISTANCE_TO_TEMP(x) ((x-1000.0)/(0.385))


// Forward function declarations
float convertPT1000ToTemperature(const float resistance);

//class member methods

float PT100::convertResistanceToTemperature(float resistance)
{

	return ( convertPT1000ToTemperature(resistance * 10) );
}


float PT1000::convertResistanceToTemperature(float resistance)
{

	return ( convertPT1000ToTemperature(resistance) );
}

float convertPT1000ToTemperature(const float resistance)
{
	float temperature = 25.0;

#ifdef USE_LINEAR_RTD_TEMP_EQ
	temperature = PT1000_RESISTANCE_TO_TEMP(resistance);
#else

#define A (3.9083*pow(10,-3))
#define B (-5.775*pow(10,-7))
	/*if(resistance < 100.0)
	    temperature = -242.02 + 2.228 * resistance + (2.5859 * pow(10, -3)) * pow(resistance, 2) - (48260 * pow(10, -6)) * pow(resistance, 3) - (2.8183 * pow(10, -3)) * pow(resistance, 4) + (1.5243 * pow(10, -10)) * pow(resistance, 5);
	else*/
	temperature = ((-A + sqrt(double(pow(A,
					     2) - 4 * B * (1 - resistance / 1000.0))) ) / (2 * B));
#endif
	return temperature;
}






