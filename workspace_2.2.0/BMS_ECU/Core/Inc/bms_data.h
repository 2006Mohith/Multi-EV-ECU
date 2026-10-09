#ifndef BMS_DATA_H
#define BMS_DATA_H

#include <stdint.h>

/*
 * ============================================================
 * BMS SYSTEM STATES
 * ============================================================
 */
typedef enum
{
    BMS_SYSTEM_INIT = 0,
    BMS_SYSTEM_NORMAL,
    BMS_SYSTEM_WARNING,
    BMS_SYSTEM_FAULT,
    BMS_SYSTEM_SHUTDOWN
} BMS_SystemState_t;


/*
 * ============================================================
 * BMS MEASUREMENT VALIDITY
 * ============================================================
 */
typedef enum
{
    BMS_MEASUREMENT_INVALID = 0,
    BMS_MEASUREMENT_VALID
} BMS_MeasurementValidity_t;


/*
 * ============================================================
 * BMS DATA STRUCTURE
 * ============================================================
 */
typedef struct
{
    /* --------------------------------------------------------
     * PACK MEASUREMENTS
     * -------------------------------------------------------- */

    float battery_voltage;
    float battery_current;
    float battery_temperature;


    /* --------------------------------------------------------
     * BATTERY ESTIMATION
     * -------------------------------------------------------- */

    float state_of_charge;
    float state_of_health;


    /* --------------------------------------------------------
     * MINIMUM / MAXIMUM OBSERVATIONS
     *
     * These are reserved for future cell-level monitoring.
     * At present, the system measures pack voltage and one
     * temperature channel.
     * -------------------------------------------------------- */

    float voltage_min;
    float voltage_max;

    float temperature_min;
    float temperature_max;


    /* --------------------------------------------------------
     * PROTECTION FLAGS
     * -------------------------------------------------------- */

    uint8_t over_voltage;
    uint8_t under_voltage;
    uint8_t over_current;
    uint8_t over_temperature;
    uint8_t under_temperature;


    /* --------------------------------------------------------
     * SENSOR / MEASUREMENT FAULTS
     * -------------------------------------------------------- */

    uint8_t voltage_sensor_fault;
    uint8_t current_sensor_fault;
    uint8_t temperature_sensor_fault;


    /* --------------------------------------------------------
     * GLOBAL FAULT INFORMATION
     * -------------------------------------------------------- */

    uint8_t fault_status;
    uint8_t fault_latched;


    /* --------------------------------------------------------
     * BMS OPERATING STATE
     * -------------------------------------------------------- */

    BMS_SystemState_t system_state;


    /* --------------------------------------------------------
     * MEASUREMENT VALIDITY
     * -------------------------------------------------------- */

    BMS_MeasurementValidity_t voltage_valid;
    BMS_MeasurementValidity_t current_valid;
    BMS_MeasurementValidity_t temperature_valid;


    /* --------------------------------------------------------
     * CONTROL STATUS
     * -------------------------------------------------------- */

    uint8_t system_enabled;


    /* --------------------------------------------------------
     * CAN STATUS
     * -------------------------------------------------------- */

    uint8_t can_rx_active;
    uint8_t can_tx_active;


    /* --------------------------------------------------------
     * COMMUNICATION / COMMAND STATUS
     * -------------------------------------------------------- */

    uint8_t status_request;
    uint8_t fault_reset_request;


    /* --------------------------------------------------------
     * DIAGNOSTIC INFORMATION
     * -------------------------------------------------------- */

    uint32_t adc_update_count;
    uint32_t fault_update_count;
    uint32_t can_rx_count;
    uint32_t can_tx_count;

} BMS_Data_t;


/*
 * ============================================================
 * GLOBAL BMS DATA INSTANCE
 * ============================================================
 */

extern BMS_Data_t BMS_Data;


#endif /* BMS_DATA_H */
