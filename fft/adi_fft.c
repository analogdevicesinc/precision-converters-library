/***************************************************************************//**
 *   @file    adi_fft.c
 *   @brief   FFT library implementation
********************************************************************************
 * Copyright (c) 2023 Analog Devices, Inc.
 *
 * This software is proprietary to Analog Devices, Inc. and its licensors.
 * By using this software you agree to the terms of the associated
 * Analog Devices Software License Agreement.
*******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include "adi_fft.h"
#include "adi_fft_windowing.h"

/******************************************************************************/
/************************ Macros/Constants ************************************/
/******************************************************************************/

/* Ignoring certain amount of DC bins for noise and other calculations */
#define ADI_FFT_DC_BINS		10

/* Power spread of the fundamental, 10 bins from either side of the fundamental */
#define ADI_FFT_FUND_BINS	10

/* Power spread of the harmonic, 3 bins from either side of the harmonic */
#define ADI_FFT_HARM_BINS	3

/******************************************************************************/
/******************** Variables and User Defined Data Types *******************/
/******************************************************************************/

/* Instance for the floating-point CFFT/CIFFT */
static arm_cfft_instance_f32 cfft_instance;

/******************************************************************************/
/************************ Functions Definitions *******************************/
/******************************************************************************/

/**
 * @brief Initialize the FFT structure
 * @param param[in] - FFT init parameters
 * @param fft_proc[in,out] - FFT processing parameters
 * @param fft_meas[in,out] - FFT measurements parameters
 * @return 0 in case of success, negative error code otherwise
 */
int adi_fft_init(struct adi_fft_init_params *param,
		 struct adi_fft_processing **fft_proc,
		 struct adi_fft_measurements **fft_meas)
{
	struct adi_fft_processing *fft_proc_init;
	struct adi_fft_measurements *fft_meas_init;
	uint8_t cnt;

	if (!param || !fft_proc || !fft_meas)
		return -EINVAL;

	fft_proc_init = (struct adi_fft_processing *)malloc(sizeof(*fft_proc_init));
	if (!fft_proc_init)
		return -ENOMEM;

	fft_meas_init = (struct adi_fft_measurements *)malloc(sizeof(
				*fft_meas_init));
	if (!fft_meas_init) {
		free(fft_proc_init);
		return -ENOMEM;
	}

	fft_proc_init->vref = param->vref;
	fft_proc_init->sample_rate = param->sample_rate;
	fft_proc_init->input_data_full_scale = param->input_data_full_scale;
	fft_proc_init->input_data_zero_scale = param->input_data_zero_scale;
	fft_proc_init->cnv_data_to_volt_without_vref =
		param->convert_data_to_volt_without_vref;
	fft_proc_init->cnv_data_to_volt_wrt_vref = param->convert_data_to_volt_wrt_vref;
	fft_proc_init->cnv_code_to_straight_binary =
		param->convert_code_to_straight_binary;
	fft_proc_init->fft_length = param->samples_count / 2;
	fft_proc_init->window = BLACKMAN_HARRIS_7TERM;
	fft_proc_init->bin_width = 0.0;
	fft_proc_init->fft_done = false;

	*fft_proc = fft_proc_init;

	fft_meas_init->fundamental = 0.0;
	fft_meas_init->pk_spurious_noise = 0.0;
	fft_meas_init->pk_spurious_freq = 0;
	fft_meas_init->THD = 0.0;
	fft_meas_init->SNR = 0.0;
	fft_meas_init->DR = 0.0;
	fft_meas_init->SINAD = 0.0;
	fft_meas_init->SFDR_dbc = 0.0;
	fft_meas_init->SFDR_dbfs = 0.0;
	fft_meas_init->ENOB = 0.0;
	fft_meas_init->RMS_noise = 0.0;
	fft_meas_init->average_bin_noise = 0.0;
	fft_meas_init->max_amplitude = 0.0;
	fft_meas_init->min_amplitude = 0.0;
	fft_meas_init->pk_pk_amplitude = 0.0;
	fft_meas_init->DC = 0.0;
	fft_meas_init->transition_noise = 0.0;
	fft_meas_init->max_amplitude_LSB = 0;
	fft_meas_init->min_amplitude_LSB = 0;
	fft_meas_init->pk_pk_amplitude_LSB = 0;
	fft_meas_init->DC_LSB = 0;
	fft_meas_init->transition_noise_LSB = 0.0;

