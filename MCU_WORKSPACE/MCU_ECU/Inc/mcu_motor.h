#ifndef MCU_MOTOR_H
#define MCU_MOTOR_H

#include <stdint.h>
#include "mcu_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * MCU Motor Control Module
 *
 * Hardware:
 * STM32F103C8T6
 *      │
 *      ├── PA8  → TIM1_CH1 PWM → TB6612FNG PWMA
 *      ├── PB0  → AIN1
 *      ├── PB1  → AIN2
 *      └── PB2  → STBY
 *
 * Motor A of TB6612FNG is used.
 * ========================================================================== */

/* ============================================================================
 * Initialization
 * ========================================================================== */

/**
 * @brief Initialize motor-control software state and force safe outputs.
 */
void MCU_Motor_Init(void);

/* ============================================================================
 * Periodic motor processing
 * ========================================================================== */

/**
 * @brief Process motor control logic.
 *
 * This function reads the validated motor command from MCU_Data,
 * determines the required direction/PWM/enable state, and updates
 * the motor outputs.
 */
void MCU_Motor_Update(void);

/* ============================================================================
 * Enable / disable control
 * ========================================================================== */

/**
 * @brief Enable the TB6612FNG motor driver.
 */
void MCU_Motor_Enable(void);

/**
 * @brief Disable the TB6612FNG motor driver.
 */
void MCU_Motor_Disable(void);

/**
 * @brief Emergency-safe motor stop.
 *
 * Forces PWM to zero, disables the driver, and places direction
 * outputs into the safe neutral condition.
 */
void MCU_Motor_SafeStop(void);

/* ============================================================================
 * Direction control
 * ========================================================================== */

/**
 * @brief Set motor direction.
 *
 * @param direction
 *        MCU_DIRECTION_NEUTRAL
 *        MCU_DIRECTION_FORWARD
 *        MCU_DIRECTION_REVERSE
 */
void MCU_Motor_SetDirection(uint8_t direction);

/* ============================================================================
 * PWM control
 * ========================================================================== */

/**
 * @brief Set motor PWM duty cycle.
 *
 * @param pwm_percent Duty cycle from 0 to 100 percent.
 */
void MCU_Motor_SetPWM(uint8_t pwm_percent);

/**
 * @brief Convert percentage to TIM1 compare value.
 *
 * @param pwm_percent Duty cycle from 0 to 100 percent.
 *
 * @return TIM1 compare value.
 */
uint16_t MCU_Motor_PwmPercentToCompare(uint8_t pwm_percent);

/* ============================================================================
 * Output handling
 * ========================================================================== */

/**
 * @brief Apply current motor output data to the physical GPIO/PWM outputs.
 */
void MCU_Motor_ApplyOutputs(void);

/* ============================================================================
 * Status functions
 * ========================================================================== */

/**
 * @brief Return whether motor output is currently enabled.
 *
 * @return 1 if enabled, 0 if disabled.
 */
uint8_t MCU_Motor_IsEnabled(void);

/**
 * @brief Return whether motor-control outputs are in the safe state.
 *
 * @return 1 if safe, 0 otherwise.
 */
uint8_t MCU_Motor_IsSafe(void);

#ifdef __cplusplus
}
#endif

#endif /* MCU_MOTOR_H */
