/*
 * mcu_faults.c
 *
 *  Created on: 21-Sept-2026
 *      Author: MohithK
 */


#include "main.h"
#include "mcu_config.h"
#include "mcu_data.h"
#include "mcu_can.h"
#include "mcu_hall.h"
#include "mcu_motor.h"
#include "mcu_input.h"
#include "mcu_state.h"
#include "mcu_faults.h"

/* ============================================================================
 * MCU Fault Management
 * ============================================================================
 *
 * This module:
 *
 * 1. Monitors subsystem health.
 * 2. Assigns a fault code and severity.
 * 3. Latches serious faults.
 * 4. Requests the state machine to enter FAULT.
 * 5. Handles explicit fault reset/recovery.
 *
 * Fault sources:
 *
 * CAN
 * VCU communication
 * VCU command
 * Hall sensor
 * Hall timeout
 * Direction conflict
 * Motor driver
 * Potentiometer
 * Buttons
 *
 * ========================================================================== */

/* ============================================================================
 * Private helper functions
 * ========================================================================== */

static void MCU_Faults_Apply(uint8_t code,
                             uint8_t severity)
{
    /*
     * Remember the previous fault code whenever the active fault changes.
     */
    if (MCU_Data.fault.code != code)
    {
        MCU_Data.fault.previous_code =
            MCU_Data.fault.code;

        MCU_Data.fault.confirmation_count = 0U;
    }

    MCU_Data.fault.code = code;
    MCU_Data.fault.severity = severity;
    MCU_Data.fault.active = MCU_TRUE;

    /*
     * Serious faults are latched.
     */
    if (severity >= MCU_FAULT_SEVERITY_FAULT)
    {
        MCU_Data.fault.latched = MCU_TRUE;
    }

    if (MCU_Data.fault.confirmation_count < 255U)
    {
        MCU_Data.fault.confirmation_count++;
    }
}

/* -------------------------------------------------------------------------- */

static void MCU_Faults_ClearIfNoFaults(void)
{
    if (MCU_Data.fault.can_fault)
    {
        return;
    }

    if (MCU_Data.fault.vcu_comm_fault)
    {
        return;
    }

    if (MCU_Data.fault.command_fault)
    {
        return;
    }

    if (MCU_Data.fault.hall_fault)
    {
        return;
    }

    if (MCU_Data.fault.direction_fault)
    {
        return;
    }

    if (MCU_Data.fault.motor_driver_fault)
    {
        return;
    }

    if (MCU_Data.fault.potentiometer_fault)
    {
        return;
    }

    if (MCU_Data.fault.button_fault)
    {
        return;
    }

    /*
     * No active fault flags remain.
     *
     * Do not automatically clear a latched fault. An explicit
     * reset command is required.
     */
    if (!MCU_Data.fault.latched)
    {
        MCU_Data.fault.active = MCU_FALSE;
        MCU_Data.fault.code = MCU_FAULT_NONE;
        MCU_Data.fault.severity =
            MCU_FAULT_SEVERITY_NONE;
    }
}

/* ============================================================================
 * Initialization
 * ========================================================================== */

void MCU_Faults_Init(void)
{
    MCU_Data.fault.active = MCU_FALSE;
    MCU_Data.fault.latched = MCU_FALSE;

    MCU_Data.fault.code = MCU_FAULT_NONE;
    MCU_Data.fault.severity =
        MCU_FAULT_SEVERITY_NONE;

    MCU_Data.fault.previous_code =
        MCU_FAULT_NONE;

    MCU_Data.fault.can_fault = MCU_FALSE;
    MCU_Data.fault.vcu_comm_fault = MCU_FALSE;
    MCU_Data.fault.command_fault = MCU_FALSE;
    MCU_Data.fault.hall_fault = MCU_FALSE;
    MCU_Data.fault.direction_fault = MCU_FALSE;
    MCU_Data.fault.motor_driver_fault = MCU_FALSE;
    MCU_Data.fault.potentiometer_fault = MCU_FALSE;
    MCU_Data.fault.button_fault = MCU_FALSE;

    MCU_Data.fault.reset_requested = MCU_FALSE;

    MCU_Data.fault.confirmation_count = 0U;
    MCU_Data.fault.recovery_count = 0U;
}