	for (cnt = 0; cnt < ADI_FFT_NUM_OF_TERMS; cnt++) {
		fft_meas_init->harmonics_mag_dbfs[cnt] = 0.0;
		fft_meas_init->harmonics_freq[cnt] = 0;
		fft_meas_init->harmonics_power[cnt] = 0.0;
	}

	*fft_meas = fft_meas_init;

	return arm_cfft_init_f32(&cfft_instance, fft_proc_init->fft_length);
}

/**
 * @brief Update the FFT parameters
 * @param param[in] - FFT init parameters
 * @param fft_proc[in,out] - FFT entry parameters
 * @return none
 */
int adi_fft_update_params(struct adi_fft_init_params *param,
			  struct adi_fft_processing *fft_proc)
{
	if (!param || !fft_proc)
		return -EINVAL;

	fft_proc->fft_length = param->samples_count / 2;
	fft_proc->sample_rate = param->sample_rate;
	fft_proc->vref = param->vref;

	return arm_cfft_init_f32(&cfft_instance, fft_proc->fft_length);
}

/**
 * @brief Convert dBFS to volts in Pk-Pk
 * @param vref[in] - reference voltage in volts
 * @param value[in] - Input value in dBFS
 * @return voltage
 */
float static adi_fft_dbfs_to_volts(float vref, float value)
{
	return (2 * vref * powf(10.0, value / 20.0));
}

/**
 * @brief Windowing function for FFT
 * @param fft_proc[in,out] - FFT processing parameters
 * @param sum[in,out] - pointer to sum of all the coeffs
 * @return 0 in case of success, negative error code otherwise
 */
static int adi_fft_windowing(struct adi_fft_processing *fft_proc,
			     double *sum)
{
	uint8_t iter;
	uint16_t cnt;
	double term = 0.0;
	const double sample_count = (fft_proc->fft_length * 2) - 1;

	if (!sum || !fft_proc)
		return -EINVAL;

	for (cnt = 0; cnt < fft_proc->fft_length * 2; cnt+=2) {
		switch (fft_proc->window) {
		case BLACKMAN_HARRIS_7TERM:
			if (fft_proc->fft_length <= 2048)
				/* Use precalculated coeficients for first 2048 samples */
				term = adi_fft_7_term_bh_4096[cnt / 2];
			else {
				for (iter = 0; iter < ADI_FFT_NUM_OF_TERMS; iter++)
					term += adi_fft_7_term_bh_coefs[iter] * cos((double)((2.0 * PI * iter *
							(cnt / 2))) / sample_count);
			}
			break;

		case RECTANGULAR:
			/* No window, all terms = 1 */
			term = 1;
			break;

		default:
			return -EINVAL;
		}

		/*  Get sum of all terms, which will be used for amplitude correction */
		*sum += term;

		/* Multiplying each (real) sample by windowing term */
		fft_proc->fft_input[cnt] *= (float)(term);
		term = 0.0;
	}

	return 0;
}

/**
 * @brief Transfer magnitude to dB
 * @param fft_proc[in,out] - FFT processing parameters
 * @param sum[in] - sum of all windowing coeffs
 * @return 0 in case of success, negative error code otherwise
 */
static int adi_fft_magnitude_to_db(struct adi_fft_processing *fft_proc,
				   double sum)
{
	uint16_t cnt;
	float correction;
	float coeff_sum;

	if (!fft_proc)
		return -EINVAL;

	/* Getting sum of coeffs.
	 * If rectangular window is choosen = no windowing, sum of coeffs
	 * is number of samples
	 **/
	if ((fft_proc->fft_length == 2048)
	    && (fft_proc->window == BLACKMAN_HARRIS_7TERM))
		coeff_sum = adi_fft_7_term_bh_4096_sum;
	else {
		if (fft_proc->window == RECTANGULAR)
			coeff_sum = (float)(fft_proc->fft_length * 2.0);
		else {
			if (sum <= 0)
				return -EINVAL;

			coeff_sum = sum;
		}
	}

	for (cnt = 0; cnt < fft_proc->fft_length; cnt++) {
		/* Apply a correction factor
		 * Divide magnigude by a sum of the windowing function coefficients
		 * Multiple by 2 because of power spread over spectrum below and above
		 * the Nyquist frequency
		 **/
		correction = (fft_proc->fft_magnitude[cnt] * 2.0) / coeff_sum;

		/* FFT magnitude with windowing correction */
		fft_proc->fft_magnitude_corrected[cnt] = correction;

		/* Convert to dB without respect to Vref */
		fft_proc->fft_dB[cnt] = 20.0 * (log10f(correction));
	}

	return 0;
}

