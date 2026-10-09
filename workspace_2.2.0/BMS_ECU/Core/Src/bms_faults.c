/*
 * bms_faults.c
 *
 * BMS protection and fault-management module.
 *
 * Protection monitored:
 *   - Over-voltage
 *   - Under-voltage
 *   - Over-current
 *   - Over-temperature
 *   - Under-temperature
 *   - Voltage sensor fault
 *   - Current sensor fault
 *   - Temperature sensor fault
 *
 * Features:
 *   - Fault detection
 *   - Fault hysteresis
 *   - Fault debounce
 *   - Fault latching
 *   - Safe fault recovery
 *
 * This module detects and manages protection faults.
 * The BMS state machine in bms_state.c owns system_state.
 */

#include "main.h"
#include "bms_faults.h"
#include "bms_data.h"
#include "bms_config.h"

/* -------------------------------------------------------------------------- */
/* Private variables                                                          */
/* -------------------------------------------------------------------------- */

/* Debounce start times */
static uint32_t over_voltage_start = 0U;
static uint32_t under_voltage_start = 0U;
static uint32_t over_current_start = 0U;
static uint32_t over_temperature_start = 0U;
static uint32_t under_temperature_start = 0U;

/* Debounce status */
static uint8_t over_voltage_pending = 0U;
static uint8_t under_voltage_pending = 0U;
static uint8_t over_current_pending = 0U;
static uint8_t over_temperature_pending = 0U;
static uint8_t under_temperature_pending = 0U;

/*
 * Fault-recovery timer.
 *
 * Once a reset has been requested, all safety conditions must remain
 * continuously safe for BMS_FAULT_RECOVERY_MS before the latch is cleared.
 */
static uint32_t recovery_start = 0U;
static uint8_t recovery_pending = 0U;

/*
 * Internal reset request.
 *
 * This is separate from BMS_Data.fault_reset_request so the protection
 * module can retain the reset request until recovery succeeds.
 */
static uint8_t fault_reset_requested = 0U;

/* -------------------------------------------------------------------------- */
/* Private helper functions                                                   */
/* -------------------------------------------------------------------------- */

/**
 * @brief Check whether a timer interval has elapsed.
 */
static uint8_t BMS_Fault_TimeElapsed(
    uint32_t start_time,
    uint32_t interval_ms)
{
    return ((uint32_t)(HAL_GetTick() - start_time) >= interval_ms);
}

/**
 * @brief Start debounce timer.
 */
static void BMS_Fault_StartDebounce(
    uint8_t *pending,
    uint32_t *start_time)
{
    if (*pending == 0U)
    {
        *pending = 1U;
        *start_time = HAL_GetTick();
    }
}

/**
 * @brief Cancel debounce timer.
 */
static void BMS_Fault_CancelDebounce(uint8_t *pending)
{
    *pending = 0U;
}

/**
 * @brief Debounce a protection condition.
 *
 * Returns 1 after the condition has remained active for the configured
 * debounce interval.
 */
static uint8_t BMS_Fault_DebounceCondition(
    uint8_t condition,
    uint8_t *pending,
    uint32_t *start_time)
{
    if (condition != 0U)
    {
        BMS_Fault_StartDebounce(
            pending,
            start_time);

        if (BMS_Fault_TimeElapsed(
                *start_time,
                BMS_FAULT_DEBOUNCE_MS))
        {
            return 1U;
        }
    }
    else
    {
        BMS_Fault_CancelDebounce(pending);
    }

    return 0U;
}

/**
 * @brief Check whether all monitored conditions are safe for recovery.
 */
