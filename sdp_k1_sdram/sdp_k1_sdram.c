/***************************************************************************//**
 *   @file    sdp_k1_sdram.c
 *   @brief   SDP-K1 SDRAM functionality
********************************************************************************
 * Copyright (c) 2022 Analog Devices, Inc.
 * All rights reserved.
 *
 * This software is proprietary to Analog Devices, Inc. and its licensors.
 * By using this software you agree to the terms of the associated
 * Analog Devices Software License Agreement.
*******************************************************************************/

#if defined (TARGET_SDP_K1)

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/

#include "sdp_k1_sdram.h"

/******************************************************************************/
/******************** Variables and User Defined Data Types *******************/
/******************************************************************************/

SDRAM_HandleTypeDef hsdram1;
static FMC_SDRAM_TimingTypeDef SdramTiming;
static FMC_SDRAM_CommandTypeDef Command;

/******************************************************************************/
/************************ Functions Definitions *******************************/
/******************************************************************************/

/**
  * @brief  Initializes the SDRAM device.
  * @retval SDRAM status
  */
uint8_t SDP_SDRAM_Init(void)
{
  static uint8_t sdramstatus = SDRAM_ERROR;
  /* SDRAM device configuration */
  hsdram1.Instance = FMC_SDRAM_DEVICE;

  /* SdramTiming configuration */
  SdramTiming.LoadToActiveDelay    = 16;
  SdramTiming.ExitSelfRefreshDelay = 16;
  SdramTiming.SelfRefreshTime      = 16;
  SdramTiming.RowCycleDelay        = 16;
  SdramTiming.WriteRecoveryTime    = 16;
  SdramTiming.RPDelay              = 16;
  SdramTiming.RCDDelay             = 16;

  hsdram1.Init.SDBank             = FMC_SDRAM_BANK1;
  hsdram1.Init.ColumnBitsNumber   = FMC_SDRAM_COLUMN_BITS_NUM_8;
  hsdram1.Init.RowBitsNumber      = FMC_SDRAM_ROW_BITS_NUM_12;
  hsdram1.Init.MemoryDataWidth    = FMC_SDRAM_MEM_BUS_WIDTH_32;
  hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
  hsdram1.Init.CASLatency         = FMC_SDRAM_CAS_LATENCY_3;
  hsdram1.Init.WriteProtection    = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
  hsdram1.Init.SDClockPeriod      = SDCLOCK_PERIOD;
  hsdram1.Init.ReadBurst          = FMC_SDRAM_RBURST_ENABLE;
  hsdram1.Init.ReadPipeDelay      = FMC_SDRAM_RPIPE_DELAY_0;

  /* SDRAM controller initialization */

  SDP_SDRAM_MspInit(&hsdram1, NULL); /* __weak function can be rewritten by the application */

  if(HAL_SDRAM_Init(&hsdram1, &SdramTiming) != HAL_OK)
  {
    sdramstatus = SDRAM_ERROR;
  }
  else
  {
    sdramstatus = SDRAM_OK;
  }

  /* SDRAM initialization sequence */
  SDP_SDRAM_Initialization_sequence(REFRESH_COUNT);

  return sdramstatus;
}

/**
  * @brief  DeInitializes the SDRAM device.
  * @retval SDRAM status
  */
uint8_t SDP_SDRAM_DeInit(void)
{
  static uint8_t sdramstatus = SDRAM_ERROR;
  /* SDRAM device de-initialization */
  hsdram1.Instance = FMC_SDRAM_DEVICE;

  if(HAL_SDRAM_DeInit(&hsdram1) != HAL_OK)
  {
    sdramstatus = SDRAM_ERROR;
  }
  else
  {
    sdramstatus = SDRAM_OK;
  }

  /* SDRAM controller de-initialization */
  SDP_SDRAM_MspDeInit(&hsdram1, NULL);

  return sdramstatus;
}

/**
  * @brief  Programs the SDRAM device.
  * @param  RefreshCount: SDRAM refresh counter value
  * @retval None
  */
