/*!
 *****************************************************************************
  @file:  rtd.h

  @brief:

  @details:
 -----------------------------------------------------------------------------
 Copyright (c) 2018, 2020 Analog Devices, Inc.  All rights reserved.

 This software is proprietary to Analog Devices, Inc. and its licensors.
 By using this software you agree to the terms of the associated
 Analog Devices Software License Agreement.

*****************************************************************************/


#ifndef RTD_H_
#define RTD_H_

class RTD
{
public:
	/**
	 * @brief converts a resistance to a temperature
	 *
	 * @param [in]	resistance - the resistance of the temperature sensor
	 *
	 * @return temperature
	 */
	virtual float convertResistanceToTemperature(const float resistance) = 0;
};

#endif