/**
 * @brief THD calculation with support of harmonics folding
 *        to 1-st nyquist zone
 * @param fft_proc[in] - FFT processing parameters
 * @param fft_meas[in,out] - FFT measurement parameters
 * @return 0 in case of success, negative error code otherwise
 */
static int adi_fft_calculate_thd(struct adi_fft_processing *fft_proc,
				 struct adi_fft_measurements *fft_meas)
{
	const uint16_t first_nyquist_zone = fft_proc->fft_length;
	uint16_t i, j, k = 0, fund_freq = 0, harmonic_position;
	int8_t m, nyquist_zone;
	float mag_helper = -200.0, freq_helper, sum = 0.0, fund_mag = -200.0;
	float fund_pow_bins[21], harm_pow_bins[5][ADI_FFT_NUM_OF_TERMS];

	if (!fft_proc || !fft_meas)
		return -EINVAL;

	/* Looking for the fundamental frequency and amplitude */
	for (i = ADI_FFT_DC_BINS; i < fft_proc->fft_length; i++) {
		/* Not counting DC bins */
		if (fft_proc->fft_dB[i] > fund_mag) {
			fund_mag = fft_proc->fft_dB[i];
			fund_freq = i;
		}
	}

	/* Get first harmonic measurements */
	fft_meas->harmonics_freq[0] = fund_freq;
	fft_meas->harmonics_mag_dbfs[0] = fund_mag;
	fft_meas->fundamental = adi_fft_dbfs_to_volts(fft_proc->vref,
				fund_mag);

	/* Get remaining harmonic measurements */
	for (i = 1; i < ADI_FFT_NUM_OF_TERMS - 1; i++) {
		if ((fft_meas->harmonics_freq[0] * (i + 1)) < first_nyquist_zone)
			/* Checking if 2nd harmonic is outside of the first nyquist zone */
			harmonic_position = fft_meas->harmonics_freq[0] * (i + 1);
		else {
			/* Determine the nyquist zone */
			nyquist_zone = 1 + (fft_meas->harmonics_freq[0] * (i + 1) /
					    first_nyquist_zone);

			if (nyquist_zone % 2)
				/* Odd nyquist zones: 3, 5, 7... */
				harmonic_position = first_nyquist_zone - (first_nyquist_zone * nyquist_zone -
						    fft_meas->harmonics_freq[0] * (i + 1));
			else
				/* Even nyquist zones: 2, 4, 6... */
				harmonic_position = first_nyquist_zone * nyquist_zone -
						    fft_meas->harmonics_freq[0] * (i + 1);
		}

		/* Extend searching range by 3 bins around expected position of the harmonic */
		for (m = -ADI_FFT_HARM_BINS ; m <= ADI_FFT_HARM_BINS ; m++) {
			if (fft_proc->fft_dB[harmonic_position + m] > mag_helper) {
				mag_helper = fft_proc->fft_dB[harmonic_position + m];
				freq_helper = (harmonic_position + m);
			}
		}

		fft_meas->harmonics_freq[i] = freq_helper;
		fft_meas->harmonics_mag_dbfs[i]  = mag_helper;
		mag_helper = -200.0;
	}

	/* Power leakage of the fundamental */
	for (i = fft_meas->harmonics_freq[0] - ADI_FFT_FUND_BINS;
	     i <= fft_meas->harmonics_freq[0] + ADI_FFT_FUND_BINS; i++) {
		sum += powf(((fft_proc->fft_magnitude_corrected[i] / (2.0*sqrt(2)))), 2.0);
		fund_pow_bins[k] = fft_proc->fft_magnitude_corrected[i];
		k++;
	}

	/* Finishing the RSS of power-leaked fundamental */
	sum = sqrt(sum);
	fft_meas->harmonics_power[0] = sum * 2.0 * sqrt(2);
	sum = 0.0;
	k = 0;