void SDP_SDRAM_Initialization_sequence(uint32_t RefreshCount)
{
  __IO uint32_t tmpmrd = 0;

  /* Step 1: Configure a clock configuration enable command */
  Command.CommandMode            = FMC_SDRAM_CMD_CLK_ENABLE;
  Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
  Command.AutoRefreshNumber      = 1;
  Command.ModeRegisterDefinition = 0;

  /* Send the command */
  HAL_SDRAM_SendCommand(&hsdram1, &Command, SDRAM_TIMEOUT);

  /* Step 2: Insert 100 us minimum delay */
  /* Inserted delay is equal to 1 ms due to systick time base unit (ms) */
  HAL_Delay(1);

  /* Step 3: Configure a PALL (precharge all) command */
  Command.CommandMode            = FMC_SDRAM_CMD_PALL;
  Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
  Command.AutoRefreshNumber      = 1;
  Command.ModeRegisterDefinition = 0;

  /* Send the command */
  HAL_SDRAM_SendCommand(&hsdram1, &Command, SDRAM_TIMEOUT);

  /* Step 4: Configure an Auto Refresh command */
  Command.CommandMode            = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
  Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
  Command.AutoRefreshNumber      = 8;
  Command.ModeRegisterDefinition = 0;

  /* Send the command */
  HAL_SDRAM_SendCommand(&hsdram1, &Command, SDRAM_TIMEOUT);

  /* Step 5: Program the external memory mode register */
  tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_1          |\
                     SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |\
                     SDRAM_MODEREG_CAS_LATENCY_3           |\
                     SDRAM_MODEREG_OPERATING_MODE_STANDARD |\
                     SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

  Command.CommandMode            = FMC_SDRAM_CMD_LOAD_MODE;
  Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
  Command.AutoRefreshNumber      = 1;
  Command.ModeRegisterDefinition = tmpmrd;

  /* Send the command */
  HAL_SDRAM_SendCommand(&hsdram1, &Command, SDRAM_TIMEOUT);

  /* Step 6: Set the refresh rate counter */
  /* Set the device refresh rate */
  HAL_SDRAM_ProgramRefreshRate(&hsdram1, RefreshCount);
}

/**
  * @brief  Reads an amount of data from the SDRAM memory in polling mode.
  * @param  pAddress: Read start address
  * @param  pData: Pointer to data to be read
  * @param  dataSize: Size of read data from the memory
  * @retval SDRAM status
  */
uint8_t SDP_SDRAM_ReadData_8b(uint32_t pAddress, uint8_t *pData, uint32_t dataSize)
{
  if(HAL_SDRAM_Read_8b(&hsdram1, (uint32_t *)pAddress, pData, dataSize) != HAL_OK)
  {
    return SDRAM_ERROR;
  }
  else
  {
    return SDRAM_OK;
  }
}

/**
  * @brief  Reads an amount of data from the SDRAM memory in polling mode.
  * @param  pAddress: Read start address
  * @param  pData: Pointer to data to be read
  * @param  dataSize: Size of read data from the memory
  * @retval SDRAM status
  */
uint8_t SDP_SDRAM_ReadData_16b(uint32_t pAddress, uint16_t *pData, uint32_t dataSize)
{
  if(HAL_SDRAM_Read_16b(&hsdram1, (uint32_t *)pAddress, pData, dataSize) != HAL_OK)
  {
    return SDRAM_ERROR;
  }
  else
  {
    return SDRAM_OK;
  }
}

/**
  * @brief  Reads an amount of data from the SDRAM memory in polling mode.
  * @param  pAddress: Read start address
  * @param  pData: Pointer to data to be read
  * @param  dataSize: Size of read data from the memory
  * @retval SDRAM status
  */
uint8_t SDP_SDRAM_ReadData_32b(uint32_t pAddress, uint32_t *pData, uint32_t dataSize)
{
  if(HAL_SDRAM_Read_32b(&hsdram1, (uint32_t *)pAddress, pData, dataSize) != HAL_OK)
  {
    return SDRAM_ERROR;
  }
  else
  {
    return SDRAM_OK;
  }
}

/**
  * @brief  Reads an amount of data from the SDRAM memory in DMA mode.
  * @param  pAddress: Read start address
  * @param  pData: Pointer to data to be read
  * @param  dataSize: Size of read data from the memory
  * @retval SDRAM status
  */
uint8_t SDP_SDRAM_ReadData_DMA(uint32_t pAddress, uint32_t *pData, uint32_t dataSize)
{
  if(HAL_SDRAM_Read_DMA(&hsdram1, (uint32_t *)pAddress, pData, dataSize) != HAL_OK)
  {
    return SDRAM_ERROR;
  }
  else
  {
    return SDRAM_OK;
  }
}