/* ============================================================================
 * Periodic fault update
 * ========================================================================== */

void MCU_Faults_Update(void)
{
    MCU_Faults_Evaluate();

    /*
     * A reset request is processed separately so that all safety
     * checks have already been evaluated before attempting recovery.
     */
    if (MCU_Data.fault.reset_requested)
    {
        MCU_Faults_ProcessRecovery();
    }
}

/* ============================================================================
 * Fault evaluation
 * ========================================================================== */

void MCU_Faults_Evaluate(void)
{
    uint8_t motor_running;

    motor_running =
        (MCU_Data.motor.state == MCU_STATE_RUN) ?
        MCU_TRUE : MCU_FALSE;

    /* ------------------------------------------------------------------------
     * CAN fault
     * ---------------------------------------------------------------------- */

    if (MCU_Data.fault.can_fault)
    {
        MCU_Faults_Apply(
            MCU_FAULT_CAN_RX,
            MCU_FAULT_SEVERITY_FAULT);

        MCU_State_RequestFault();
        return;
    }

    /* ------------------------------------------------------------------------
     * VCU communication fault
     *
     * In local-test mode, VCU communication is intentionally not required.
     * ---------------------------------------------------------------------- */

    if (!MCU_LOCAL_TEST_MODE_ENABLED &&
        MCU_Data.fault.vcu_comm_fault)
    {
        MCU_Faults_Apply(
            MCU_FAULT_VCU_COMM,
            MCU_FAULT_SEVERITY_FAULT);

        MCU_State_RequestFault();
        return;
    }

    /* ------------------------------------------------------------------------
     * Invalid VCU command
     * ---------------------------------------------------------------------- */

    if (MCU_Data.fault.command_fault)
    {
        MCU_Faults_Apply(
            MCU_FAULT_CAN_COMMAND,
            MCU_FAULT_SEVERITY_FAULT);

        MCU_State_RequestFault();
        return;
    }

    /* ------------------------------------------------------------------------
     * Direction conflict
     * ---------------------------------------------------------------------- */

    if (MCU_Data.fault.direction_fault)
    {
        MCU_Faults_Apply(
            MCU_FAULT_DIRECTION,
            MCU_FAULT_SEVERITY_FAULT);

        MCU_State_RequestFault();
        return;
    }

    /* ------------------------------------------------------------------------
     * Button fault
     * ---------------------------------------------------------------------- */

    if (MCU_Data.fault.button_fault)
    {
        MCU_Faults_Apply(
            MCU_FAULT_BUTTON,
            MCU_FAULT_SEVERITY_FAULT);

        MCU_State_RequestFault();
        return;
    }

    /* ------------------------------------------------------------------------
     * Hall sensor signal fault
     * ---------------------------------------------------------------------- */

    if (MCU_Data.fault.hall_fault)
    {
        MCU_Faults_Apply(
            MCU_FAULT_HALL_SIGNAL,
            MCU_FAULT_SEVERITY_FAULT);

        MCU_State_RequestFault();
        return;
    }

    /* ------------------------------------------------------------------------
     * Hall timeout
     *
     * Only relevant while the motor is expected to rotate.
     * ---------------------------------------------------------------------- */

    if (motor_running &&
        MCU_Hall_IsTimeout())
    {
        MCU_Data.fault.hall_fault = MCU_TRUE;

        MCU_Faults_Apply(
            MCU_FAULT_HALL_TIMEOUT,
            MCU_FAULT_SEVERITY_FAULT);

        MCU_State_RequestFault();
        return;
    }

    /* ------------------------------------------------------------------------
     * Motor driver fault
     * ---------------------------------------------------------------------- */

    if (MCU_Data.fault.motor_driver_fault)
    {
        MCU_Faults_Apply(
            MCU_FAULT_MOTOR_DRIVER,
            MCU_FAULT_SEVERITY_FAULT);

        MCU_State_RequestFault();
        return;
    }

    /* ------------------------------------------------------------------------
     * Potentiometer fault
     *
     * Potentiometer validity is required for the current local-test mode.
     * ---------------------------------------------------------------------- */

    if (!MCU_Data.adc.valid)
    {
        MCU_Data.fault.potentiometer_fault = MCU_TRUE;
    }
    else
    {
        MCU_Data.fault.potentiometer_fault = MCU_FALSE;
    }

    if (MCU_Data.fault.potentiometer_fault)
    {
        MCU_Faults_Apply(
            MCU_FAULT_POTENTIOMETER,
            MCU_FAULT_SEVERITY_FAULT);

        MCU_State_RequestFault();
        return;
    }

    /* ------------------------------------------------------------------------
     * If no fault remains, update/clear the general fault status.
     * ---------------------------------------------------------------------- */

    MCU_Faults_ClearIfNoFaults();
}

