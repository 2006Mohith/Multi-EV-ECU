/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : VCU ECU Main Program
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/

#include "main.h"
#include "adc.h"
#include "can.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */

#include "vcu_config.h"
#include "vcu_data.h"
#include "vcu_debug.h"
#include "vcu_lcd.h"
#include "vcu_can.h"
#include "vcu_system.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

static uint32_t vcu_last_control_tick = 0U;
static uint32_t vcu_last_can_tick = 0U;
static uint32_t vcu_last_lcd_tick = 0U;
static uint32_t vcu_last_debug_tick = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

void SystemClock_Config(void);

/* USER CODE BEGIN PFP */

static void VCU_Main_Update(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  uint32_t current_tick;

  /* MCU Configuration--------------------------------------------------------*/

  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_CAN_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */

  /*
   * Initialize the complete VCU application.
   *
   * This initializes:
   *   - shared VCU data
   *   - ADC software layer
   *   - digital input layer
   *   - fault manager
   *   - state machine
   *   - CAN software layer
   */
  VCU_System_Init();

  /*
   * Initialize LCD and debug layers.
   */
  VCU_LCD_Init();
  VCU_Debug_Init();

  /*
   * Force all safety-critical outputs OFF at startup.
   */
  HAL_GPIO_WritePin(
      GPIOB,
      GPIO_PIN_0,
      GPIO_PIN_RESET);

  HAL_GPIO_WritePin(
      GPIOB,
      GPIO_PIN_1,
      GPIO_PIN_RESET);

  HAL_GPIO_WritePin(
      GPIOB,
      GPIO_PIN_8,
      GPIO_PIN_RESET);

  HAL_GPIO_WritePin(
      GPIOB,
      GPIO_PIN_9,
      GPIO_PIN_RESET);

  /*
   * Establish periodic task timestamps.
   */
  current_tick = HAL_GetTick();

  vcu_last_control_tick = current_tick;
  vcu_last_can_tick = current_tick;
  vcu_last_lcd_tick = current_tick;
  vcu_last_debug_tick = current_tick;

  VCU_Debug_SendString(
      "VCU ECU START\r\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  while (1)
  {
    current_tick = HAL_GetTick();

    /*
     * =====================================================
     * 20 ms CONTROL TASK
     * =====================================================
     */
    if ((current_tick - vcu_last_control_tick) >=
        VCU_STATE_UPDATE_PERIOD_MS)
    {
      vcu_last_control_tick = current_tick;

      VCU_Main_Update();
    }

    /*
     * =====================================================
     * 20 ms CAN SUPERVISION
     * =====================================================
     */
    if ((current_tick - vcu_last_can_tick) >=
        VCU_CAN_UPDATE_PERIOD_MS)
    {
      vcu_last_can_tick = current_tick;

      VCU_CAN_Update();

      /*
       * Broadcast VCU status periodically.
       */
      (void)VCU_CAN_SendStatus();
    }

    /*
     * =====================================================
     * 500 ms LCD TASK
     * =====================================================
     */
    if ((current_tick - vcu_last_lcd_tick) >=
        VCU_LCD_UPDATE_PERIOD_MS)
    {
      vcu_last_lcd_tick = current_tick;

      if (VCU_LCD_IsReady() != 0U)
      {
        VCU_LCD_Update();
      }
    }

    /*
     * =====================================================
     * 1000 ms DEBUG TASK
     * =====================================================
     */
    if ((current_tick - vcu_last_debug_tick) >= 1000U)
    {
      vcu_last_debug_tick = current_tick;

      VCU_Debug_SendUInt(
          "STATE: ",
          (uint32_t)VCU_Data.system_state);

      VCU_Debug_SendUInt(
          "BMS COMM: ",
          (uint32_t)VCU_Data.bms.communication_valid);

      VCU_Debug_SendUInt(
          "MCU COMM: ",
          (uint32_t)VCU_Data.mcu.communication_valid);

      VCU_Debug_SendUInt(
          "FAULT: ",
          (uint32_t)VCU_Data.faults.code);
    }
  }
}

/* =========================================================
 * MAIN VCU CONTROL UPDATE
 * ========================================================= */

static void VCU_Main_Update(void)
{
  /*
   * Execute one complete VCU control cycle.
   */
  VCU_System_Update();
}

/* =========================================================
 * SYSTEM CLOCK CONFIGURATION
 * ========================================================= */

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /*
   * HSE = 8 MHz
   * PLL = HSE × 9
   * SYSCLK = 72 MHz
   */
  RCC_OscInitStruct.OscillatorType =
      RCC_OSCILLATORTYPE_HSE;

  RCC_OscInitStruct.HSEState =
      RCC_HSE_ON;

  RCC_OscInitStruct.HSEPredivValue =
      RCC_HSE_PREDIV_DIV1;

  RCC_OscInitStruct.HSIState =
      RCC_HSI_ON;

  RCC_OscInitStruct.PLL.PLLState =
      RCC_PLL_ON;

  RCC_OscInitStruct.PLL.PLLSource =
      RCC_PLLSOURCE_HSE;

  RCC_OscInitStruct.PLL.PLLMUL =
      RCC_PLL_MUL9;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /*
   * AHB  = 72 MHz
   * APB1 = 36 MHz
   * APB2 = 72 MHz
   */
  RCC_ClkInitStruct.ClockType =
      RCC_CLOCKTYPE_HCLK |
      RCC_CLOCKTYPE_SYSCLK |
      RCC_CLOCKTYPE_PCLK1 |
      RCC_CLOCKTYPE_PCLK2;

  RCC_ClkInitStruct.SYSCLKSource =
      RCC_SYSCLKSOURCE_PLLCLK;

  RCC_ClkInitStruct.AHBCLKDivider =
      RCC_SYSCLK_DIV1;

  RCC_ClkInitStruct.APB1CLKDivider =
      RCC_HCLK_DIV2;

  RCC_ClkInitStruct.APB2CLKDivider =
      RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(
          &RCC_ClkInitStruct,
          FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }

  /*
   * ADC clock = 72 MHz / 6 = 12 MHz
   */
  PeriphClkInit.PeriphClockSelection =
      RCC_PERIPHCLK_ADC;

  PeriphClkInit.AdcClockSelection =
      RCC_ADCPCLK2_DIV6;

  if (HAL_RCCEx_PeriphCLKConfig(
          &PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* =========================================================
 * ERROR HANDLER
 * ========================================================= */

void Error_Handler(void)
{
  __disable_irq();

  while (1)
  {
    /*
     * All safety-critical outputs OFF.
     */
    HAL_GPIO_WritePin(
        GPIOB,
        GPIO_PIN_8,
        GPIO_PIN_RESET);

    HAL_GPIO_WritePin(
        GPIOB,
        GPIO_PIN_9,
        GPIO_PIN_RESET);

    /*
     * Fault LED ON.
     */
    HAL_GPIO_WritePin(
        GPIOB,
        GPIO_PIN_1,
        GPIO_PIN_SET);
  }
}

#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}

#endif /* USE_FULL_ASSERT */
