/*
 * vcu_system.c
 *
 *  Created on: 15-Sept-2026
 *      Author: MohithK
 */


#include "vcu_system.h"

#include "gpio.h"
#include "vcu_adc.h"
#include "vcu_can.h"
#include "vcu_faults.h"
#include "vcu_input.h"
#include "vcu_state.h"

/* =========================================================
 * INITIALIZATION
 * ========================================================= */

void VCU_System_Init(void)
{
    /*
     * Initialize the shared application data first.
     */
    VCU_Data_Init();

    /*
     * Initialize input processing.
     */
    VCU_ADC_Init();
    VCU_Input_Init();

    /*
     * Initialize fault and state management.
     */
    VCU_Faults_Init();
    VCU_State_Init();

    /*
     * CAN initialization.
     */
    VCU_CAN_Init();

    /*
     * Start in a safe state.
     */
    VCU_Data.system_state = VCU_STATE_OFF;
    VCU_Data.system_enabled = VCU_DISABLED;

    VCU_Data.outputs.motor_enable = VCU_MOTOR_DISABLE;
    VCU_Data.outputs.contactor_enable = VCU_CONTACTOR_DISABLE;

    VCU_System_UpdateOutputs();
}

/* =========================================================
 * COMPLETE VCU CONTROL UPDATE
 * ========================================================= */

void VCU_System_Update(void)
{
    /*
     * 1. Acquire analog inputs.
     */
    (void)VCU_ADC_Update();

    /*
     * 2. Acquire digital inputs.
     */
    VCU_Input_UpdateDigital();

    /*
     * 3. Process throttle, brake and direction.
     */
    VCU_Input_Process();

    /*
     * 4. Evaluate faults.
     */
    VCU_Faults_Update();

    /*
     * 5. Update the VCU state machine.
     */
    VCU_State_Update();

    /*
     * 6. Apply resulting outputs.
     */
    VCU_System_UpdateOutputs();
}

/* =========================================================
 * OUTPUT UPDATE
 * ========================================================= */

void VCU_System_UpdateOutputs(void)
{
    GPIO_PinState status_led_state;
    GPIO_PinState fault_led_state;
    GPIO_PinState motor_enable_state;
    GPIO_PinState contactor_enable_state;

    /*
     * Status LED is ON in normal operational states.
     */
    if ((VCU_Data.system_state == VCU_STATE_READY) ||
        (VCU_Data.system_state == VCU_STATE_DRIVE) ||
        (VCU_Data.system_state == VCU_STATE_BRAKE))
    {
        status_led_state = GPIO_PIN_SET;
    }
    else
    {
        status_led_state = GPIO_PIN_RESET;
    }

    /*
     * Fault LED is ON whenever a fault is active.
     */
    if (VCU_Data.faults.active != 0U)
    {
        fault_led_state = GPIO_PIN_SET;
    }
    else
    {
        fault_led_state = GPIO_PIN_RESET;
    }

    /*
     * Motor enable output.
     */
    if (VCU_Data.outputs.motor_enable != VCU_MOTOR_DISABLE)
    {
        motor_enable_state = GPIO_PIN_SET;
    }
    else
    {
        motor_enable_state = GPIO_PIN_RESET;
    }

    /*
     * Contactor enable output.
     */
    if (VCU_Data.outputs.contactor_enable !=
        VCU_CONTACTOR_DISABLE)
    {
        contactor_enable_state = GPIO_PIN_SET;
    }
    else
    {
        contactor_enable_state = GPIO_PIN_RESET;
    }

    /*
     * Write outputs to the STM32 GPIO pins.
     */
    HAL_GPIO_WritePin(
        GPIOB,
        GPIO_PIN_0,
        status_led_state);

    HAL_GPIO_WritePin(
        GPIOB,
        GPIO_PIN_1,
        fault_led_state);

    HAL_GPIO_WritePin(
        GPIOB,
        GPIO_PIN_8,
        motor_enable_state);

    HAL_GPIO_WritePin(
        GPIOB,
        GPIO_PIN_9,
        contactor_enable_state);
}

/* =========================================================
 * SYSTEM HEALTH
 * ========================================================= */

uint8_t VCU_System_IsHealthy(void)
{
    /*
     * Any active fault makes the system unhealthy.
     */
    if (VCU_Data.faults.active != 0U)
    {
        return 0U;
    }

    /*
     * BMS communication must be valid.
     */
    if (VCU_Data.bms.communication_valid == 0U)
    {
        return 0U;
    }

    /*
     * MCU communication must be valid.
     */
    if (VCU_Data.mcu.communication_valid == 0U)
    {
        return 0U;
    }

    /*
     * Driver inputs must be plausible.
     */
    if (VCU_Input_IsPlausible() == 0U)
    {
        return 0U;
    }

    return 1U;
}