/* ============================================================================
 * Individual fault setters
 * ========================================================================== */

void MCU_Faults_SetCANFault(void)
{
    MCU_Data.fault.can_fault = MCU_TRUE;

    MCU_Faults_Apply(
        MCU_FAULT_CAN_RX,
        MCU_FAULT_SEVERITY_FAULT);

    MCU_State_RequestFault();
}

/* -------------------------------------------------------------------------- */

void MCU_Faults_SetVCUCommFault(void)
{
    MCU_Data.fault.vcu_comm_fault = MCU_TRUE;

    MCU_Faults_Apply(
        MCU_FAULT_VCU_COMM,
        MCU_FAULT_SEVERITY_FAULT);

    MCU_State_RequestFault();
}

/* -------------------------------------------------------------------------- */

void MCU_Faults_SetCommandFault(void)
{
    MCU_Data.fault.command_fault = MCU_TRUE;

    MCU_Faults_Apply(
        MCU_FAULT_CAN_COMMAND,
        MCU_FAULT_SEVERITY_FAULT);

    MCU_State_RequestFault();
}

/* -------------------------------------------------------------------------- */

void MCU_Faults_SetHallFault(void)
{
    MCU_Data.fault.hall_fault = MCU_TRUE;

    MCU_Faults_Apply(
        MCU_FAULT_HALL_SIGNAL,
        MCU_FAULT_SEVERITY_FAULT);

    MCU_State_RequestFault();
}

/* -------------------------------------------------------------------------- */

void MCU_Faults_SetHallTimeoutFault(void)
{
    /*
     * The data structure has one Hall fault flag, so Hall timeout is
     * represented by the same hall_fault flag with a distinct fault code.
     */
    MCU_Data.fault.hall_fault = MCU_TRUE;

    MCU_Faults_Apply(
        MCU_FAULT_HALL_TIMEOUT,
        MCU_FAULT_SEVERITY_FAULT);

    MCU_State_RequestFault();
}

/* -------------------------------------------------------------------------- */

void MCU_Faults_SetDirectionFault(void)
{
    MCU_Data.fault.direction_fault = MCU_TRUE;

    MCU_Faults_Apply(
        MCU_FAULT_DIRECTION,
        MCU_FAULT_SEVERITY_FAULT);

    MCU_State_RequestFault();
}

/* -------------------------------------------------------------------------- */

void MCU_Faults_SetMotorDriverFault(void)
{
    MCU_Data.fault.motor_driver_fault = MCU_TRUE;

    MCU_Faults_Apply(
        MCU_FAULT_MOTOR_DRIVER,
        MCU_FAULT_SEVERITY_FAULT);

    MCU_State_RequestFault();
}