/**
  * @brief  Writes an amount of data to the SDRAM memory in polling mode.
  * @param  pAddress: Write start address
  * @param  pData: Pointer to data to be written
  * @param  dataSize: Size of written data from the memory
  * @retval SDRAM status
  */
uint8_t SDP_SDRAM_WriteData_8b(uint32_t pAddress, uint8_t *pData, uint32_t dataSize)
{
  if(HAL_SDRAM_Write_8b(&hsdram1, (uint32_t *)pAddress, pData, dataSize) != HAL_OK)
  {
    return SDRAM_ERROR;
  }
  else
  {
    return SDRAM_OK;
  }
}

/**
  * @brief  Writes an amount of data to the SDRAM memory in polling mode.
  * @param  pAddress: Write start address
  * @param  pData: Pointer to data to be written
  * @param  dataSize: Size of written data from the memory
  * @retval SDRAM status
  */
uint8_t SDP_SDRAM_WriteData_16b(uint32_t pAddress, uint16_t *pData, uint32_t dataSize)
{
  if(HAL_SDRAM_Write_16b(&hsdram1, (uint32_t *)pAddress, pData, dataSize) != HAL_OK)
  {
    return SDRAM_ERROR;
  }
  else
  {
    return SDRAM_OK;
  }
}

/**
  * @brief  Writes an amount of data to the SDRAM memory in polling mode.
  * @param  pAddress: Write start address
  * @param  pData: Pointer to data to be written
  * @param  dataSize: Size of written data from the memory
  * @retval SDRAM status
  */
uint8_t SDP_SDRAM_WriteData_32b(uint32_t pAddress, uint32_t *pData, uint32_t dataSize)
{
  if(HAL_SDRAM_Write_32b(&hsdram1, (uint32_t *)pAddress, pData, dataSize) != HAL_OK)
  {
    return SDRAM_ERROR;
  }
  else
  {
    return SDRAM_OK;
  }
}

/**
  * @brief  Writes an amount of data to the SDRAM memory in DMA mode.
  * @param  pAddress: Write start address
  * @param  pData: Pointer to data to be written
  * @param  dataSize: Size of written data from the memory
  * @retval SDRAM status
  */
uint8_t SDP_SDRAM_WriteData_DMA(uint32_t pAddress, uint32_t *pData, uint32_t dataSize)
{
  if(HAL_SDRAM_Write_DMA(&hsdram1, (uint32_t *)pAddress, pData, dataSize) != HAL_OK)
  {
    return SDRAM_ERROR;
  }
  else
  {
    return SDRAM_OK;
  }
}

/**
  * @brief  Sends command to the SDRAM bank.
  * @param  SdramCmd: Pointer to SDRAM command structure
  * @retval SDRAM status
  */
uint8_t SDP_SDRAM_Sendcmd(FMC_SDRAM_CommandTypeDef *SdramCmd)
{
  if(HAL_SDRAM_SendCommand(&hsdram1, SdramCmd, SDRAM_TIMEOUT) != HAL_OK)
  {
    return SDRAM_ERROR;
  }
  else
  {
    return SDRAM_OK;
  }
}

/**
  * @brief  Initializes SDRAM MSP.
  * @param  hsdram: SDRAM handle
  * @param  Params
  * @retval None
  */
