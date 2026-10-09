#include "vcu_state.h"

#include "vcu_input.h"
#include "vcu_faults.h"

/* =========================================================
 * PRIVATE STATE REQUEST FLAGS
 * ========================================================= */

static uint8_t vcu_enable_request = 0U;
static uint8_t vcu_disable_request = 0U;
static uint8_t vcu_shutdown_request = 0U;
static uint8_t vcu_recovery_request = 0U;

/* =========================================================
 * PRIVATE HELPERS
 * ========================================================= */

static uint8_t VCU_State_IsSafetyReady(void)
{
    /*
     * Ignition must be active.
     */
    if (VCU_Data.input.digital.ignition != VCU_INPUT_ACTIVE)
    {
        return 0U;
    }

    /*
     * Emergency input must be inactive.
     */
    if (VCU_Data.input.digital.emergency != VCU_INPUT_INACTIVE)
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

    /*
     * BMS communication must be valid.
     */
    if (VCU_Data.bms.communication_valid == 0U)
    {
        return 0U;
    }

    /*
     * BMS must not report a fault.
     */
    if (VCU_Data.bms.fault_status != VCU_FAULT_NONE)
    {
        return 0U;
    }

    /*
     * MCU communication must be valid before drive.
     */
    if (VCU_Data.mcu.communication_valid == 0U)
    {
        return 0U;
    }

    /*
     * MCU must not report a fault.
     */
    if (VCU_Data.mcu.fault_status != VCU_FAULT_NONE)
    {
        return 0U;
    }

    return 1U;
}

/* =========================================================
 * INITIALIZATION
 * ========================================================= */

void VCU_State_Init(void)
{
    vcu_enable_request = 0U;
    vcu_disable_request = 0U;
    vcu_shutdown_request = 0U;
    vcu_recovery_request = 0U;

    VCU_Data.system_state = VCU_STATE_INIT;
    VCU_Data.system_enabled = VCU_DISABLED;

    VCU_Data.outputs.motor_enable = VCU_MOTOR_DISABLE;
    VCU_Data.outputs.contactor_enable = VCU_CONTACTOR_DISABLE;
}

/* =========================================================
 * STATE UPDATE
 * ========================================================= */

