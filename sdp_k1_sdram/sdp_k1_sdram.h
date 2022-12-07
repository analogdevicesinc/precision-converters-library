/***************************************************************************//**
 *   @file    sdp_k1_sdram.h
 *   @brief   SDP-K1 SDRAM functionality header
********************************************************************************
 * Copyright (c) 2022 Analog Devices, Inc.
 * All rights reserved.
 *
 * This software is proprietary to Analog Devices, Inc. and its licensors.
 * By using this software you agree to the terms of the associated
 * Analog Devices Software License Agreement.
*******************************************************************************/

#ifndef __SDPK1_SDRAM_H
#define __SDPK1_SDRAM_H

#if defined (TARGET_SDP_K1)

#ifdef __cplusplus
 extern "C" {
#endif

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/

#include "stm32f4xx_hal.h"

/******************************************************************************/
/********************** Macros and Constants Definition ***********************/
/******************************************************************************/

/**
  * @brief  SDRAM status structure definition
  */
#define   SDRAM_OK         ((uint8_t)0x00)
#define   SDRAM_ERROR      ((uint8_t)0x01)

#define SDRAM_DEVICE_ADDR  ((uint32_t)0xC0000000)
#define SDRAM_DEVICE_SIZE  ((uint32_t)0x1000000)  /* SDRAM device size in MBytes */

/* #define SDRAM_MEMORY_WIDTH            FMC_SDRAM_MEM_BUS_WIDTH_8  */
/* #define SDRAM_MEMORY_WIDTH            FMC_SDRAM_MEM_BUS_WIDTH_16 */
#define SDRAM_MEMORY_WIDTH               FMC_SDRAM_MEM_BUS_WIDTH_32

#define SDCLOCK_PERIOD                   FMC_SDRAM_CLOCK_PERIOD_2
/* #define SDCLOCK_PERIOD                FMC_SDRAM_CLOCK_PERIOD_3 */

#define REFRESH_COUNT                    ((uint32_t)0x0569)   /* SDRAM refresh counter (90Mhz SD clock) */

#define SDRAM_TIMEOUT                    ((uint32_t)0xFFFF)

/* DMA definitions for SDRAM DMA transfer */
#define __DMAx_CLK_ENABLE                 __HAL_RCC_DMA2_CLK_ENABLE
#define __DMAx_CLK_DISABLE                __HAL_RCC_DMA2_CLK_DISABLE
#define SDRAM_DMAx_CHANNEL                DMA_CHANNEL_0
#define SDRAM_DMAx_STREAM                 DMA2_Stream0
#define SDRAM_DMAx_IRQn                   DMA2_Stream0_IRQn
#define BSP_SDRAM_DMA_IRQHandler          DMA2_Stream0_IRQHandler

/* SDRAM register defines */
#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)

/* SDP SDRAM Pin defines */
#define SDRAM_A0        GPIO_PIN_0
#define SDRAM_A1        GPIO_PIN_1
#define SDRAM_A2        GPIO_PIN_2
#define SDRAM_A3        GPIO_PIN_3
#define SDRAM_A4        GPIO_PIN_4
#define SDRAM_A5        GPIO_PIN_5
#define SDRAM_A6        GPIO_PIN_12
#define SDRAM_A7        GPIO_PIN_13
#define SDRAM_A8        GPIO_PIN_14
#define SDRAM_A9        GPIO_PIN_15
#define SDRAM_A10       GPIO_PIN_0
#define SDRAM_A11       GPIO_PIN_1
#define SDRAM_A12       GPIO_PIN_2
#define SDRAM_A13       GPIO_PIN_3
#define SDRAM_A14       GPIO_PIN_4
#define SDRAM_A15       GPIO_PIN_5
#define SDRAM_D0        GPIO_PIN_14
#define SDRAM_D1        GPIO_PIN_15
#define SDRAM_D2        GPIO_PIN_0
#define SDRAM_D3        GPIO_PIN_1
#define SDRAM_D4        GPIO_PIN_7
#define SDRAM_D5        GPIO_PIN_8
#define SDRAM_D6        GPIO_PIN_9
#define SDRAM_D7        GPIO_PIN_10
#define SDRAM_D8        GPIO_PIN_11
#define SDRAM_D9        GPIO_PIN_12
#define SDRAM_D10       GPIO_PIN_13
#define SDRAM_D11       GPIO_PIN_14
#define SDRAM_D12       GPIO_PIN_15
#define SDRAM_D13       GPIO_PIN_8
#define SDRAM_D14       GPIO_PIN_9
#define SDRAM_D15       GPIO_PIN_10
#define SDRAM_D16       GPIO_PIN_8
#define SDRAM_D17       GPIO_PIN_9
#define SDRAM_D18       GPIO_PIN_10
#define SDRAM_D19       GPIO_PIN_11
#define SDRAM_D20       GPIO_PIN_12
#define SDRAM_D21       GPIO_PIN_13
#define SDRAM_D22       GPIO_PIN_14
#define SDRAM_D23       GPIO_PIN_15
#define SDRAM_D24       GPIO_PIN_0
#define SDRAM_D25       GPIO_PIN_1
#define SDRAM_D26       GPIO_PIN_2
#define SDRAM_D27       GPIO_PIN_3
#define SDRAM_D28       GPIO_PIN_6
#define SDRAM_D29       GPIO_PIN_7
#define SDRAM_D30       GPIO_PIN_9
#define SDRAM_D31       GPIO_PIN_10
#define SDRAM_NBL0      GPIO_PIN_0
#define SDRAM_NBL1      GPIO_PIN_1
#define SDRAM_NBL2      GPIO_PIN_4
#define SDRAM_NBL3      GPIO_PIN_5
#define SDRAM_SDCLK     GPIO_PIN_8
#define SDRAM_N_CAS     GPIO_PIN_15
#define SDRAM_N_RAS     GPIO_PIN_11
#define SDRAM_SDCKE0    GPIO_PIN_2
#define SDRAM_SDNE0     GPIO_PIN_3
#define SDRAM_N_WE      GPIO_PIN_5

/******************************************************************************/
/************************ Public Declarations *********************************/
/******************************************************************************/

uint8_t SDP_SDRAM_Init(void);
uint8_t SDP_SDRAM_DeInit(void);
void    SDP_SDRAM_Initialization_sequence(uint32_t RefreshCount);
uint8_t SDP_SDRAM_ReadData_8b(uint32_t pAddress, uint8_t *pData, uint32_t dataSize);
uint8_t SDP_SDRAM_ReadData_16b(uint32_t pAddress, uint16_t *pData, uint32_t dataSize);
uint8_t SDP_SDRAM_ReadData_32b(uint32_t pAddress, uint32_t *pData, uint32_t dataSize);
uint8_t SDP_SDRAM_ReadData_DMA(uint32_t pAddress, uint32_t *pData, uint32_t dataSize);
uint8_t SDP_SDRAM_WriteData_8b(uint32_t pAddress, uint8_t *pData, uint32_t dataSize);
uint8_t SDP_SDRAM_WriteData_16b(uint32_t pAddress, uint16_t *pData, uint32_t dataSize);
uint8_t SDP_SDRAM_WriteData_32b(uint32_t pAddress, uint32_t *pData, uint32_t dataSize);
uint8_t SDP_SDRAM_WriteData_DMA(uint32_t pAddress, uint32_t *pData, uint32_t dataSize);
uint8_t SDP_SDRAM_Sendcmd(FMC_SDRAM_CommandTypeDef *SdramCmd);

/* These functions can be modified in case the current settings (e.g. DMA stream)
   need to be changed for specific application needs */
void    SDP_SDRAM_MspInit(SDRAM_HandleTypeDef  *hsdram, void *Params);
void    SDP_SDRAM_MspDeInit(SDRAM_HandleTypeDef  *hsdram, void *Params);

#ifdef __cplusplus
}
#endif

#endif	// TARGET_SDP_K1
#endif	// __SDPK1_SDRAM_H
