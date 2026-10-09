/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : BMS ECU main application
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

#include "adc.h"
#include "can.h"
#include "i2c.h"
#include "usart.h"

#include "bms_data.h"
#include "bms_config.h"
#include "bms_adc.h"
#include "bms_temperature.h"
#include "bms_soc.h"
#include "bms_soh.h"
#include "bms_faults.h"
#include "bms_can.h"
#include "bms_state.h"
#include "bms_lcd.h"

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

static uint32_t last_adc_update = 0U;
static uint32_t last_temperature_update = 0U;
static uint32_t last_fault_update = 0U;
static uint32_t last_can_update = 0U;
static uint32_t last_lcd_update = 0U;
static uint32_t last_state_update = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/* USER CODE BEGIN PFP */

static void BMS_UpdateOutputs(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief Update BMS status and fault LEDs.
  *
  * PB0 = BMS status LED
  * PB1 = BMS fault LED
  */
static void BMS_UpdateOutputs(void)
{
    BMS_SystemState_t state = BMS_Data.system_state;

    /* ---------------------------------------------------------- */
    /* STATUS LED                                                  */
    /* ---------------------------------------------------------- */

    if ((state == BMS_SYSTEM_NORMAL) ||
        (state == BMS_SYSTEM_WARNING))
    {
        HAL_GPIO_WritePin(
            BMS_STATUS_LED_GPIO_PORT,
            BMS_STATUS_LED_PIN,
            GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(
            BMS_STATUS_LED_GPIO_PORT,
            BMS_STATUS_LED_PIN,
            GPIO_PIN_RESET);
    }

    /* ---------------------------------------------------------- */
    /* FAULT LED                                                   */
    /* ---------------------------------------------------------- */

    if ((state == BMS_SYSTEM_FAULT) ||
        (BMS_Data.fault_status != 0U) ||
        (BMS_Data.fault_latched != 0U))
    {
        HAL_GPIO_WritePin(
            BMS_FAULT_LED_GPIO_PORT,
            BMS_FAULT_LED_PIN,
            GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(
            BMS_FAULT_LED_GPIO_PORT,
            BMS_FAULT_LED_PIN,
            GPIO_PIN_RESET);
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* Reset of all peripherals, Initializes the Flash interface and Systick */
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_ADC2_Init();
    MX_CAN_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();

    /* USER CODE BEGIN 2 */

    /* ========================================================== */
    /* BMS SOFTWARE INITIALIZATION                                */
    /* ========================================================== */

    BMS_ADC_Init();

    BMS_Temperature_Init();

    BMS_SOC_Init();

    BMS_SOH_Init();

    BMS_Faults_Init();

    BMS_State_Init();

    BMS_LCD_Init();

    /* ========================================================== */
    /* START CAN                                                  */
    /* ========================================================== */

    BMS_CAN_Start();

    /* ========================================================== */
    /* INITIAL OUTPUT STATE                                       */
    /* ========================================================== */

    BMS_UpdateOutputs();

    /* ========================================================== */
    /* INITIALIZE SOFTWARE SCHEDULER                              */
    /* ========================================================== */

    {
        uint32_t now = HAL_GetTick();

        last_adc_update = now;
        last_temperature_update = now;
        last_fault_update = now;
        last_can_update = now;
        last_lcd_update = now;
        last_state_update = now;
    }

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */

    while (1)
    {
        uint32_t now = HAL_GetTick();

        /* ====================================================== */
        /* ADC UPDATE                                             */
        /* ====================================================== */

        if ((now - last_adc_update) >=
            BMS_ADC_UPDATE_PERIOD_MS)
        {
            last_adc_update = now;

            BMS_ADC_Update();
        }

        /* ====================================================== */
        /* TEMPERATURE UPDATE                                     */
        /* ====================================================== */

        if ((now - last_temperature_update) >=
            BMS_TEMP_UPDATE_PERIOD_MS)
        {
            last_temperature_update = now;

            BMS_Temperature_Update();
        }

        /* ====================================================== */
        /* SOC UPDATE                                             */
        /* ====================================================== */

        BMS_SOC_Update();

        /* ====================================================== */
        /* SOH UPDATE                                             */
        /* ====================================================== */

        BMS_SOH_Update();

        /* ====================================================== */
        /* FAULT UPDATE                                           */
        /* ====================================================== */

        if ((now - last_fault_update) >=
            BMS_FAULT_UPDATE_PERIOD_MS)
        {
            last_fault_update = now;

            BMS_Faults_Update();
        }

        /* ====================================================== */
        /* CAN COMMAND PROCESSING                                  */
        /* ====================================================== */

        BMS_CAN_ProcessCommands();

        /* ====================================================== */
        /* STATE MACHINE UPDATE                                    */
        /* ====================================================== */

        if ((now - last_state_update) >=
            BMS_FAULT_UPDATE_PERIOD_MS)
        {
            last_state_update = now;

            BMS_State_Update();
        }

        /* ====================================================== */
        /* PERIODIC CAN STATUS                                    */
        /* ====================================================== */

        if ((now - last_can_update) >=
            BMS_CAN_UPDATE_PERIOD_MS)
        {
            last_can_update = now;

            BMS_CAN_SendTestMessage();
        }

        /* ====================================================== */
        /* LCD UPDATE                                             */
        /* ====================================================== */

        if ((now - last_lcd_update) >=
            BMS_LCD_UPDATE_PERIOD_MS)
        {
            last_lcd_update = now;

            BMS_LCD_Update();
        }

        /* ====================================================== */
        /* UPDATE OUTPUTS                                         */
        /* ====================================================== */

        BMS_UpdateOutputs();
    }

    /* USER CODE END WHILE */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /* ========================================================== */
    /* HSE + PLL                                                  */
    /* ========================================================== */

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

    /* ========================================================== */
    /* CPU / AHB / APB CLOCKS                                    */
    /* ========================================================== */

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

    /* ========================================================== */
    /* ADC CLOCK = PCLK2 / 6                                      */
    /* ========================================================== */

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

/**
  * @brief GPIO Initialization Function
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* ========================================================== */
    /* BMS STATUS LED - PB0                                      */
    /* ========================================================== */

    HAL_GPIO_WritePin(
        BMS_STATUS_LED_GPIO_PORT,
        BMS_STATUS_LED_PIN,
        GPIO_PIN_RESET);

    GPIO_InitStruct.Pin =
        BMS_STATUS_LED_PIN;

    GPIO_InitStruct.Mode =
        GPIO_MODE_OUTPUT_PP;

    GPIO_InitStruct.Pull =
        GPIO_NOPULL;

    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(
        BMS_STATUS_LED_GPIO_PORT,
        &GPIO_InitStruct);

    /* ========================================================== */
    /* BMS FAULT LED - PB1                                       */
    /* ========================================================== */

    HAL_GPIO_WritePin(
        BMS_FAULT_LED_GPIO_PORT,
        BMS_FAULT_LED_PIN,
        GPIO_PIN_RESET);

    GPIO_InitStruct.Pin =
        BMS_FAULT_LED_PIN;

    GPIO_InitStruct.Mode =
        GPIO_MODE_OUTPUT_PP;

    GPIO_InitStruct.Pull =
        GPIO_NOPULL;

    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(
        BMS_FAULT_LED_GPIO_PORT,
        &GPIO_InitStruct);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
        /*
         * Fatal MCU/HAL initialization error.
         */
    }
}

#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  */
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}

#endif /* USE_FULL_ASSERT */
