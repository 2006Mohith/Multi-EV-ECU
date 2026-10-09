/*
 * vcu_faults.c
 *
 *  Created on: 15-Sept-2026
 *      Author: MohithK
 */


#include "vcu_faults.h"

#include "vcu_config.h"
#include "vcu_input.h"

/* =========================================================
 * PRIVATE HELPERS
 * ========================================================= */

static void VCU_Faults_UpdateSummary(void)
{
    VCU_Data.faults.active = 0U;
    VCU_Data.faults.code = VCU_FAULT_NONE;
    VCU_Data.faults.severity = VCU_FAULT_SEVERITY_NONE;

    /*
     * Critical faults are evaluated first.
     */

    if (VCU_Data.faults.emergency_fault != 0U)
    {
        VCU_Data.faults.active = 1U;
        VCU_Data.faults.code = VCU_FAULT_EMERGENCY;
        VCU_Data.faults.severity = VCU_FAULT_SEVERITY_CRITICAL;
        return;
    }

    if (VCU_Data.faults.bms_critical_fault != 0U)
    {
        VCU_Data.faults.active = 1U;
        VCU_Data.faults.code = VCU_FAULT_BMS_CRITICAL;
        VCU_Data.faults.severity = VCU_FAULT_SEVERITY_CRITICAL;
        return;
    }

    if (VCU_Data.faults.mcu_critical_fault != 0U)
    {
        VCU_Data.faults.active = 1U;
        VCU_Data.faults.code = VCU_FAULT_MCU_CRITICAL;
        VCU_Data.faults.severity = VCU_FAULT_SEVERITY_CRITICAL;
        return;
    }

    if (VCU_Data.faults.throttle_fault != 0U)
    {
        VCU_Data.faults.active = 1U;
        VCU_Data.faults.code = VCU_FAULT_THROTTLE;
        VCU_Data.faults.severity = VCU_FAULT_SEVERITY_CRITICAL;
        return;
    }

    if (VCU_Data.faults.brake_fault != 0U)
    {
        VCU_Data.faults.active = 1U;
        VCU_Data.faults.code = VCU_FAULT_BRAKE;
        VCU_Data.faults.severity = VCU_FAULT_SEVERITY_CRITICAL;
        return;
    }

    if (VCU_Data.faults.direction_fault != 0U)
    {
        VCU_Data.faults.active = 1U;
        VCU_Data.faults.code = VCU_FAULT_DIRECTION;
        VCU_Data.faults.severity = VCU_FAULT_SEVERITY_CRITICAL;
        return;
    }

    if (VCU_Data.faults.ignition_fault != 0U)
    {
        VCU_Data.faults.active = 1U;
        VCU_Data.faults.code = VCU_FAULT_IGNITION;
        VCU_Data.faults.severity = VCU_FAULT_SEVERITY_CRITICAL;
        return;
    }

    /*
     * Communication faults are treated as critical because
     * the VCU must not command the vehicle without valid
     * BMS/MCU communication.
     */

    if (VCU_Data.faults.bms_communication_fault != 0U)
    {
        VCU_Data.faults.active = 1U;
        VCU_Data.faults.code = VCU_FAULT_BMS_COMMUNICATION;
        VCU_Data.faults.severity = VCU_FAULT_SEVERITY_CRITICAL;
        return;
    }

    if (VCU_Data.faults.mcu_communication_fault != 0U)
    {
        VCU_Data.faults.active = 1U;
        VCU_Data.faults.code = VCU_FAULT_MCU_COMMUNICATION;
        VCU_Data.faults.severity = VCU_FAULT_SEVERITY_CRITICAL;
        return;
    }
}

/* =========================================================
 * INITIALIZATION
 * ========================================================= */

void VCU_Faults_Init(void)
{
    VCU_Data.faults.active = 0U;
    VCU_Data.faults.code = VCU_FAULT_NONE;
    VCU_Data.faults.severity = VCU_FAULT_SEVERITY_NONE;

    VCU_Data.faults.bms_communication_fault = 0U;
    VCU_Data.faults.bms_critical_fault = 0U;

    VCU_Data.faults.mcu_communication_fault = 0U;
    VCU_Data.faults.mcu_critical_fault = 0U;

    VCU_Data.faults.throttle_fault = 0U;
    VCU_Data.faults.brake_fault = 0U;

    VCU_Data.faults.direction_fault = 0U;
    VCU_Data.faults.emergency_fault = 0U;
    VCU_Data.faults.ignition_fault = 0U;
}

/* =========================================================
 * FAULT UPDATE
 * ========================================================= */

void VCU_Faults_Update(void)
{
    VCU_Data.faults.bms_communication_fault =
        VCU_Faults_CheckBMSCommunication();

    VCU_Data.faults.bms_critical_fault =
        VCU_Faults_CheckBMSCritical();

    VCU_Data.faults.mcu_communication_fault =
        VCU_Faults_CheckMCUCommunication();

    VCU_Data.faults.mcu_critical_fault =
        VCU_Faults_CheckMCUCritical();

    VCU_Data.faults.throttle_fault =
        VCU_Faults_CheckThrottle();

    VCU_Data.faults.brake_fault =
        VCU_Faults_CheckBrake();

    VCU_Data.faults.direction_fault =
        VCU_Faults_CheckDirection();

    VCU_Data.faults.emergency_fault =
        VCU_Faults_CheckEmergency();

    VCU_Data.faults.ignition_fault =
        VCU_Faults_CheckIgnition();

    VCU_Faults_UpdateSummary();
}