	/* Power leakage of the harmonics */
	for (j = 1; j < ADI_FFT_NUM_OF_TERMS - 1; j++) {
		for (i = fft_meas->harmonics_freq[j] - ADI_FFT_HARM_BINS;
		     i <= fft_meas->harmonics_freq[j] + ADI_FFT_HARM_BINS; i++) {
			sum += powf(((fft_proc->fft_magnitude_corrected[i] / (2.0*sqrt(2)))), 2.0);
			harm_pow_bins[j - 1][k] = fft_proc->fft_magnitude_corrected[i];
			k++;
		}

		/* Finishing the RSS of power-leaked harmonics */
		k = 0;
		sum = sqrt(sum);
		fft_meas->harmonics_power[j] = sum * 2.0 * sqrt(2);
		sum = 0.0;
	}

	/* The THD formula */
	fft_meas->THD = sqrtf(powf(fft_meas->harmonics_power[1],
				   2.0) + powf(fft_meas->harmonics_power[2],
					       2.0) + powf(fft_meas->harmonics_power[3],
							       2.0) + powf(fft_meas->harmonics_power[4],
									       2.0) + powf(fft_meas->harmonics_power[5], 2.0)) / fft_meas->harmonics_power[0];

	/* Back from volts to dB */
	fft_meas->THD = 20.0 * log10f(fft_meas->THD);

	return 0;
}

/**
 * @brief Calculate amplitudes: min, max, pk-pk amplitude and DC part
 * @param fft_proc[in,out] - FFT processing parameters
 * @param fft_meas[in,out] - FFT measurement parameters
 * @return 0 in case of success, negative error code otherwise
 */
static int adi_fft_waveform_stat(struct adi_fft_processing *fft_proc,
				 struct adi_fft_measurements *fft_meas)
{
	uint16_t cnt;
	int16_t max_position, min_position;
	int32_t max = -fft_proc->input_data_zero_scale;
	int32_t min = fft_proc->input_data_zero_scale;
	int32_t offset_correction;
	int64_t sum = 0;
	double deviation = 0.0, mean;

	if (!fft_proc || !fft_meas)
		return -EINVAL;

	/* Sum of all coeffs, to find the Mean value */
	for (cnt = 0; cnt < fft_proc->fft_length * 2; cnt++)
		sum += fft_proc->input_data[cnt];

	/* Calculating mean value = DC offset */
	mean = (sum / (fft_proc->fft_length * 2));

	/* DC part in LSBs */
	fft_meas->DC_LSB = (int32_t)(mean) + fft_proc->input_data_zero_scale;
	offset_correction = (int32_t)(mean);

	/* Find Min, Max amplitudes + Deviation */
	for (cnt = 0; cnt < fft_proc->fft_length * 2;
	     cnt++) {
		/* Calculating the Deviation for Transition noise */
		deviation += pow(fft_proc->input_data[cnt] - mean, 2.0);

		/* Looking for MAX value */
		if (fft_proc->input_data[cnt] > max) {
			max = fft_proc->input_data[cnt];
			max_position = cnt;
		}

		/* Looking for MIN value */
		if (fft_proc->input_data[cnt] < min) {
			min = fft_proc->input_data[cnt];
			min_position = cnt;
		}
	}

	/* Amplitudes in Volts */
	fft_meas->max_amplitude = fft_proc->cnv_data_to_volt_wrt_vref(
					  fft_proc->input_data[max_position], 0);
	fft_meas->min_amplitude = fft_proc->cnv_data_to_volt_wrt_vref(
					  fft_proc->input_data[min_position], 0);
	fft_meas->pk_pk_amplitude = fft_meas->max_amplitude - fft_meas->min_amplitude;
	fft_meas->DC = (2.0 * fft_proc->vref * ((float)(((int32_t)(
			fft_meas->DC_LSB) - fft_proc->input_data_zero_scale)))) /
		       fft_proc->input_data_full_scale;

	/* Amplitudes in LSBs */
	fft_meas->max_amplitude_LSB = fft_proc->input_data[max_position] +
				      fft_proc->input_data_zero_scale;
	fft_meas->min_amplitude_LSB = fft_proc->input_data[min_position] +
				      fft_proc->input_data_zero_scale;
	fft_meas->pk_pk_amplitude_LSB = fft_meas->max_amplitude_LSB -
					fft_meas->min_amplitude_LSB;

	/* Transition noise */
	deviation = (sqrt(deviation / (fft_proc->fft_length * 2.0)));
	fft_meas->transition_noise_LSB = (uint32_t)(deviation);
	fft_meas->transition_noise = (2.0 * fft_proc->vref *
				      fft_meas->transition_noise_LSB) / fft_proc->input_data_full_scale;