/* -------------------------------------------------------------------------- */

void MCU_Faults_SetPotentiometerFault(void)
{
    MCU_Data.fault.potentiometer_fault = MCU_TRUE;

    MCU_Faults_Apply(
        MCU_FAULT_POTENTIOMETER,
        MCU_FAULT_SEVERITY_FAULT);

    MCU_State_RequestFault();
}

/* -------------------------------------------------------------------------- */

void MCU_Faults_SetButtonFault(void)
{
    MCU_Data.fault.button_fault = MCU_TRUE;

    MCU_Faults_Apply(
        MCU_FAULT_BUTTON,
        MCU_FAULT_SEVERITY_FAULT);

    MCU_State_RequestFault();
}

/* -------------------------------------------------------------------------- */

void MCU_Faults_SetSystemFault(void)
{
    MCU_Faults_Apply(
        MCU_FAULT_SYSTEM,
        MCU_FAULT_SEVERITY_CRITICAL);

    MCU_State_RequestFault();
}

/* ============================================================================
 * Individual fault clear functions
 * ========================================================================== */

void MCU_Faults_ClearCANFault(void)
{
    MCU_Data.fault.can_fault = MCU_FALSE;
    MCU_Faults_ClearIfNoFaults();
}

/* -------------------------------------------------------------------------- */

void MCU_Faults_ClearVCUCommFault(void)
{
    MCU_Data.fault.vcu_comm_fault = MCU_FALSE;
    MCU_Faults_ClearIfNoFaults();
}

/* -------------------------------------------------------------------------- */

void MCU_Faults_ClearCommandFault(void)
{
    MCU_Data.fault.command_fault = MCU_FALSE;
    MCU_Faults_ClearIfNoFaults();
}

/* -------------------------------------------------------------------------- */

void MCU_Faults_ClearHallFault(void)
{
    MCU_Data.fault.hall_fault = MCU_FALSE;
    MCU_Faults_ClearIfNoFaults();
}

/* -------------------------------------------------------------------------- */

void MCU_Faults_ClearHallTimeoutFault(void)
{
    MCU_Data.fault.hall_fault = MCU_FALSE;
    MCU_Faults_ClearIfNoFaults();
}

/* -------------------------------------------------------------------------- */

void MCU_Faults_ClearDirectionFault(void)
{
    MCU_Data.fault.direction_fault = MCU_FALSE;
    MCU_Faults_ClearIfNoFaults();
}

/* -------------------------------------------------------------------------- */

void MCU_Faults_ClearMotorDriverFault(void)
{
    MCU_Data.fault.motor_driver_fault = MCU_FALSE;
    MCU_Faults_ClearIfNoFaults();
}

/* -------------------------------------------------------------------------- */

void MCU_Faults_ClearPotentiometerFault(void)
{
    MCU_Data.fault.potentiometer_fault = MCU_FALSE;
    MCU_Faults_ClearIfNoFaults();
}

/* -------------------------------------------------------------------------- */

void MCU_Faults_ClearButtonFault(void)
{
    MCU_Data.fault.button_fault = MCU_FALSE;
    MCU_Faults_ClearIfNoFaults();
}

/* -------------------------------------------------------------------------- */

void MCU_Faults_ClearSystemFault(void)
{
    /*
     * System fault does not have a dedicated flag in the current
     * MCU_Data fault structure. The general fault status is cleared
     * only through an explicit reset/recovery.
     */
    MCU_Faults_ClearIfNoFaults();
}

/* ============================================================================
 * Reset request
 * ========================================================================== */

void MCU_Faults_RequestReset(void)
{
    MCU_Data.fault.reset_requested = MCU_TRUE;
}

/* ============================================================================
 * Complete reset
 * ========================================================================== */

