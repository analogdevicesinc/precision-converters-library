/***************************************************************************//**
 *   @file    adi_fft_windowing.h
 *   @brief   FFT windowing functionality headers
********************************************************************************
 * Copyright (c) 2023 Analog Devices, Inc.
 *
 * This software is proprietary to Analog Devices, Inc. and its licensors.
 * By using this software you agree to the terms of the associated
 * Analog Devices Software License Agreement.
*******************************************************************************/

#ifndef _ADI_FFT_WINDOWING_H_
#define _ADI_FFT_WINDOWING_H_

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/

#include <arm_math.h>

/******************************************************************************/
/************************ Macros/Constants ************************************/
/******************************************************************************/

/* Number of terms for blackman harris 7 term FFT */
#define ADI_FFT_NUM_OF_TERMS		7

/******************************************************************************/
/************************ Public Declarations *********************************/
/******************************************************************************/

extern const double adi_fft_7_term_bh_coefs[ADI_FFT_NUM_OF_TERMS];
extern const double adi_fft_7_term_bh_4096_sum;
extern const float32_t adi_fft_7_term_bh_4096[4096];

#endif	// !_ADI_FFT_WINDOWING_H_