	/* RMS noise */
	fft_meas->RMS_noise = fft_meas->transition_noise;

	/* Applying mean value to each sample = removing DC offset */
	for (cnt = 0; cnt < fft_proc->fft_length * 2; cnt++)
		fft_proc->input_data[cnt] -= offset_correction;

	return 0;
}

/**
 * @brief Calculate noise from the FFT plot
 * @param fft_proc[in,out] - FFT processing parameters
 * @param fft_meas[in,out] - FFT measurement parameters
 * @return 0 in case of success, negative error code otherwise
 */
static int adi_fft_calculate_noise(struct adi_fft_processing *fft_proc,
				   struct adi_fft_measurements *fft_meas)
{
	/* Magic constant from the LabView FFT core correcting
	 * only dynamic range */
	const float LW_DR_correction_const = 4.48;
	uint16_t cnt;
	float biggest_spur = -300;
	double RSS = 0.0, mean = 0.0;

	if (!fft_proc || !fft_meas)
		return -EINVAL;

	/* Initalizing pk_spurious variables */
	fft_meas->pk_spurious_noise = -200.0;
	fft_meas->pk_spurious_freq = 0;

	for (cnt = 0; cnt < ADI_FFT_DC_BINS; cnt++)
		/* Ignoring DC bins */
		fft_proc->noise_bins[cnt] = 0.0;

	for (cnt = ADI_FFT_DC_BINS; cnt < fft_proc->fft_length; cnt++) {
		/* Ignoring spread near the fundamental */
		if ((cnt <= fft_meas->harmonics_freq[0] + ADI_FFT_FUND_BINS)
		    && (cnt >= fft_meas->harmonics_freq[0] - ADI_FFT_FUND_BINS))
			fft_proc->noise_bins[cnt] = 0.0;
		/* Ignoring spread near harmonics */
		else if ((cnt <= fft_meas->harmonics_freq[1] + ADI_FFT_HARM_BINS)
			 && (cnt >= fft_meas->harmonics_freq[1] - ADI_FFT_HARM_BINS))
			fft_proc->noise_bins[cnt] = 0.0;
		else if ((cnt <= fft_meas->harmonics_freq[2] + ADI_FFT_HARM_BINS)
			 && (cnt >= fft_meas->harmonics_freq[2] - ADI_FFT_HARM_BINS))
			fft_proc->noise_bins[cnt] = 0.0;
		else if ((cnt <= fft_meas->harmonics_freq[3] + ADI_FFT_HARM_BINS)
			 && (cnt >= fft_meas->harmonics_freq[3] - ADI_FFT_HARM_BINS))
			fft_proc->noise_bins[cnt] = 0.0;
		else if ((cnt <= fft_meas->harmonics_freq[4] + ADI_FFT_HARM_BINS)
			 && (cnt >= fft_meas->harmonics_freq[4] - ADI_FFT_HARM_BINS))
			fft_proc->noise_bins[cnt] = 0.0;
		else if ((cnt <= fft_meas->harmonics_freq[5] + ADI_FFT_HARM_BINS)
			 && (cnt >= fft_meas->harmonics_freq[5] - ADI_FFT_HARM_BINS))
			fft_proc->noise_bins[cnt] = 0.0;
		else {
			/* Root Sum Square = RSS for noise calculations */
			fft_proc->noise_bins[cnt] = fft_proc->fft_magnitude_corrected[cnt];
			RSS += pow(((double)(fft_proc->fft_magnitude_corrected[cnt] / (2.0*sqrt(2)))),
				   2.0);

			/* Average bin noise */
			mean += fft_proc->fft_magnitude_corrected[cnt];

			/* Peak spurious amplitude */
			if (fft_proc->fft_magnitude_corrected[cnt] > fft_meas->pk_spurious_noise) {
				fft_meas->pk_spurious_noise = fft_proc->fft_magnitude_corrected[cnt];
				fft_meas->pk_spurious_freq = cnt;
			}
		}
	}

	mean /= (double)(fft_proc->fft_length);

	/* RSS of FFT spectrum without DC, Fundamental and Harmonics */
	RSS = sqrt(RSS);
	RSS = RSS * 2.0 * sqrt(2);

	/* Peak spurious amplitude = Highest amplitude excluding DC,
	 * the Fundamental and the Harmonics */
	fft_meas->pk_spurious_noise = 20.0 * log10f(1.0 / fft_meas->pk_spurious_noise);