void VCU_State_Update(void)
{
    /*
     * Emergency input has the highest software priority.
     */
    if (VCU_Input_IsEmergencyActive() != 0U)
    {
        VCU_Data.system_state = VCU_STATE_FAULT;
        VCU_Data.system_enabled = VCU_DISABLED;

        VCU_Data.outputs.motor_enable = VCU_MOTOR_DISABLE;
        VCU_Data.outputs.contactor_enable = VCU_CONTACTOR_DISABLE;

        return;
    }

    /*
     * Explicit shutdown request.
     */
    if (vcu_shutdown_request != 0U)
    {
        VCU_Data.system_state = VCU_STATE_SHUTDOWN;
        VCU_Data.system_enabled = VCU_DISABLED;

        VCU_Data.outputs.motor_enable = VCU_MOTOR_DISABLE;
        VCU_Data.outputs.contactor_enable = VCU_CONTACTOR_DISABLE;

        vcu_shutdown_request = 0U;

        return;
    }

    /*
     * Existing fault handling.
     */
    if (VCU_State_IsFaultActive() != 0U)
    {
        VCU_Data.system_state = VCU_STATE_FAULT;
        VCU_Data.system_enabled = VCU_DISABLED;

        VCU_Data.outputs.motor_enable = VCU_MOTOR_DISABLE;
        VCU_Data.outputs.contactor_enable = VCU_CONTACTOR_DISABLE;

        /*
         * Only a fault recovery request can leave FAULT.
         */
        if (vcu_recovery_request != 0U)
        {
            if (VCU_Faults_CanRecover() != 0U)
            {
                vcu_recovery_request = 0U;
                VCU_Data.faults.active = 0U;
                VCU_Data.system_state = VCU_STATE_OFF;
            }
        }

        return;
    }

    /*
     * Explicit disable request.
     */
    if (vcu_disable_request != 0U)
    {
        VCU_Data.system_enabled = VCU_DISABLED;

        VCU_Data.outputs.motor_enable = VCU_MOTOR_DISABLE;
        VCU_Data.outputs.contactor_enable = VCU_CONTACTOR_DISABLE;

        VCU_Data.system_state = VCU_STATE_OFF;

        vcu_disable_request = 0U;

        return;
    }

    /*
     * Ignition OFF means the vehicle is OFF.
     */
    if (VCU_Input_IsIgnitionActive() == 0U)
    {
        VCU_Data.system_enabled = VCU_DISABLED;

        VCU_Data.outputs.motor_enable = VCU_MOTOR_DISABLE;
        VCU_Data.outputs.contactor_enable = VCU_CONTACTOR_DISABLE;

        VCU_Data.system_state = VCU_STATE_OFF;

        return;
    }

    /*
     * Check whether the complete system is ready.
     */
    if (VCU_State_IsSafetyReady() == 0U)
    {
        VCU_Data.system_enabled = VCU_DISABLED;

        VCU_Data.outputs.motor_enable = VCU_MOTOR_DISABLE;
        VCU_Data.outputs.contactor_enable = VCU_CONTACTOR_DISABLE;

        VCU_Data.system_state = VCU_STATE_WARNING;

        return;
    }

    /*
     * Process enable request.
     */
    if (vcu_enable_request != 0U)
    {
        VCU_Data.system_enabled = VCU_ENABLED;

        VCU_Data.outputs.contactor_enable = VCU_CONTACTOR_ENABLE;

        vcu_enable_request = 0U;
    }

    /*
     * If the system is not enabled, remain READY.
     */
    if (VCU_Data.system_enabled == VCU_DISABLED)
    {
        VCU_Data.outputs.motor_enable = VCU_MOTOR_DISABLE;
        VCU_Data.outputs.contactor_enable = VCU_CONTACTOR_DISABLE;

        VCU_Data.system_state = VCU_STATE_READY;

        return;
    }

    /*
     * Brake state has priority over drive state.
     */
    if (VCU_Data.input.brake_request != 0U)
    {
        VCU_Data.outputs.motor_enable = VCU_MOTOR_DISABLE;
        VCU_Data.outputs.contactor_enable = VCU_CONTACTOR_ENABLE;

        VCU_Data.system_state = VCU_STATE_BRAKE;

        return;
    }

    /*
     * Drive is permitted only with a valid direction
     * and accelerator request.
     */
    if ((VCU_Data.input.accelerator_request != 0U) &&
        (VCU_Data.input.direction != VCU_DIR_NEUTRAL))
    {
        VCU_Data.outputs.motor_enable = VCU_MOTOR_ENABLE;
        VCU_Data.outputs.contactor_enable = VCU_CONTACTOR_ENABLE;

        VCU_Data.system_state = VCU_STATE_DRIVE;

        return;
    }

    /*
     * System is enabled but no drive command is active.
     */
    VCU_Data.outputs.motor_enable = VCU_MOTOR_DISABLE;
    VCU_Data.outputs.contactor_enable = VCU_CONTACTOR_ENABLE;

    VCU_Data.system_state = VCU_STATE_READY;
}

/* =========================================================
 * ENABLE REQUEST
 * ========================================================= */

void VCU_State_RequestEnable(void)
{
    vcu_enable_request = 1U;
}

/* =========================================================
 * DISABLE REQUEST
 * ========================================================= */

void VCU_State_RequestDisable(void)
{
    vcu_disable_request = 1U;
}

/* =========================================================
 * SHUTDOWN REQUEST
 * ========================================================= */

void VCU_State_RequestShutdown(void)
{
    vcu_shutdown_request = 1U;
}

/* =========================================================
 * FAULT RECOVERY REQUEST
 * ========================================================= */

void VCU_State_RequestRecovery(void)
{
    vcu_recovery_request = 1U;
}

/* =========================================================
 * GET CURRENT STATE
 * ========================================================= */

VCU_SystemState_t VCU_State_Get(void)
{
    return VCU_Data.system_state;
}

/* =========================================================
 * DRIVE ALLOWED
 * ========================================================= */

uint8_t VCU_State_IsDriveAllowed(void)
{
    if (VCU_Data.system_state == VCU_STATE_DRIVE)
    {
        return 1U;
    }

    return 0U;
}

/* =========================================================
 * FAULT ACTIVE
 * ========================================================= */

uint8_t VCU_State_IsFaultActive(void)
{
    if (VCU_Data.faults.active != 0U)
    {
        return 1U;
    }

    return 0U;
}

/* =========================================================
 * WARNING ACTIVE
 * ========================================================= */

uint8_t VCU_State_IsWarningActive(void)
{
    if (VCU_Data.system_state == VCU_STATE_WARNING)
    {
        return 1U;
    }

    return 0U;
}
