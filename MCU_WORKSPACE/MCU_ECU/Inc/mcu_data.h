#ifndef MCU_DATA_H
#define MCU_DATA_H

#include <stdint.h>
#include "mcu_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * ADC / potentiometer data
 * ========================================================================== */

typedef struct
{
    uint16_t pot_raw;
    float    pot_voltage;
    uint8_t  pot_percent;
    uint8_t  valid;
} MCU_ADC_Data_t;

/* ============================================================================
 * Hall sensor data
 * ========================================================================== */

typedef struct
{
    uint32_t last_capture;
    uint32_t previous_capture;
    uint32_t pulse_period_us;

    uint32_t rpm;

    uint8_t  pulse_received;
    uint8_t  signal_valid;
    uint8_t  timeout;

    uint32_t pulse_count;
} MCU_Hall_Data_t;

/* ============================================================================
 * Motor command data
 * ========================================================================== */

typedef struct
{
    uint8_t command;
    uint8_t direction;
    uint8_t pwm_percent;
    uint8_t enable;
    uint8_t requested_state;

    uint8_t command_counter;

    uint8_t valid;
} MCU_Motor_Command_t;

/* ============================================================================
 * Motor operating data
 * ========================================================================== */

typedef struct
{
    uint8_t  state;
    uint8_t  direction;
    uint8_t  enabled;

    uint8_t  pwm_percent;
    uint16_t pwm_compare;

    uint32_t rpm;

    uint8_t  driver_enabled;

    uint8_t  braking;

    uint8_t  direction_change_pending;
} MCU_Motor_Data_t;

/* ============================================================================
 * VCU communication data
 * ========================================================================== */

typedef struct
{
    uint8_t  communication_valid;
    uint8_t  command_received;

    uint32_t last_command_time;
    uint32_t last_status_time;

    uint32_t timeout_count;

    uint8_t  last_command_counter;
    uint8_t  heartbeat_valid;
} MCU_VCU_Comm_Data_t;

/* ============================================================================
 * CAN heartbeat data
 * ========================================================================== */

typedef struct
{
    uint8_t tx_counter;
    uint8_t rx_counter;

    uint8_t vcu_heartbeat_received;
    uint8_t vcu_heartbeat_counter;

    uint32_t last_heartbeat_time;

    uint8_t valid;
} MCU_CAN_Heartbeat_Data_t;

/* ============================================================================
 * Local button/input data
 * ========================================================================== */

typedef struct
{
    uint8_t start_pressed;
    uint8_t stop_pressed;
    uint8_t forward_pressed;
    uint8_t reverse_pressed;

    uint8_t direction_conflict;

    uint8_t inputs_valid;

    uint32_t last_start_time;
    uint32_t last_stop_time;
    uint32_t last_forward_time;
    uint32_t last_reverse_time;
} MCU_Input_Data_t;

/* ============================================================================
 * Fault data
 * ========================================================================== */

typedef struct
{
    uint8_t active;
    uint8_t latched;

    uint8_t code;
    uint8_t severity;

    uint8_t previous_code;

    uint8_t hall_fault;
    uint8_t can_fault;
    uint8_t vcu_comm_fault;
    uint8_t command_fault;
    uint8_t direction_fault;
    uint8_t motor_driver_fault;
    uint8_t potentiometer_fault;
    uint8_t button_fault;

    uint8_t reset_requested;

    uint8_t confirmation_count;
    uint8_t recovery_count;
} MCU_Fault_Data_t;

/* ============================================================================
 * Output data
 * ========================================================================== */

typedef struct
{
    uint8_t ain1;
    uint8_t ain2;
    uint8_t standby;

    uint8_t pwm_percent;

    uint8_t status_led;
    uint8_t fault_led;
    uint8_t buzzer;

    uint8_t motor_output_enabled;
} MCU_Output_Data_t;

/* ============================================================================
 * LCD data
 * ========================================================================== */

typedef struct
{
    uint8_t page;
    uint8_t update_required;

    uint32_t last_update_time;
} MCU_LCD_Data_t;

/* ============================================================================
 * Software task supervision
 * ========================================================================== */

typedef struct
{
    uint8_t task_flags;

    uint32_t last_supervision_time;

    uint8_t adc_ok;
    uint8_t can_ok;
    uint8_t hall_ok;
    uint8_t motor_ok;
    uint8_t input_ok;
    uint8_t lcd_ok;

    uint8_t supervision_fault;
} MCU_Task_Data_t;

/* ============================================================================
 * Global MCU data container
 * ========================================================================== */

typedef struct
{
    MCU_ADC_Data_t             adc;
    MCU_Hall_Data_t            hall;

    MCU_Motor_Command_t        motor_command;
    MCU_Motor_Data_t            motor;

    MCU_VCU_Comm_Data_t        vcu_comm;
    MCU_CAN_Heartbeat_Data_t   can_heartbeat;

    MCU_Input_Data_t           input;
    MCU_Fault_Data_t            fault;
    MCU_Output_Data_t           output;

    MCU_LCD_Data_t              lcd;
    MCU_Task_Data_t             task;

    uint8_t                     system_initialized;
    uint8_t                     system_healthy;
} MCU_Data_t;

/* ============================================================================
 * Global data object
 * ========================================================================== */

extern MCU_Data_t MCU_Data;

/* ============================================================================
 * Initialization
 * ========================================================================== */

void MCU_Data_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* MCU_DATA_H */
