/*************************************************************************//**
 *   @file   adi_fft.h
 *   @brief  FFT library implementation headers
******************************************************************************
* Copyright (c) 2023 Analog Devices, Inc.
* All rights reserved.
*
* This software is proprietary to Analog Devices, Inc. and its licensors.
* By using this software you agree to the terms of the associated
* Analog Devices Software License Agreement.
*****************************************************************************/

#ifndef _ADI_FFT_H_
#define _ADI_FFT_H_

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include "adi_fft_windowing.h"

/******************************************************************************/
/************************ Macros/Constants ************************************/
/******************************************************************************/

/* Maximum number of default samples used for FFT analysis (<=2048) */
#if !defined(ADI_FFT_MAX_SAMPLES)
#define ADI_FFT_MAX_SAMPLES		2048
#endif

/******************************************************************************/
/************************ Public Declarations *********************************/
/******************************************************************************/

typedef float(*adi_fft_data_to_volt_conv)(int32_t, uint8_t);
typedef int32_t(*adi_fft_code_to_straight_bin_conv)(uint32_t, uint8_t);

/* FFT windowing type */
enum adi_fft_windowing_type {
	BLACKMAN_HARRIS_7TERM,
	RECTANGULAR
};

/* FFT init parameters specific to device */
struct adi_fft_init_params {
	/* Device reference voltage */
	float vref;
	/* Device sample rate */
	uint32_t sample_rate;
	/* Samples count */
	uint16_t samples_count;
	/* Input data full scale value */
	int32_t input_data_full_scale;
	/* Input data zero scale value */
	int32_t input_data_zero_scale;
	/* Convert input data to voltage without respect to vref */
	adi_fft_data_to_volt_conv convert_data_to_volt_without_vref;
	/* Convert input data to voltage with respect to vref */
	adi_fft_data_to_volt_conv convert_data_to_volt_wrt_vref;
	/* Convert code to straight binary data */
	adi_fft_code_to_straight_bin_conv convert_code_to_straight_binary;
};

/* FFT processing parameters */
struct adi_fft_processing {
	/* Device reference voltage */
	float vref;
	/* Device sample rate */
	uint32_t sample_rate;
	/* Input data full scale value */
	int32_t input_data_full_scale;
	/* Input data zero scale value */
	int32_t input_data_zero_scale;
	/* Convert input data to voltage without respect to vref */
	adi_fft_data_to_volt_conv cnv_data_to_volt_without_vref;
	/* Convert input data to voltage with respect to vref */
	adi_fft_data_to_volt_conv cnv_data_to_volt_wrt_vref;
	/* Convert code to straight binary data */
	adi_fft_code_to_straight_bin_conv cnv_code_to_straight_binary;
	/* FFT length (samples_count / 2) */
	uint16_t fft_length;
	/* FFT bin width */
	float bin_width;
	/* Input data (unformatted/straight binary for ADCs) */
	int32_t input_data[ADI_FFT_MAX_SAMPLES];
	/* Maximum length of FFT magnitude */
	float fft_magnitude[ADI_FFT_MAX_SAMPLES / 2];
	/* Magnitude with windowing correction */
	float fft_magnitude_corrected[ADI_FFT_MAX_SAMPLES / 2];
	/* FFT effective gain */
	float fft_dB[ADI_FFT_MAX_SAMPLES / 2];
	/* Maximum length of FFT input array supporred - Real + Imaginary components */
	float fft_input[ADI_FFT_MAX_SAMPLES];
	/* FFT bins excluding DC, fundamental and Harmonics */
	float noise_bins[ADI_FFT_MAX_SAMPLES / 2];
	/* FFT window type */
	enum adi_fft_windowing_type window;
	/* FFT done status */
	bool fft_done;
};

/* FFT meausurement parameters */
struct adi_fft_measurements {
	/* Harmonics, including their power leakage */
	float harmonics_power[ADI_FFT_NUM_OF_TERMS];
	/* Harmonic magnitudes for THD */
	float harmonics_mag_dbfs[ADI_FFT_NUM_OF_TERMS];
	/* Harmonic frequencies for THD */
	uint16_t harmonics_freq[ADI_FFT_NUM_OF_TERMS];
	/* Fundamental in volts */
	float fundamental;
	/* Peak spurious noise (amplitude) */
	float pk_spurious_noise;
	/* Peak Spurious Frequency */
	uint16_t pk_spurious_freq;
	/* Total Harmonic Distortion */
	float THD;
	/* Signal to Noise Ratio */
	float SNR;
	/* Dynamic Range */
	float DR;
	/* Signal to Noise And Distortion ratio */
	float SINAD;
	/* Spurious Free Dynamic Range, dBc */
	float SFDR_dbc;
	/* Spurious Free Dynamic Range, dbFS */
	float SFDR_dbfs;
	/* ENOB - Effective Number Of Bits */
	float ENOB;
	/* RMS noise */
	float RMS_noise;
	float average_bin_noise;
	/* Maximum amplitude in volts */
	float max_amplitude;
	/* Minimum amplitude in volts */
	float min_amplitude;
	/* Peak to Peak amplitude in volts */
	float pk_pk_amplitude;
	/* DC bias in volts */
	float DC;
	/* Transition noise */
	float transition_noise;
	/* Maximum amplitude in LSB */
	uint32_t max_amplitude_LSB;
	/* Minimum amplitude in LSB */
	uint32_t min_amplitude_LSB;
	/* Peak to Peak amplitude in LSB */
	uint32_t pk_pk_amplitude_LSB;
	/* DC bias in LSB */
	int32_t DC_LSB;
	/* Transition noise in LSB */
	float transition_noise_LSB;
};

int adi_fft_init(struct adi_fft_init_params *param,
		 struct adi_fft_processing *fft_proc,
		 struct adi_fft_measurements *fft_meas);
int adi_fft_update_params(struct adi_fft_init_params *param,
			  struct adi_fft_processing *fft_proc);
int adi_fft_perform(struct adi_fft_processing *fft_proc,
		    struct adi_fft_measurements *fft_meas);

#endif // !_ADI_FFT_H_
