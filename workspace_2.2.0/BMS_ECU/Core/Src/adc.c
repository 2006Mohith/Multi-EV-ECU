/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    adc.c
  * @brief   This file provides code for the configuration
  *          of the ADC instances.
  ******************************************************************************
  * @attention
  *
  * BMS ECU ADC Configuration
  *
  * ADC1:
  *   PA0 -> ADC1_IN0 -> Battery Voltage
  *   PA1 -> ADC1_IN1 -> Battery Current
  *
  * ADC2:
  *   Currently unused by the BMS application.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "adc.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;

/* ADC1 init function */
void MX_ADC1_Init(void)
{
  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /*
   * ---------------------------------------------------------
   * ADC1 Configuration
   * ---------------------------------------------------------
   *
   * PA0 -> ADC1_IN0 -> Battery Voltage
   * PA1 -> ADC1_IN1 -> Battery Current
   *
   * Only 2 conversions are required.
   */
  hadc1.Instance = ADC1;

  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;

  /*
   * IMPORTANT:
   * BMS_ADC_Update() reads exactly 2 ADC conversions.
   */
  hadc1.Init.NbrOfConversion = 2;

  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /*
   * ---------------------------------------------------------
   * ADC1 Rank 1
   * ---------------------------------------------------------
   *
   * PA0 -> ADC1_IN0
   * Battery Voltage
   */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;

  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /*
   * ---------------------------------------------------------
   * ADC1 Rank 2
   * ---------------------------------------------------------
   *
   * PA1 -> ADC1_IN1
   * Battery Current
   */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;

  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /*
   * No Rank 3.
   *
   * PA2 / ADC_CHANNEL_2 is not used by ADC1.
   */

  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */
}


/* ADC2 init function */
void MX_ADC2_Init(void)
{
  /* USER CODE BEGIN ADC2_Init 0 */

  /* USER CODE END ADC2_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC2_Init 1 */

  /* USER CODE END ADC2_Init 1 */

  /*
   * ---------------------------------------------------------
   * ADC2 Configuration
   * ---------------------------------------------------------
   *
   * ADC2 is currently not used by the BMS software.
   *
   * It is kept here because CubeMX currently generates
   * ADC2 initialization.
   */
  hadc2.Instance = ADC2;

  hadc2.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc2.Init.ContinuousConvMode = DISABLE;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.NbrOfConversion = 1;

  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    Error_Handler();
  }

  /*
   * ADC2 Channel 2
   *
   * PA2 -> ADC2_IN2
   *
   * Currently reserved for future use,
   * such as a temperature sensor.
   */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;

  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN ADC2_Init 2 */

  /* USER CODE END ADC2_Init 2 */
}


/**
  * @brief ADC MSP Initialization
  * This function configures the hardware resources used in this example
  * @param adcHandle: ADC handle pointer
  * @retval None
  */
void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /*
   * ---------------------------------------------------------
   * ADC1 MSP Initialization
   * ---------------------------------------------------------
   */
  if (adcHandle->Instance == ADC1)
  {
    /* USER CODE BEGIN ADC1_MspInit 0 */

    /* USER CODE END ADC1_MspInit 0 */

    /*
     * Enable ADC1 clock
     */
    __HAL_RCC_ADC1_CLK_ENABLE();

    /*
     * Enable GPIOA clock
     */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /*
     * ADC1 GPIO Configuration
     *
     * PA0 -> ADC1_IN0 -> Battery Voltage
     * PA1 -> ADC1_IN1 -> Battery Current
     */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USER CODE BEGIN ADC1_MspInit 1 */

    /* USER CODE END ADC1_MspInit 1 */
  }

  /*
   * ---------------------------------------------------------
   * ADC2 MSP Initialization
   * ---------------------------------------------------------
   */
  else if (adcHandle->Instance == ADC2)
  {
    /* USER CODE BEGIN ADC2_MspInit 0 */

    /* USER CODE END ADC2_MspInit 0 */

    /*
     * Enable ADC2 clock
     */
    __HAL_RCC_ADC2_CLK_ENABLE();

    /*
     * Enable GPIOA clock
     */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /*
     * ADC2 GPIO Configuration
     *
     * PA2 -> ADC2_IN2
     *
     * Currently reserved for future temperature sensor.
     */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USER CODE BEGIN ADC2_MspInit 1 */

    /* USER CODE END ADC2_MspInit 1 */
  }
}


/**
  * @brief ADC MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param adcHandle: ADC handle pointer
  * @retval None
  */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{
  /*
   * ---------------------------------------------------------
   * ADC1 MSP De-Initialization
   * ---------------------------------------------------------
   */
  if (adcHandle->Instance == ADC1)
  {
    /* USER CODE BEGIN ADC1_MspDeInit 0 */

    /* USER CODE END ADC1_MspDeInit 0 */

    /*
     * Disable ADC1 clock
     */
    __HAL_RCC_ADC1_CLK_DISABLE();

    /*
     * De-initialize PA0 and PA1
     */
    HAL_GPIO_DeInit(
        GPIOA,
        GPIO_PIN_0 | GPIO_PIN_1
    );

    /* USER CODE BEGIN ADC1_MspDeInit 1 */

    /* USER CODE END ADC1_MspDeInit 1 */
  }

  /*
   * ---------------------------------------------------------
   * ADC2 MSP De-Initialization
   * ---------------------------------------------------------
   */
  else if (adcHandle->Instance == ADC2)
  {
    /* USER CODE BEGIN ADC2_MspDeInit 0 */

    /* USER CODE END ADC2_MspDeInit 0 */

    /*
     * Disable ADC2 clock
     */
    __HAL_RCC_ADC2_CLK_DISABLE();

    /*
     * De-initialize PA2
     */
    HAL_GPIO_DeInit(
        GPIOA,
        GPIO_PIN_2
    );

    /* USER CODE BEGIN ADC2_MspDeInit 1 */

    /* USER CODE END ADC2_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
