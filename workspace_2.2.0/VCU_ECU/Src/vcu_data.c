#include "vcu_data.h"

/* =========================================================
 * GLOBAL VCU DATA
 * ========================================================= */

VCU_Data_t VCU_Data;

/* =========================================================
 * VCU DATA INITIALIZATION
 * ========================================================= */

void VCU_Data_Init(void)
{
    VCU_Data.system_state = VCU_STATE_INIT;

    VCU_Data.system_enabled = VCU_DISABLED;

    /* -------------------------
     * Analog inputs
     * ------------------------- */

    VCU_Data.input.analog.throttle_raw = 0U;
    VCU_Data.input.analog.brake_raw = 0U;

    VCU_Data.input.analog.aux1_raw = 0U;
    VCU_Data.input.analog.aux2_raw = 0U;

    VCU_Data.input.analog.throttle_percent = 0.0f;
    VCU_Data.input.analog.brake_percent = 0.0f;

    VCU_Data.input.analog.throttle_valid = VCU_INPUT_VALID;
    VCU_Data.input.analog.brake_valid = VCU_INPUT_VALID;

    /* -------------------------
     * Digital inputs
     * ------------------------- */

    VCU_Data.input.digital.ignition = VCU_INPUT_INACTIVE;
    VCU_Data.input.digital.emergency = VCU_INPUT_INACTIVE;
    VCU_Data.input.digital.forward = VCU_INPUT_INACTIVE;
    VCU_Data.input.digital.reverse = VCU_INPUT_INACTIVE;

    VCU_Data.input.direction = VCU_DIR_NEUTRAL;

    VCU_Data.input.accelerator_request = 0U;
    VCU_Data.input.brake_request = 0U;

    /* -------------------------
     * Driver request
     * ------------------------- */

    VCU_Data.driver_request.enable_request = VCU_DISABLED;
    VCU_Data.driver_request.motor_enable_request = VCU_MOTOR_DISABLE;
    VCU_Data.driver_request.contactor_enable_request = VCU_CONTACTOR_DISABLE;

    VCU_Data.driver_request.throttle_request_percent = 0U;
    VCU_Data.driver_request.brake_request_percent = 0U;

    VCU_Data.driver_request.direction = VCU_DIR_NEUTRAL;

    /* -------------------------
     * BMS data
     * ------------------------- */

    VCU_Data.bms.battery_voltage = 0.0f;
    VCU_Data.bms.battery_current = 0.0f;

    VCU_Data.bms.soc = 0U;
    VCU_Data.bms.temperature = 0;
    VCU_Data.bms.soh = 0U;

    VCU_Data.bms.fault_status = VCU_FAULT_NONE;

    VCU_Data.bms.communication_valid = 0U;
    VCU_Data.bms.enabled = VCU_DISABLED;

    VCU_Data.bms.last_rx_time_ms = 0U;

    /* -------------------------
     * MCU data
     * ------------------------- */

    VCU_Data.mcu.motor_speed = 0U;
    VCU_Data.mcu.motor_current = 0;

    VCU_Data.mcu.controller_temperature = 0;

    VCU_Data.mcu.enable_state = VCU_DISABLED;
    VCU_Data.mcu.fault_status = VCU_FAULT_NONE;

    VCU_Data.mcu.communication_valid = 0U;

    VCU_Data.mcu.last_rx_time_ms = 0U;

    /* -------------------------
     * Fault data
     * ------------------------- */

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

    /* -------------------------
     * Outputs
     * ------------------------- */

    VCU_Data.outputs.status_led = 0U;
    VCU_Data.outputs.fault_led = 0U;

    VCU_Data.outputs.motor_enable = VCU_MOTOR_DISABLE;
    VCU_Data.outputs.contactor_enable = VCU_CONTACTOR_DISABLE;

    /* -------------------------
     * Communication data
     * ------------------------- */

    VCU_Data.communication.bms_status_received = 0U;
    VCU_Data.communication.mcu_status_received = 0U;

    VCU_Data.communication.bms_command_pending = 0U;
    VCU_Data.communication.mcu_command_pending = 0U;

    VCU_Data.communication.last_bms_rx_time_ms = 0U;
    VCU_Data.communication.last_mcu_rx_time_ms = 0U;

    VCU_Data.communication.last_status_tx_time_ms = 0U;
    VCU_Data.communication.last_command_tx_time_ms = 0U;
}
