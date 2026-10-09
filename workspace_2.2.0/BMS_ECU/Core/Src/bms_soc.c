/*
 * bms_soc.c
 *
 * BMS State of Charge estimation.
 *
 * Current implementation:
 *   Filtered pack-voltage based SOC.
 *
 * Features:
 *   - Voltage validity checking
 *   - Linear voltage-to-SOC mapping
 *   - Low-pass filtering
 *   - SOC clamping
 *   - Startup initialization
 *
 * Future enhancement:
 *   - Coulomb counting
 *   - Battery capacity calibration
 *   - Rest-voltage correction
 *   - Temperature compensation
 *   - Battery chemistry specific lookup table
 */

#include "bms_soc.h"
#include "bms_data.h"
#include "bms_config.h"

#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* Private configuration                                                       */
/* -------------------------------------------------------------------------- */

/*
 * SOC filter coefficient.
 *
 * Smaller value:
 *   More filtering / slower response.
 *
 * Larger value:
 *   Faster response / less filtering.
 */
#define BMS_SOC_FILTER_ALPHA          0.20f

/*
 * Small voltage regions around the configured endpoints.
 */
#define BMS_SOC_FULL_HYSTERESIS       0.20f
#define BMS_SOC_EMPTY_HYSTERESIS      0.20f

/* -------------------------------------------------------------------------- */
/* Private variables                                                          */
/* -------------------------------------------------------------------------- */

/* Filtered SOC value */
static float soc_filtered = 0.0f;

/* Indicates that a first valid SOC estimate has been received */
static uint8_t soc_initialized = 0U;

/* -------------------------------------------------------------------------- */
/* Private functions                                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief Clamp a floating-point value to a specified range.
 */
static float BMS_SOC_Clamp(
    float value,
    float minimum,
    float maximum)
{
    if (value < minimum)
    {
        return minimum;
    }

    if (value > maximum)
    {
        return maximum;
    }

    return value;
}

/**
 * @brief Calculate instantaneous SOC from battery-pack voltage.
 *
 * This is intentionally a simple linear estimator.
 *
 * SOC = 0% at BMS_MIN_VOLTAGE
 * SOC = 100% at BMS_MAX_VOLTAGE
 */
static float BMS_SOC_CalculateInstantaneous(float voltage)
{
    float voltage_range;
    float soc;

    /*
     * Clearly below empty region.
     */
    if (voltage <=
        (BMS_MIN_VOLTAGE - BMS_SOC_EMPTY_HYSTERESIS))
    {
        return BMS_SOC_MIN;
    }

    /*
     * Clearly above full region.
     */
    if (voltage >=
        (BMS_MAX_VOLTAGE + BMS_SOC_FULL_HYSTERESIS))
    {
        return BMS_SOC_MAX;
    }

    /*
     * At or below configured minimum.
     */
    if (voltage <= BMS_MIN_VOLTAGE)
    {
        return BMS_SOC_MIN;
    }

    /*
     * At or above configured maximum.
     */
    if (voltage >= BMS_MAX_VOLTAGE)
    {
        return BMS_SOC_MAX;
    }

    /*
     * Protect against an invalid configuration in which
     * minimum and maximum voltages are equal.
     */
    voltage_range =
        BMS_MAX_VOLTAGE - BMS_MIN_VOLTAGE;

    if (voltage_range <= 0.0f)
    {
        return BMS_SOC_MIN;
    }

    /*
     * Linear interpolation.
     */
    soc =
        ((voltage - BMS_MIN_VOLTAGE) /
         voltage_range) * 100.0f;

    /*
     * Final range protection.
     */
    return BMS_SOC_Clamp(
        soc,
        BMS_SOC_MIN,
        BMS_SOC_MAX);
}

/**
 * @brief Apply low-pass filtering to SOC.
 */
static float BMS_SOC_Filter(
    float previous,
    float new_value)
{
    return
        (previous *
         (1.0f - BMS_SOC_FILTER_ALPHA))
        +
        (new_value *
         BMS_SOC_FILTER_ALPHA);
}

/* -------------------------------------------------------------------------- */
/* Public functions                                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize SOC module.
 */
void BMS_SOC_Init(void)
{
    soc_filtered = BMS_SOC_MIN;

    soc_initialized = 0U;

    BMS_Data.state_of_charge =
        BMS_SOC_MIN;
}

/**
 * @brief Update SOC estimate.
 */
void BMS_SOC_Update(void)
{
    float instantaneous_soc;

    /*
     * --------------------------------------------------------------
     * Validate voltage measurement
     * --------------------------------------------------------------
     *
     * Never update SOC from an invalid voltage measurement.
     */
    if (BMS_Data.voltage_valid !=
        BMS_MEASUREMENT_VALID)
    {
        return;
    }

    /*
     * --------------------------------------------------------------
     * Calculate instantaneous voltage-based SOC
     * --------------------------------------------------------------
     */
    instantaneous_soc =
        BMS_SOC_CalculateInstantaneous(
            BMS_Data.battery_voltage);

    /*
     * --------------------------------------------------------------
     * First valid SOC acquisition
     * --------------------------------------------------------------
     *
     * Use the first valid estimate immediately rather than
     * slowly ramping up from 0%.
     */
    if (soc_initialized == 0U)
    {
        soc_filtered =
            instantaneous_soc;

        soc_initialized = 1U;
    }
    else
    {
        /*
         * Apply low-pass filtering.
         */
        soc_filtered =
            BMS_SOC_Filter(
                soc_filtered,
                instantaneous_soc);
    }

    /*
     * --------------------------------------------------------------
     * Endpoint enforcement
     * --------------------------------------------------------------
     *
     * At the actual configured protection endpoints,
     * report the corresponding exact SOC.
     */
    if (BMS_Data.battery_voltage <=
        BMS_MIN_VOLTAGE)
    {
        soc_filtered =
            BMS_SOC_MIN;
    }

    if (BMS_Data.battery_voltage >=
        BMS_MAX_VOLTAGE)
    {
        soc_filtered =
            BMS_SOC_MAX;
    }

    /*
     * --------------------------------------------------------------
     * Final clamp
     * --------------------------------------------------------------
     */
    soc_filtered =
        BMS_SOC_Clamp(
            soc_filtered,
            BMS_SOC_MIN,
            BMS_SOC_MAX);

    /*
     * --------------------------------------------------------------
     * Store SOC
     * --------------------------------------------------------------
     */
    BMS_Data.state_of_charge =
        soc_filtered;
}
