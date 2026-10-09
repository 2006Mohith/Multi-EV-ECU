/*
 * mcu_motor.c
 *
 *  Created on: 21-Sept-2026
 *      Author: MohithK
 */


#include "main.h"
#include "tim.h"
#include "mcu_config.h"
#include "mcu_data.h"
#include "mcu_motor.h"

/* ============================================================================
 * Motor control implementation
 *
 * Hardware:
 * PA8 -> TIM1_CH1 PWM -> TB6612FNG PWMA
 * PB0 -> AIN1
 * PB1 -> AIN2
 * PB2 -> STBY
 * ========================================================================== */

/* ============================================================================
 * Local helper
 * ========================================================================== */

static void MCU_Motor_ForceSafeHardwareState(void)
{
    /* Stop PWM */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0U);

    /* Neutral direction */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);

    /* Disable TB6612FNG */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
}

/* ============================================================================
 * Initialization
 * ========================================================================== */

void MCU_Motor_Init(void)
{
    /* Start TIM1 PWM channel */
    if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    /* Force a safe motor state immediately */
    MCU_Motor_ForceSafeHardwareState();

    /* Initialize software state */
    MCU_Data.motor.state = MCU_STATE_DISABLED;
    MCU_Data.motor.direction = MCU_DIRECTION_NEUTRAL;
    MCU_Data.motor.enabled = MCU_DISABLED;
    MCU_Data.motor.pwm_percent = 0U;
    MCU_Data.motor.pwm_compare = 0U;
    MCU_Data.motor.driver_enabled = MCU_DISABLED;
    MCU_Data.motor.braking = MCU_FALSE;
    MCU_Data.motor.direction_change_pending = MCU_FALSE;

    MCU_Data.output.ain1 = 0U;
    MCU_Data.output.ain2 = 0U;
    MCU_Data.output.standby = 0U;
    MCU_Data.output.pwm_percent = 0U;
    MCU_Data.output.motor_output_enabled = MCU_DISABLED;
}

/* ============================================================================
 * Motor update
 * ========================================================================== */

void MCU_Motor_Update(void)
{
    uint8_t requested_direction;
    uint8_t requested_pwm;
    uint8_t requested_enable;

    requested_direction = MCU_Data.motor_command.direction;
    requested_pwm = MCU_Data.motor_command.pwm_percent;
    requested_enable = MCU_Data.motor_command.enable;

    /* ------------------------------------------------------------------------
     * Validate requested PWM
     * ---------------------------------------------------------------------- */

    if (requested_pwm > MCU_PWM_MAX_PERCENT)
    {
        requested_pwm = MCU_PWM_MAX_PERCENT;
    }

    /* ------------------------------------------------------------------------
     * If command is invalid or motor is disabled, force safe state.
     * ---------------------------------------------------------------------- */

    if ((MCU_Data.motor_command.valid == MCU_FALSE) ||
        (requested_enable == MCU_DISABLED))
    {
        MCU_Motor_SafeStop();
        return;
    }

    /* ------------------------------------------------------------------------
     * Direction validation
     * ---------------------------------------------------------------------- */

    if ((requested_direction != MCU_DIRECTION_FORWARD) &&
        (requested_direction != MCU_DIRECTION_REVERSE) &&
        (requested_direction != MCU_DIRECTION_NEUTRAL))
    {
        MCU_Motor_SafeStop();
        return;
    }

    /* Neutral request means stop */
    if (requested_direction == MCU_DIRECTION_NEUTRAL)
    {
        MCU_Motor_SafeStop();
        return;
    }

    /* ------------------------------------------------------------------------
     * Apply valid command
     * ---------------------------------------------------------------------- */

    MCU_Motor_Enable();
    MCU_Motor_SetDirection(requested_direction);
    MCU_Motor_SetPWM(requested_pwm);
    MCU_Motor_ApplyOutputs();

    MCU_Data.motor.state = MCU_STATE_RUN;
    MCU_Data.motor.enabled = MCU_ENABLED;
    MCU_Data.motor.direction = requested_direction;
    MCU_Data.motor.pwm_percent = requested_pwm;
    MCU_Data.motor.driver_enabled = MCU_ENABLED;
}

/* ============================================================================
 * Enable motor driver
 * ========================================================================== */

void MCU_Motor_Enable(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);

    MCU_Data.output.standby = 1U;
    MCU_Data.motor.driver_enabled = MCU_ENABLED;
}

/* ============================================================================
 * Disable motor driver
 * ========================================================================== */

void MCU_Motor_Disable(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0U);

    MCU_Data.output.standby = 0U;
    MCU_Data.output.pwm_percent = 0U;
    MCU_Data.output.motor_output_enabled = MCU_DISABLED;

    MCU_Data.motor.driver_enabled = MCU_DISABLED;
    MCU_Data.motor.enabled = MCU_DISABLED;
    MCU_Data.motor.pwm_percent = 0U;
}