	/* Looking for the biggest spur among harmonics */
	for (cnt = 1 ; cnt < 6; cnt++) {
		if (fft_meas->harmonics_mag_dbfs[cnt] > biggest_spur)
			biggest_spur = fft_meas->harmonics_mag_dbfs[cnt];
	}

	/* Looking for the biggest spur among harmonics and pk_spurious_noise */
	if (biggest_spur > fft_meas->pk_spurious_noise)
		biggest_spur = fft_meas->pk_spurious_noise;

	/* Spurious Free Dynamic Range SFDR related to the carrer =
	 * biggest spur - the Fundamental, [dBc] - Decibels related to the carrier */
	fft_meas->SFDR_dbc = biggest_spur - fft_meas->harmonics_mag_dbfs[0];

	/* Spurious Free Dynamic Range SFDR related to the full-scale =
	 * biggest spur - full-scale [dBFS], where full-scale is 0 dBFS */
	fft_meas->SFDR_dbfs = biggest_spur;

	/* Average bin noise = Mean value of FFT spectrum excluding DC,
	 * the Fundamental and the Harmonics */
	fft_meas->average_bin_noise = (float)(20.0 * log10(mean));

	/* DR = 1 / RSS of FFT spectrum without DC, Fund. and
	 * Harmonics + Magic constant from the Labview FFT core */
	fft_meas->DR = (20.0 * log10f(1.0 / (float)(RSS))) + LW_DR_correction_const;

	/* SNR = Power of the fundamental / RSS of FFT spectrum without DC,
	 * Fundamental and Harmonics */
	fft_meas->SNR = 20.0 * log10f((fft_meas->harmonics_power[0]) / (RSS));

	/* SINAD */
	fft_meas->SINAD = -10.0 * log10f(powf(10.0,
					      (fabs(fft_meas->SNR))*(-1.0) / 10.0) + powf(10.0,
							      fabs(fft_meas->THD)*(-1.0) / 10.0));

	/* ENOB - Effective number of bits */
	fft_meas->ENOB = (fft_meas->SINAD - 1.67 + fabs(
				  fft_meas->harmonics_mag_dbfs[0])) / 6.02;

	return 0;
}

/**
 * @brief Perform the FFT
 * @param fft_proc[in,out] - FFT processing parameters
 * @param fft_meas[in,out] - FFT measurements parameters
 * @return 0 in case of success, negative error code otherwise
 */
int adi_fft_perform(struct adi_fft_processing *fft_proc,
		    struct adi_fft_measurements *fft_meas)
{
	int ret;
	uint32_t cnt;
	uint32_t sample_cnt;
	double coeffs_sum = 0.0;

	if (!fft_proc || !fft_meas)
		return -EINVAL;

	fft_proc->fft_done = false;
	fft_proc->bin_width = (float)(fft_proc->sample_rate) / ((float)
			      fft_proc->fft_length * 2);

	/* Perform DC characterization */
	ret = adi_fft_waveform_stat(fft_proc, fft_meas);
	if (ret)
		return ret;

	/* Convert codes without DC offset to "volts" without respect to Vref voltage */
	sample_cnt = 0;
	for (cnt = 0; cnt < fft_proc->fft_length * 2; cnt+=2) {
		/* Real part */
		fft_proc->fft_input[cnt] = fft_proc->cnv_data_to_volt_without_vref(
						   fft_proc->input_data[sample_cnt], 0);

		/* Imaginary part (always zero for complex FFT) */
		fft_proc->fft_input[cnt+1] = 0;

		sample_cnt++;
	}

	/* Apply windowing */
	ret = adi_fft_windowing(fft_proc, &coeffs_sum);
	if (ret)
		return ret;

	/* Perform the FFT through CMSIS-DSP support libraries */
	arm_cfft_f32(&cfft_instance, fft_proc->fft_input, 0, 1);

	/* Transform from complex FFT to magnitude */
	arm_cmplx_mag_f32(fft_proc->fft_input, fft_proc->fft_magnitude,
			  fft_proc->fft_length);

	/* Perform AC characterization */
	ret = adi_fft_magnitude_to_db(fft_proc, coeffs_sum);
	if (ret)
		return ret;

	ret = adi_fft_calculate_thd(fft_proc, fft_meas);
	if (ret)
		return ret;

	ret = adi_fft_calculate_noise(fft_proc, fft_meas);
	if (ret)
		return ret;

	fft_proc->fft_done = true;

	return 0;
}