/* =========================================================
 * BMS COMMUNICATION FAULT
 * ========================================================= */

uint8_t VCU_Faults_CheckBMSCommunication(void)
{
    if (VCU_Data.bms.communication_valid == 0U)
    {
        return 1U;
    }

    return 0U;
}

/* =========================================================
 * BMS CRITICAL FAULT
 * ========================================================= */

uint8_t VCU_Faults_CheckBMSCritical(void)
{
    if (VCU_Data.bms.fault_status != VCU_FAULT_NONE)
    {
        return 1U;
    }

    return 0U;
}

/* =========================================================
 * MCU COMMUNICATION FAULT
 * ========================================================= */

uint8_t VCU_Faults_CheckMCUCommunication(void)
{
    if (VCU_Data.mcu.communication_valid == 0U)
    {
        return 1U;
    }

    return 0U;
}

/* =========================================================
 * MCU CRITICAL FAULT
 * ========================================================= */

uint8_t VCU_Faults_CheckMCUCritical(void)
{
    if (VCU_Data.mcu.fault_status != VCU_FAULT_NONE)
    {
        return 1U;
    }

    return 0U;
}

/* =========================================================
 * THROTTLE FAULT
 * ========================================================= */

uint8_t VCU_Faults_CheckThrottle(void)
{
    if (VCU_Data.input.analog.throttle_valid != VCU_INPUT_VALID)
    {
        return 1U;
    }

    if (VCU_Data.input.analog.throttle_percent <
        VCU_THROTTLE_MIN_PERCENT)
    {
        return 1U;
    }

    if (VCU_Data.input.analog.throttle_percent >
        VCU_THROTTLE_MAX_PERCENT)
    {
        return 1U;
    }

    return 0U;
}

/* =========================================================
 * BRAKE FAULT
 * ========================================================= */

uint8_t VCU_Faults_CheckBrake(void)
{
    if (VCU_Data.input.analog.brake_valid != VCU_INPUT_VALID)
    {
        return 1U;
    }

    if (VCU_Data.input.analog.brake_percent <
        VCU_BRAKE_MIN_PERCENT)
    {
        return 1U;
    }

    if (VCU_Data.input.analog.brake_percent >
        VCU_BRAKE_MAX_PERCENT)
    {
        return 1U;
    }

    /*
     * Simultaneous heavy throttle and brake is implausible.
     */
    if ((VCU_Data.input.analog.brake_percent >
         VCU_BRAKE_DEADZONE_PERCENT) &&
        (VCU_Data.input.analog.throttle_percent >
         VCU_MAX_THROTTLE_WHILE_BRAKING))
    {
        return 1U;
    }

    return 0U;
}

/* =========================================================
 * DIRECTION FAULT
 * ========================================================= */

uint8_t VCU_Faults_CheckDirection(void)
{
    if (VCU_Input_IsDirectionConflict() != 0U)
    {
        return 1U;
    }

    return 0U;
}

/* =========================================================
 * EMERGENCY FAULT
 * ========================================================= */

uint8_t VCU_Faults_CheckEmergency(void)
{
    if (VCU_Input_IsEmergencyActive() != 0U)
    {
        return 1U;
    }

    return 0U;
}

/* =========================================================
 * IGNITION FAULT
 * ========================================================= */

uint8_t VCU_Faults_CheckIgnition(void)
{
    /*
     * Ignition OFF is not itself a fault.
     * It is simply the normal OFF condition.
     */
    return 0U;
}

/* =========================================================
 * CLEAR FAULTS
 * ========================================================= */

void VCU_Faults_Clear(void)
{
    /*
     * Only clear the internally latched summary.
     * Active physical/communication fault conditions
     * will be detected again by VCU_Faults_Update().
     */

    VCU_Data.faults.active = 0U;
    VCU_Data.faults.code = VCU_FAULT_NONE;
    VCU_Data.faults.severity = VCU_FAULT_SEVERITY_NONE;
}

/* =========================================================
 * FAULT RECOVERY
 * ========================================================= */

uint8_t VCU_Faults_CanRecover(void)
{
    /*
     * Recovery is permitted only when:
     *   - emergency is inactive
     *   - inputs are plausible
     *   - BMS communication is valid
     *   - BMS reports no fault
     *   - MCU communication is valid
     *   - MCU reports no fault
     */

    if (VCU_Input_IsEmergencyActive() != 0U)
    {
        return 0U;
    }

    if (VCU_Input_IsPlausible() == 0U)
    {
        return 0U;
    }

    if (VCU_Data.bms.communication_valid == 0U)
    {
        return 0U;
    }

    if (VCU_Data.bms.fault_status != VCU_FAULT_NONE)
    {
        return 0U;
    }

    if (VCU_Data.mcu.communication_valid == 0U)
    {
        return 0U;
    }

    if (VCU_Data.mcu.fault_status != VCU_FAULT_NONE)
    {
        return 0U;
    }

    return 1U;
}

/* =========================================================
 * GET ACTIVE FAULT CODE
 * ========================================================= */

uint8_t VCU_Faults_GetActiveCode(void)
{
    return VCU_Data.faults.code;
}

/* =========================================================
 * GET FAULT SEVERITY
 * ========================================================= */

uint8_t VCU_Faults_GetSeverity(void)
{
    return VCU_Data.faults.severity;
}

/* =========================================================
 * CHECK WHETHER A FAULT IS ACTIVE
 * ========================================================= */

uint8_t VCU_Faults_IsActive(void)
{
    return VCU_Data.faults.active;
}