static uint8_t BMS_Fault_AllConditionsSafe(void)
{
    /* ---------------------------------------------------------- */
    /* Measurement validity                                      */
    /* ---------------------------------------------------------- */

    if (BMS_Data.voltage_valid != BMS_MEASUREMENT_VALID)
    {
        return 0U;
    }

    if (BMS_Data.current_valid != BMS_MEASUREMENT_VALID)
    {
        return 0U;
    }

    if (BMS_Data.temperature_valid != BMS_MEASUREMENT_VALID)
    {
        return 0U;
    }

    /* ---------------------------------------------------------- */
    /* Sensor faults                                              */
    /* ---------------------------------------------------------- */

    if (BMS_Data.voltage_sensor_fault != 0U)
    {
        return 0U;
    }

    if (BMS_Data.current_sensor_fault != 0U)
    {
        return 0U;
    }

    if (BMS_Data.temperature_sensor_fault != 0U)
    {
        return 0U;
    }

    /* ---------------------------------------------------------- */
    /* Voltage recovery region                                   */
    /* ---------------------------------------------------------- */

    if (BMS_Data.battery_voltage > BMS_OVERVOLTAGE_RELEASE)
    {
        return 0U;
    }

    if (BMS_Data.battery_voltage < BMS_UNDERVOLTAGE_RELEASE)
    {
        return 0U;
    }

    /* ---------------------------------------------------------- */
    /* Current recovery region                                   */
    /* ---------------------------------------------------------- */

    if (BMS_Data.battery_current > BMS_OVERCURRENT_RELEASE)
    {
        return 0U;
    }

    if (BMS_Data.battery_current < -BMS_OVERCURRENT_RELEASE)
    {
        return 0U;
    }

    /* ---------------------------------------------------------- */
    /* Temperature recovery region                               */
    /* ---------------------------------------------------------- */

    if (BMS_Data.battery_temperature > BMS_OVERTEMPERATURE_RELEASE)
    {
        return 0U;
    }

    if (BMS_Data.battery_temperature < BMS_UNDERTEMPERATURE_RELEASE)
    {
        return 0U;
    }

    return 1U;
}

/**
 * @brief Process pending fault recovery.
 *
 * A reset request never clears a fault immediately.
 * The battery must remain continuously safe for the complete recovery
 * interval.
 */
static void BMS_Fault_ProcessRecovery(void)
{
    /* ---------------------------------------------------------- */
    /* No reset request                                           */
    /* ---------------------------------------------------------- */

    if (fault_reset_requested == 0U)
    {
        recovery_pending = 0U;
        return;
    }

    /* ---------------------------------------------------------- */
    /* Safety conditions must remain continuously valid          */
    /* ---------------------------------------------------------- */

    if (BMS_Fault_AllConditionsSafe() == 0U)
    {
        /*
         * Safety was lost.
         * Restart the recovery timer when conditions become safe again.
         */
        recovery_pending = 0U;

        return;
    }

    /* ---------------------------------------------------------- */
    /* Start recovery timer                                       */
    /* ---------------------------------------------------------- */

    if (recovery_pending == 0U)
    {
        recovery_pending = 1U;
        recovery_start = HAL_GetTick();

        return;
    }

    /* ---------------------------------------------------------- */
    /* Check recovery interval                                    */
    /* ---------------------------------------------------------- */

    if (BMS_Fault_TimeElapsed(
            recovery_start,
            BMS_FAULT_RECOVERY_MS))
    {
        /*
         * Safe conditions remained valid for the complete recovery period.
         */
        BMS_Data.fault_latched = 0U;

        fault_reset_requested = 0U;

        BMS_Data.fault_reset_request = 0U;

        recovery_pending = 0U;
    }
}

/* -------------------------------------------------------------------------- */
/* Public functions                                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize fault-management module.
 */
