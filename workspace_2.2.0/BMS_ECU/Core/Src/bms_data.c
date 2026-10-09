/*
 * bms_data.c
 *
 * Central BMS data storage and initialization.
 */

#include "bms_data.h"
#include "bms_config.h"


/*
 * ============================================================
 * GLOBAL BMS DATA
 * ============================================================
 *
 * All BMS measurements, estimates, protection flags,
 * communication states and diagnostics are stored here.
 */

BMS_Data_t BMS_Data =
{
    /* --------------------------------------------------------
     * PACK MEASUREMENTS
     * -------------------------------------------------------- */

    .battery_voltage = 0.0f,

    .battery_current = 0.0f,

    .battery_temperature =
        BMS_DEFAULT_TEMPERATURE,


    /* --------------------------------------------------------
     * BATTERY ESTIMATION
     * -------------------------------------------------------- */

    .state_of_charge = 0.0f,

    .state_of_health =
        BMS_SOH_INITIAL,


    /* --------------------------------------------------------
     * OBSERVED LIMITS
     * -------------------------------------------------------- */

    .voltage_min = 0.0f,

    .voltage_max = 0.0f,

    .temperature_min =
        BMS_DEFAULT_TEMPERATURE,

    .temperature_max =
        BMS_DEFAULT_TEMPERATURE,


    /* --------------------------------------------------------
     * PROTECTION FLAGS
     * -------------------------------------------------------- */

    .over_voltage = 0U,

    .under_voltage = 0U,

    .over_current = 0U,

    .over_temperature = 0U,

    .under_temperature = 0U,


    /* --------------------------------------------------------
     * SENSOR FAULT FLAGS
     * -------------------------------------------------------- */

    .voltage_sensor_fault = 0U,

    .current_sensor_fault = 0U,

    .temperature_sensor_fault = 0U,


    /* --------------------------------------------------------
     * GLOBAL FAULT INFORMATION
     * -------------------------------------------------------- */

    .fault_status = 0U,

    .fault_latched = 0U,


    /* --------------------------------------------------------
     * BMS OPERATING STATE
     * -------------------------------------------------------- */

    .system_state =
        BMS_SYSTEM_INIT,


    /* --------------------------------------------------------
     * MEASUREMENT VALIDITY
     * -------------------------------------------------------- */

    .voltage_valid =
        BMS_MEASUREMENT_INVALID,

    .current_valid =
        BMS_MEASUREMENT_INVALID,

    .temperature_valid =
        BMS_MEASUREMENT_INVALID,


    /* --------------------------------------------------------
     * CONTROL STATUS
     * -------------------------------------------------------- */

    .system_enabled = 0U,


    /* --------------------------------------------------------
     * CAN STATUS
     * -------------------------------------------------------- */

    .can_rx_active = 0U,

    .can_tx_active = 0U,


    /* --------------------------------------------------------
     * CAN COMMAND STATUS
     * -------------------------------------------------------- */

    .status_request = 0U,

    .fault_reset_request = 0U,


    /* --------------------------------------------------------
     * DIAGNOSTIC COUNTERS
     * -------------------------------------------------------- */

    .adc_update_count = 0U,

    .fault_update_count = 0U,

    .can_rx_count = 0U,

    .can_tx_count = 0U
};
