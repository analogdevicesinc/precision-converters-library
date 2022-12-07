/*!
 *****************************************************************************
  @file:  ptxxx.h

  @brief:

  @details:
 -----------------------------------------------------------------------------
 Copyright (c) 2018, 2020 Analog Devices, Inc.  All rights reserved.

 This software is proprietary to Analog Devices, Inc. and its licensors.
 By using this software you agree to the terms of the associated
 Analog Devices Software License Agreement.

*****************************************************************************/


#ifndef RTD_PTXXX_H_
#define RTD_PTXXX_H_

#include <math.h>

#include "rtd.h"

class PT100 : public RTD
{
public:
	float convertResistanceToTemperature(float resistance);
};

class PT1000 : public RTD
{
public:
	float convertResistanceToTemperature(float resistance);
};

#endif /* RTD_PTXXX_H_ */