void BMS_Faults_Init(void)
{
    /* ---------------------------------------------------------- */
    /* Protection flags                                           */
    /* ---------------------------------------------------------- */

    BMS_Data.over_voltage = 0U;
    BMS_Data.under_voltage = 0U;
    BMS_Data.over_current = 0U;
    BMS_Data.over_temperature = 0U;
    BMS_Data.under_temperature = 0U;

    /* ---------------------------------------------------------- */
    /* Sensor faults                                              */
    /* ---------------------------------------------------------- */

    BMS_Data.voltage_sensor_fault = 0U;
    BMS_Data.current_sensor_fault = 0U;
    BMS_Data.temperature_sensor_fault = 0U;

    /* ---------------------------------------------------------- */
    /* Global fault information                                   */
    /* ---------------------------------------------------------- */

    BMS_Data.fault_status = 0U;
    BMS_Data.fault_latched = 0U;

    /*
     * State machine owns system_state.
     * We only make sure the system starts disabled.
     */
    BMS_Data.system_enabled = 0U;

    /* ---------------------------------------------------------- */
    /* Reset debounce state                                       */
    /* ---------------------------------------------------------- */

    over_voltage_start = 0U;
    under_voltage_start = 0U;
    over_current_start = 0U;
    over_temperature_start = 0U;
    under_temperature_start = 0U;

    over_voltage_pending = 0U;
    under_voltage_pending = 0U;
    over_current_pending = 0U;
    over_temperature_pending = 0U;
    under_temperature_pending = 0U;

    /* ---------------------------------------------------------- */
    /* Reset recovery state                                       */
    /* ---------------------------------------------------------- */

    recovery_start = 0U;
    recovery_pending = 0U;
    fault_reset_requested = 0U;

    /* ---------------------------------------------------------- */
    /* Diagnostic counter                                         */
    /* ---------------------------------------------------------- */

    BMS_Data.fault_update_count = 0U;
}

/**
 * @brief Evaluate all BMS protection conditions.
 */