/* ============================================================================
 * Safe stop
 * ========================================================================== */

void MCU_Motor_SafeStop(void)
{
    MCU_Motor_ForceSafeHardwareState();

    MCU_Data.output.ain1 = 0U;
    MCU_Data.output.ain2 = 0U;
    MCU_Data.output.standby = 0U;
    MCU_Data.output.pwm_percent = 0U;
    MCU_Data.output.motor_output_enabled = MCU_DISABLED;

    MCU_Data.motor.state = MCU_STATE_DISABLED;
    MCU_Data.motor.direction = MCU_DIRECTION_NEUTRAL;
    MCU_Data.motor.enabled = MCU_DISABLED;
    MCU_Data.motor.pwm_percent = 0U;
    MCU_Data.motor.pwm_compare = 0U;
    MCU_Data.motor.driver_enabled = MCU_DISABLED;
}

/* ============================================================================
 * Direction control
 * ========================================================================== */

void MCU_Motor_SetDirection(uint8_t direction)
{
    switch (direction)
    {
        case MCU_DIRECTION_FORWARD:

            HAL_GPIO_WritePin(GPIOB,
                              GPIO_PIN_0,
                              GPIO_PIN_SET);

            HAL_GPIO_WritePin(GPIOB,
                              GPIO_PIN_1,
                              GPIO_PIN_RESET);

            MCU_Data.output.ain1 = 1U;
            MCU_Data.output.ain2 = 0U;
            MCU_Data.motor.direction = MCU_DIRECTION_FORWARD;

            break;

        case MCU_DIRECTION_REVERSE:

            HAL_GPIO_WritePin(GPIOB,
                              GPIO_PIN_0,
                              GPIO_PIN_RESET);

            HAL_GPIO_WritePin(GPIOB,
                              GPIO_PIN_1,
                              GPIO_PIN_SET);

            MCU_Data.output.ain1 = 0U;
            MCU_Data.output.ain2 = 1U;
            MCU_Data.motor.direction = MCU_DIRECTION_REVERSE;

            break;

        case MCU_DIRECTION_NEUTRAL:
        default:

            HAL_GPIO_WritePin(GPIOB,
                              GPIO_PIN_0,
                              GPIO_PIN_RESET);

            HAL_GPIO_WritePin(GPIOB,
                              GPIO_PIN_1,
                              GPIO_PIN_RESET);

            MCU_Data.output.ain1 = 0U;
            MCU_Data.output.ain2 = 0U;
            MCU_Data.motor.direction = MCU_DIRECTION_NEUTRAL;

            break;
    }
}

/* ============================================================================
 * PWM control
 * ========================================================================== */

void MCU_Motor_SetPWM(uint8_t pwm_percent)
{
    uint16_t compare_value;

    if (pwm_percent > MCU_PWM_MAX_PERCENT)
    {
        pwm_percent = MCU_PWM_MAX_PERCENT;
    }

    compare_value = MCU_Motor_PwmPercentToCompare(pwm_percent);

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, compare_value);

    MCU_Data.output.pwm_percent = pwm_percent;
    MCU_Data.output.motor_output_enabled =
        (pwm_percent > 0U) ? MCU_ENABLED : MCU_DISABLED;

    MCU_Data.motor.pwm_percent = pwm_percent;
    MCU_Data.motor.pwm_compare = compare_value;
}

/* ============================================================================
 * PWM percentage to timer compare conversion
 * ========================================================================== */

uint16_t MCU_Motor_PwmPercentToCompare(uint8_t pwm_percent)
{
    uint32_t compare_value;

    if (pwm_percent >= MCU_PWM_MAX_PERCENT)
    {
        return (uint16_t)MCU_PWM_PERIOD;
    }

    compare_value =
        ((uint32_t)pwm_percent * (uint32_t)MCU_PWM_PERIOD) / 100U;

    return (uint16_t)compare_value;
}

/* ============================================================================
 * Apply output data to hardware
 * ========================================================================== */

void MCU_Motor_ApplyOutputs(void)
{
    if (MCU_Data.output.standby != 0U)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
    }

    if (MCU_Data.output.ain1 != 0U)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    }

    if (MCU_Data.output.ain2 != 0U)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
    }

    __HAL_TIM_SET_COMPARE(&htim1,
                          TIM_CHANNEL_1,
                          MCU_Data.motor.pwm_compare);
}

/* ============================================================================
 * Status
 * ========================================================================== */

uint8_t MCU_Motor_IsEnabled(void)
{
    return MCU_Data.motor.enabled;
}

uint8_t MCU_Motor_IsSafe(void)
{
    if ((MCU_Data.motor.enabled == MCU_DISABLED) &&
        (MCU_Data.motor.driver_enabled == MCU_DISABLED) &&
        (MCU_Data.motor.pwm_percent == 0U))
    {
        return MCU_TRUE;
    }

    return MCU_FALSE;
}
