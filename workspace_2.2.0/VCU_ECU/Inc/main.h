/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define VCU_STATUS_LED_Pin GPIO_PIN_0
#define VCU_STATUS_LED_GPIO_Port GPIOB
#define VCU_FAULT_LED_Pin GPIO_PIN_1
#define VCU_FAULT_LED_GPIO_Port GPIOB
#define GPIO_Input_Pin GPIO_PIN_10
#define GPIO_Input_GPIO_Port GPIOB
#define GPIO_InputB11_Pin GPIO_PIN_11
#define GPIO_InputB11_GPIO_Port GPIOB
#define GPIO_InputB12_Pin GPIO_PIN_12
#define GPIO_InputB12_GPIO_Port GPIOB
#define GPIO_InputB13_Pin GPIO_PIN_13
#define GPIO_InputB13_GPIO_Port GPIOB
#define MOTOR_ENABLE_Pin GPIO_PIN_8
#define MOTOR_ENABLE_GPIO_Port GPIOB
#define CONTACTOR_ENABLE_Pin GPIO_PIN_9
#define CONTACTOR_ENABLE_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