void BMS_Faults_Update(void)
{
    uint8_t over_voltage_condition;
    uint8_t under_voltage_condition;
    uint8_t over_current_condition;
    uint8_t over_temperature_condition;
    uint8_t under_temperature_condition;

    /* ========================================================== */
    /* SENSOR FAULTS                                              */
    /* ========================================================== */

    /*
     * Rebuild active fault status from the current sensor-fault
     * conditions and protection conditions.
     */
    BMS_Data.fault_status = 0U;

    if (BMS_Data.voltage_sensor_fault != 0U)
    {
        BMS_Data.fault_status |=
            BMS_FAULT_VOLTAGE_SENSOR;
    }

    if (BMS_Data.current_sensor_fault != 0U)
    {
        BMS_Data.fault_status |=
            BMS_FAULT_CURRENT_SENSOR;
    }

    if (BMS_Data.temperature_sensor_fault != 0U)
    {
        BMS_Data.fault_status |=
            BMS_FAULT_TEMPERATURE_SENSOR;
    }

    /* ========================================================== */
    /* THRESHOLD CONDITIONS                                       */
    /* ========================================================== */

    /*
     * Over-voltage.
     */
    over_voltage_condition =
        (BMS_Data.battery_voltage > BMS_MAX_VOLTAGE)
        ? 1U
        : 0U;

    /*
     * Under-voltage.
     */
    under_voltage_condition =
        (BMS_Data.battery_voltage < BMS_MIN_VOLTAGE)
        ? 1U
        : 0U;

    /*
     * Over-current in either direction.
     *
     * Positive current and negative current are both treated as
     * over-current conditions.
     */
    over_current_condition =
        ((BMS_Data.battery_current > BMS_MAX_CURRENT) ||
         (BMS_Data.battery_current < -BMS_MAX_CURRENT))
        ? 1U
        : 0U;

    /*
     * Over-temperature.
     */
    over_temperature_condition =
        (BMS_Data.battery_temperature > BMS_MAX_TEMPERATURE)
        ? 1U
        : 0U;

    /*
     * Under-temperature.
     */
    under_temperature_condition =
        (BMS_Data.battery_temperature < BMS_MIN_TEMPERATURE)
        ? 1U
        : 0U;

    /* ========================================================== */
    /* DEBOUNCED PROTECTION                                       */
    /* ========================================================== */

    /*
     * Over-voltage.
     */
    if (BMS_Fault_DebounceCondition(
            over_voltage_condition,
            &over_voltage_pending,
            &over_voltage_start))
    {
        BMS_Data.over_voltage = 1U;
    }
    else if (BMS_Data.battery_voltage <=
             BMS_OVERVOLTAGE_RELEASE)
    {
        BMS_Data.over_voltage = 0U;
    }

    /*
     * Under-voltage.
     */
    if (BMS_Fault_DebounceCondition(
            under_voltage_condition,
            &under_voltage_pending,
            &under_voltage_start))
    {
        BMS_Data.under_voltage = 1U;
    }
    else if (BMS_Data.battery_voltage >=
             BMS_UNDERVOLTAGE_RELEASE)
    {
        BMS_Data.under_voltage = 0U;
    }

    /*
     * Over-current.
     */
    if (BMS_Fault_DebounceCondition(
            over_current_condition,
            &over_current_pending,
            &over_current_start))
    {
        BMS_Data.over_current = 1U;
    }
    else if ((BMS_Data.battery_current <=
              BMS_OVERCURRENT_RELEASE) &&
             (BMS_Data.battery_current >=
              -BMS_OVERCURRENT_RELEASE))
    {
        BMS_Data.over_current = 0U;
    }

    /*
     * Over-temperature.
     */
    if (BMS_Fault_DebounceCondition(
            over_temperature_condition,
            &over_temperature_pending,
            &over_temperature_start))
    {
        BMS_Data.over_temperature = 1U;
    }
    else if (BMS_Data.battery_temperature <=
             BMS_OVERTEMPERATURE_RELEASE)
    {
        BMS_Data.over_temperature = 0U;
    }

    /*
     * Under-temperature.
     */
    if (BMS_Fault_DebounceCondition(
            under_temperature_condition,
            &under_temperature_pending,
            &under_temperature_start))
    {
        BMS_Data.under_temperature = 1U;
    }
    else if (BMS_Data.battery_temperature >=
             BMS_UNDERTEMPERATURE_RELEASE)
    {
        BMS_Data.under_temperature = 0U;
    }

    /* ========================================================== */
    /* BUILD FINAL FAULT BITMASK                                  */
    /* ========================================================== */

    if (BMS_Data.over_voltage != 0U)
    {
        BMS_Data.fault_status |=
            BMS_FAULT_OVERVOLTAGE;
    }

    if (BMS_Data.under_voltage != 0U)
    {
        BMS_Data.fault_status |=
            BMS_FAULT_UNDERVOLTAGE;
    }

    if (BMS_Data.over_current != 0U)
    {
        BMS_Data.fault_status |=
            BMS_FAULT_OVERCURRENT;
    }

    if (BMS_Data.over_temperature != 0U)
    {
        BMS_Data.fault_status |=
            BMS_FAULT_OVERTEMPERATURE;
    }

    if (BMS_Data.under_temperature != 0U)
    {
        BMS_Data.fault_status |=
            BMS_FAULT_UNDERTEMPERATURE;
    }

    /* ========================================================== */
    /* FAULT LATCH                                                */
    /* ========================================================== */

    if (BMS_Data.fault_status != 0U)
    {
        /*
         * Any active protection or sensor fault latches the
         * BMS into a fault condition.
         */
        BMS_Data.fault_latched = 1U;

        /*
         * A newly occurring fault cancels any recovery timer.
         */
        recovery_pending = 0U;
    }

    /* ========================================================== */
    /* FAULT RECOVERY                                             */
    /* ========================================================== */

    BMS_Fault_ProcessRecovery();

    /*
     * IMPORTANT:
     *
     * Do NOT modify BMS_Data.system_state here.
     *
     * bms_state.c is the single owner of the BMS operating state.
     */

    /* ========================================================== */
    /* DIAGNOSTIC COUNTER                                         */
    /* ========================================================== */

    BMS_Data.fault_update_count++;
}

/**
 * @brief Request clearing of a latched fault.
 *
 * The fault is not cleared immediately.
 * Conditions must remain continuously safe for the configured
 * recovery interval.
 */
void BMS_Faults_RequestReset(void)
{
    /*
     * Store the request internally.
     */
    fault_reset_requested = 1U;

    BMS_Data.fault_reset_request = 1U;

    /*
     * Try to begin recovery immediately.
     * BMS_Faults_Update() will continue the process.
     */
    BMS_Fault_ProcessRecovery();
}

/**
 * @brief Return whether a fault is currently active.
 */
uint8_t BMS_Faults_IsActive(void)
{
    if (BMS_Data.fault_status != 0U)
    {
        return 1U;
    }

    return 0U;
}

/**
 * @brief Return whether a fault is latched.
 */
uint8_t BMS_Faults_IsLatched(void)
{
    return BMS_Data.fault_latched;
}
