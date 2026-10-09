#include "mcu_data.h"

/* ============================================================================
 * Global MCU data object
 * ========================================================================== */

MCU_Data_t MCU_Data;

/* ============================================================================
 * MCU data initialization
 * ========================================================================== */

void MCU_Data_Init(void)
{
    /* Clear the complete data structure */
    MCU_Data.adc.pot_raw = 0U;
    MCU_Data.adc.pot_voltage = 0.0f;
    MCU_Data.adc.pot_percent = 0U;
    MCU_Data.adc.valid = MCU_FALSE;

    /* ------------------------------------------------------------------------
     * Hall sensor
     * ---------------------------------------------------------------------- */

    MCU_Data.hall.last_capture = 0U;
    MCU_Data.hall.previous_capture = 0U;
    MCU_Data.hall.pulse_period_us = 0U;
    MCU_Data.hall.rpm = 0U;
    MCU_Data.hall.pulse_received = MCU_FALSE;
    MCU_Data.hall.signal_valid = MCU_FALSE;
    MCU_Data.hall.timeout = MCU_TRUE;
    MCU_Data.hall.pulse_count = 0U;

    /* ------------------------------------------------------------------------
     * Motor command
     * ---------------------------------------------------------------------- */

    MCU_Data.motor_command.command = MCU_COMMAND_NONE;
    MCU_Data.motor_command.direction = MCU_DIRECTION_NEUTRAL;
    MCU_Data.motor_command.pwm_percent = 0U;
    MCU_Data.motor_command.enable = MCU_DISABLED;
    MCU_Data.motor_command.requested_state = MCU_STATE_DISABLED;
    MCU_Data.motor_command.command_counter = 0U;
    MCU_Data.motor_command.valid = MCU_FALSE;

    /* ------------------------------------------------------------------------
     * Motor state
     * ---------------------------------------------------------------------- */

    MCU_Data.motor.state = MCU_STATE_INIT;
    MCU_Data.motor.direction = MCU_DIRECTION_NEUTRAL;
    MCU_Data.motor.enabled = MCU_DISABLED;
    MCU_Data.motor.pwm_percent = 0U;
    MCU_Data.motor.pwm_compare = 0U;
    MCU_Data.motor.rpm = 0U;
    MCU_Data.motor.driver_enabled = MCU_DISABLED;
    MCU_Data.motor.braking = MCU_FALSE;
    MCU_Data.motor.direction_change_pending = MCU_FALSE;

    /* ------------------------------------------------------------------------
     * VCU communication
     * ---------------------------------------------------------------------- */

    MCU_Data.vcu_comm.communication_valid = MCU_FALSE;
    MCU_Data.vcu_comm.command_received = MCU_FALSE;
    MCU_Data.vcu_comm.last_command_time = 0U;
    MCU_Data.vcu_comm.last_status_time = 0U;
    MCU_Data.vcu_comm.timeout_count = 0U;
    MCU_Data.vcu_comm.last_command_counter = 0U;
    MCU_Data.vcu_comm.heartbeat_valid = MCU_FALSE;

    /* ------------------------------------------------------------------------
     * CAN heartbeat
     * ---------------------------------------------------------------------- */

    MCU_Data.can_heartbeat.tx_counter = 0U;
    MCU_Data.can_heartbeat.rx_counter = 0U;
    MCU_Data.can_heartbeat.vcu_heartbeat_received = MCU_FALSE;
    MCU_Data.can_heartbeat.vcu_heartbeat_counter = 0U;
    MCU_Data.can_heartbeat.last_heartbeat_time = 0U;
    MCU_Data.can_heartbeat.valid = MCU_FALSE;

    /* ------------------------------------------------------------------------
     * Local inputs
     * ---------------------------------------------------------------------- */

    MCU_Data.input.start_pressed = MCU_FALSE;
    MCU_Data.input.stop_pressed = MCU_FALSE;
    MCU_Data.input.forward_pressed = MCU_FALSE;
    MCU_Data.input.reverse_pressed = MCU_FALSE;
    MCU_Data.input.direction_conflict = MCU_FALSE;
    MCU_Data.input.inputs_valid = MCU_TRUE;

    MCU_Data.input.last_start_time = 0U;
    MCU_Data.input.last_stop_time = 0U;
    MCU_Data.input.last_forward_time = 0U;
    MCU_Data.input.last_reverse_time = 0U;

    /* ------------------------------------------------------------------------
     * Fault data
     * ---------------------------------------------------------------------- */

    MCU_Data.fault.active = MCU_FALSE;
    MCU_Data.fault.latched = MCU_FALSE;
    MCU_Data.fault.code = MCU_FAULT_NONE;
    MCU_Data.fault.severity = MCU_FAULT_SEVERITY_NONE;
    MCU_Data.fault.previous_code = MCU_FAULT_NONE;

    MCU_Data.fault.hall_fault = MCU_FALSE;
    MCU_Data.fault.can_fault = MCU_FALSE;
    MCU_Data.fault.vcu_comm_fault = MCU_FALSE;
    MCU_Data.fault.command_fault = MCU_FALSE;
    MCU_Data.fault.direction_fault = MCU_FALSE;
    MCU_Data.fault.motor_driver_fault = MCU_FALSE;
    MCU_Data.fault.potentiometer_fault = MCU_FALSE;
    MCU_Data.fault.button_fault = MCU_FALSE;

    MCU_Data.fault.reset_requested = MCU_FALSE;
    MCU_Data.fault.confirmation_count = 0U;
    MCU_Data.fault.recovery_count = 0U;

    /* ------------------------------------------------------------------------
     * Output data
     * ---------------------------------------------------------------------- */

    MCU_Data.output.ain1 = MCU_MOTOR_STARTUP_AIN1;
    MCU_Data.output.ain2 = MCU_MOTOR_STARTUP_AIN2;
    MCU_Data.output.standby = MCU_MOTOR_STARTUP_STBY;

    MCU_Data.output.pwm_percent = MCU_MOTOR_STARTUP_PWM;

    MCU_Data.output.status_led = MCU_STATUS_LED_OFF;
    MCU_Data.output.fault_led = MCU_FAULT_LED_OFF;
    MCU_Data.output.buzzer = MCU_BUZZER_OFF;

    MCU_Data.output.motor_output_enabled = MCU_DISABLED;

    /* ------------------------------------------------------------------------
     * LCD data
     * ---------------------------------------------------------------------- */

    MCU_Data.lcd.page = 0U;
    MCU_Data.lcd.update_required = MCU_TRUE;
    MCU_Data.lcd.last_update_time = 0U;

    /* ------------------------------------------------------------------------
     * Software task supervision
     * ---------------------------------------------------------------------- */

    MCU_Data.task.task_flags = 0U;
    MCU_Data.task.last_supervision_time = 0U;

    MCU_Data.task.adc_ok = MCU_FALSE;
    MCU_Data.task.can_ok = MCU_FALSE;
    MCU_Data.task.hall_ok = MCU_FALSE;
    MCU_Data.task.motor_ok = MCU_FALSE;
    MCU_Data.task.input_ok = MCU_FALSE;
    MCU_Data.task.lcd_ok = MCU_FALSE;

    MCU_Data.task.supervision_fault = MCU_FALSE;

    /* ------------------------------------------------------------------------
     * System state
     * ---------------------------------------------------------------------- */

    MCU_Data.system_initialized = MCU_TRUE;
    MCU_Data.system_healthy = MCU_FALSE;
}