void MCU_Faults_Reset(void)
{
    MCU_Data.fault.active = MCU_FALSE;
    MCU_Data.fault.latched = MCU_FALSE;

    MCU_Data.fault.previous_code =
        MCU_Data.fault.code;

    MCU_Data.fault.code = MCU_FAULT_NONE;

    MCU_Data.fault.severity =
        MCU_FAULT_SEVERITY_NONE;

    MCU_Data.fault.can_fault = MCU_FALSE;
    MCU_Data.fault.vcu_comm_fault = MCU_FALSE;
    MCU_Data.fault.command_fault = MCU_FALSE;
    MCU_Data.fault.hall_fault = MCU_FALSE;
    MCU_Data.fault.direction_fault = MCU_FALSE;
    MCU_Data.fault.motor_driver_fault = MCU_FALSE;
    MCU_Data.fault.potentiometer_fault = MCU_FALSE;
    MCU_Data.fault.button_fault = MCU_FALSE;

    MCU_Data.fault.reset_requested = MCU_FALSE;

    if (MCU_Data.fault.recovery_count < 255U)
    {
        MCU_Data.fault.recovery_count++;
    }
}

/* ============================================================================
 * Fault status
 * ========================================================================== */

uint8_t MCU_Faults_IsActive(void)
{
    return MCU_Data.fault.active;
}

uint8_t MCU_Faults_IsLatched(void)
{
    return MCU_Data.fault.latched;
}

uint8_t MCU_Faults_GetCode(void)
{
    return MCU_Data.fault.code;
}

uint8_t MCU_Faults_GetSeverity(void)
{
    return MCU_Data.fault.severity;
}

/* ============================================================================
 * Recovery permission
 * ========================================================================== */

uint8_t MCU_Faults_CanRecover(void)
{
    /*
     * Shutdown cannot be recovered by the normal fault-reset path.
     */
    if (MCU_State_IsShutdown())
    {
        return MCU_FALSE;
    }

    /*
     * A direction conflict must be removed before recovery.
     */
    if (MCU_Input_IsDirectionConflict())
    {
        return MCU_FALSE;
    }

    /*
     * CAN hardware must be healthy.
     */
    if (!MCU_Data.task.can_ok)
    {
        return MCU_FALSE;
    }

    /*
     * ADC must be valid.
     */
    if (!MCU_Data.adc.valid)
    {
        return MCU_FALSE;
    }

    /*
     * Motor subsystem must be healthy.
     */
    if (!MCU_Data.task.motor_ok)
    {
        return MCU_FALSE;
    }

    /*
     * In normal networked operation, VCU communication must be
     * restored before recovery.
     */
    if (!MCU_LOCAL_TEST_MODE_ENABLED)
    {
        if (!MCU_Data.vcu_comm.communication_valid)
        {
            return MCU_FALSE;
        }
    }

    /*
     * Motor must already be safely stopped before clearing a fault.
     */
    if (MCU_Data.motor.enabled)
    {
        return MCU_FALSE;
    }

    return MCU_TRUE;
}

/* ============================================================================
 * Recovery processing
 * ========================================================================== */

uint8_t MCU_Faults_ProcessRecovery(void)
{
    if (!MCU_Data.fault.reset_requested)
    {
        return MCU_FALSE;
    }

    /*
     * Do not clear the request until recovery conditions have been checked.
     */
    if (!MCU_Faults_CanRecover())
    {
        return MCU_FALSE;
    }

    MCU_Faults_Reset();

    return MCU_TRUE;
}

/* ============================================================================
 * Confirmation count
 * ========================================================================== */

uint8_t MCU_Faults_GetConfirmationCount(void)
{
    return MCU_Data.fault.confirmation_count;
}

/* ============================================================================
 * Recovery count
 * ========================================================================== */

uint8_t MCU_Faults_GetRecoveryCount(void)
{
    return MCU_Data.fault.recovery_count;
}
