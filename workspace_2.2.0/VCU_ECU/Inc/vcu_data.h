#ifndef VCU_DATA_H
#define VCU_DATA_H

#include <stdint.h>

#include "vcu_config.h"

/* =========================================================
 * VCU SYSTEM STATE
 * ========================================================= */

typedef enum
{
    VCU_STATE_INIT = VCU_SYSTEM_INIT,
    VCU_STATE_OFF = VCU_SYSTEM_OFF,
    VCU_STATE_READY = VCU_SYSTEM_READY,
    VCU_STATE_DRIVE = VCU_SYSTEM_DRIVE,
    VCU_STATE_BRAKE = VCU_SYSTEM_BRAKE,
    VCU_STATE_WARNING = VCU_SYSTEM_WARNING,
    VCU_STATE_FAULT = VCU_SYSTEM_FAULT,
    VCU_STATE_SHUTDOWN = VCU_SYSTEM_SHUTDOWN

} VCU_SystemState_t;

/* =========================================================
 * DIRECTION
 * ========================================================= */

typedef enum
{
    VCU_DIR_NEUTRAL = VCU_DIRECTION_NEUTRAL,
    VCU_DIR_FORWARD = VCU_DIRECTION_FORWARD,
    VCU_DIR_REVERSE = VCU_DIRECTION_REVERSE

} VCU_Direction_t;

/* =========================================================
 * INPUT VALIDITY
 * ========================================================= */

typedef enum
{
    VCU_INPUT_VALID = 0U,
    VCU_INPUT_INVALID = 1U

} VCU_InputValidity_t;

/* =========================================================
 * DIGITAL INPUTS
 * ========================================================= */

typedef struct
{
    uint8_t ignition;
    uint8_t emergency;
    uint8_t forward;
    uint8_t reverse;

} VCU_DigitalInputs_t;

/* =========================================================
 * ANALOG INPUTS
 * ========================================================= */

typedef struct
{
    uint16_t throttle_raw;
    uint16_t brake_raw;

    uint16_t aux1_raw;
    uint16_t aux2_raw;

    float throttle_percent;
    float brake_percent;

    VCU_InputValidity_t throttle_valid;
    VCU_InputValidity_t brake_valid;

} VCU_AnalogInputs_t;

/* =========================================================
 * DRIVER INPUT DATA
 * ========================================================= */

typedef struct
{
    VCU_AnalogInputs_t analog;
    VCU_DigitalInputs_t digital;

    VCU_Direction_t direction;

    uint8_t accelerator_request;
    uint8_t brake_request;

} VCU_InputData_t;

/* =========================================================
 * DRIVER REQUEST
 * ========================================================= */

typedef struct
{
    uint8_t enable_request;
    uint8_t motor_enable_request;
    uint8_t contactor_enable_request;

    uint8_t throttle_request_percent;
    uint8_t brake_request_percent;

    VCU_Direction_t direction;

} VCU_DriverRequest_t;

/* =========================================================
 * BMS DATA
 * ========================================================= */

typedef struct
{
    float battery_voltage;
    float battery_current;

    uint8_t soc;
    int8_t temperature;
    uint8_t soh;

    uint8_t fault_status;

    uint8_t communication_valid;
    uint8_t enabled;

    uint32_t last_rx_time_ms;

} VCU_BMS_Data_t;

/* =========================================================
 * MCU DATA
 * ========================================================= */

typedef struct
{
    uint16_t motor_speed;
    int16_t motor_current;

    int8_t controller_temperature;

    uint8_t enable_state;
    uint8_t fault_status;

    uint8_t communication_valid;

    uint32_t last_rx_time_ms;

} VCU_MCU_Data_t;

/* =========================================================
 * FAULT DATA
 * ========================================================= */

typedef struct
{
    uint8_t active;
    uint8_t code;
    uint8_t severity;

    uint8_t bms_communication_fault;
    uint8_t bms_critical_fault;

    uint8_t mcu_communication_fault;
    uint8_t mcu_critical_fault;

    uint8_t throttle_fault;
    uint8_t brake_fault;

    uint8_t direction_fault;
    uint8_t emergency_fault;
    uint8_t ignition_fault;

} VCU_Fault_Data_t;

/* =========================================================
 * OUTPUT DATA
 * ========================================================= */

typedef struct
{
    uint8_t status_led;
    uint8_t fault_led;

    uint8_t motor_enable;
    uint8_t contactor_enable;

} VCU_OutputData_t;

/* =========================================================
 * COMMUNICATION DATA
 * ========================================================= */

typedef struct
{
    uint8_t bms_status_received;
    uint8_t mcu_status_received;

    uint8_t bms_command_pending;
    uint8_t mcu_command_pending;

    uint32_t last_bms_rx_time_ms;
    uint32_t last_mcu_rx_time_ms;

    uint32_t last_status_tx_time_ms;
    uint32_t last_command_tx_time_ms;

} VCU_CommunicationData_t;

/* =========================================================
 * COMPLETE VCU DATA
 * ========================================================= */

typedef struct
{
    VCU_SystemState_t system_state;

    uint8_t system_enabled;

    VCU_InputData_t input;
    VCU_DriverRequest_t driver_request;

    VCU_BMS_Data_t bms;
    VCU_MCU_Data_t mcu;

    VCU_Fault_Data_t faults;

    VCU_OutputData_t outputs;
    VCU_CommunicationData_t communication;

} VCU_Data_t;

/* =========================================================
 * GLOBAL VCU DATA
 * ========================================================= */

extern VCU_Data_t VCU_Data;

/* =========================================================
 * DATA INITIALIZATION
 * ========================================================= */

void VCU_Data_Init(void);

#endif /* VCU_DATA_H */