__weak void SDP_SDRAM_MspInit(SDRAM_HandleTypeDef  *hsdram, void *Params)
{
  static DMA_HandleTypeDef dma_handle;
  GPIO_InitTypeDef gpio_init_structure;

  /* Enable FMC clock */
  __HAL_RCC_FMC_CLK_ENABLE();

  /* Enable chosen DMAx clock */
  __DMAx_CLK_ENABLE();

  /* Enable GPIOs clock */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();

  /* Common GPIO configuration */
  gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
  gpio_init_structure.Pull      = GPIO_PULLUP;
  gpio_init_structure.Speed     = GPIO_SPEED_HIGH;
  gpio_init_structure.Alternate = GPIO_AF12_FMC;

  /* GPIO Configuration */
  gpio_init_structure.Pin   = SDRAM_D0 | SDRAM_D1 | SDRAM_D2 | SDRAM_D3 | SDRAM_D13 |\
                              SDRAM_D14 | SDRAM_D15;
  HAL_GPIO_Init(GPIOD, &gpio_init_structure);

  gpio_init_structure.Pin   = SDRAM_NBL0 | SDRAM_NBL1 | SDRAM_D4| SDRAM_D5 | SDRAM_D6 |\
                              SDRAM_D7 | SDRAM_D8 | SDRAM_D9 | SDRAM_D10 | SDRAM_D11 |\
                              SDRAM_D12;
  HAL_GPIO_Init(GPIOE, &gpio_init_structure);

  gpio_init_structure.Pin   = SDRAM_A0 | SDRAM_A1 | SDRAM_A2| SDRAM_A3 | SDRAM_A4 |\
                              SDRAM_A5 | SDRAM_N_RAS | SDRAM_A6 | SDRAM_A7 | SDRAM_A8 |\
                              SDRAM_A9;
  HAL_GPIO_Init(GPIOF, &gpio_init_structure);

  gpio_init_structure.Pin   = SDRAM_A10 | SDRAM_A11 | SDRAM_A12 | SDRAM_A13|\
                              SDRAM_A14 | SDRAM_A15 | SDRAM_SDCLK | SDRAM_N_CAS;
  HAL_GPIO_Init(GPIOG, &gpio_init_structure);

  gpio_init_structure.Pin   = SDRAM_SDCKE0 | SDRAM_SDNE0 | SDRAM_N_WE | SDRAM_D16 |\
                              SDRAM_D17 | SDRAM_D18 | SDRAM_D19 | SDRAM_D20 | SDRAM_D21 |\
                              SDRAM_D22 | SDRAM_D23;
  HAL_GPIO_Init(GPIOH, &gpio_init_structure);

  gpio_init_structure.Pin   = SDRAM_D24 | SDRAM_D25 | SDRAM_D26 | SDRAM_D27 | SDRAM_D28 |\
                              SDRAM_D29 | SDRAM_D30 | SDRAM_D31 | SDRAM_NBL2 | SDRAM_NBL3;
  HAL_GPIO_Init(GPIOI, &gpio_init_structure);

  /* Configure common DMA parameters */
  dma_handle.Init.Channel             = SDRAM_DMAx_CHANNEL;
  dma_handle.Init.Direction           = DMA_MEMORY_TO_MEMORY;
  dma_handle.Init.PeriphInc           = DMA_PINC_ENABLE;
  dma_handle.Init.MemInc              = DMA_MINC_ENABLE;
  dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  dma_handle.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
  dma_handle.Init.Mode                = DMA_NORMAL;
  dma_handle.Init.Priority            = DMA_PRIORITY_HIGH;
  dma_handle.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
  dma_handle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
  dma_handle.Init.MemBurst            = DMA_MBURST_SINGLE;
  dma_handle.Init.PeriphBurst         = DMA_PBURST_SINGLE;

  dma_handle.Instance = SDRAM_DMAx_STREAM;

   /* Associate the DMA handle */
  __HAL_LINKDMA(hsdram, hdma, dma_handle);

  /* Deinitialize the stream for new transfer */
  HAL_DMA_DeInit(&dma_handle);

  /* Configure the DMA stream */
  HAL_DMA_Init(&dma_handle);

  /* NVIC configuration for DMA transfer complete interrupt */
  HAL_NVIC_SetPriority(SDRAM_DMAx_IRQn, 0x0F, 0);
  HAL_NVIC_EnableIRQ(SDRAM_DMAx_IRQn);
}

/**
  * @brief  DeInitializes SDRAM MSP.
  * @param  hsdram: SDRAM handle
  * @param  Params
  * @retval None
  */
__weak void SDP_SDRAM_MspDeInit(SDRAM_HandleTypeDef  *hsdram, void *Params)
{
    static DMA_HandleTypeDef dma_handle;

    /* Disable NVIC configuration for DMA interrupt */
    HAL_NVIC_DisableIRQ(SDRAM_DMAx_IRQn);

    /* Deinitialize the stream for new transfer */
    dma_handle.Instance = SDRAM_DMAx_STREAM;
    HAL_DMA_DeInit(&dma_handle);

}

#endif  // TARGET_SDP_K1
