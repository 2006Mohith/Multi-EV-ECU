/*
 * bms_adc.c
 *
 * BMS ADC measurement and signal-processing module.
 *
 * ADC1:
 *   PA0 -> Battery voltage
 *   PA1 -> Battery current
 *
 * Responsibilities:
 *   - ADC acquisition
 *   - Multi-sample averaging
 *   - Voltage conversion
 *   - Current conversion
 *   - Measurement plausibility checking
 *   - Sensor fault detection
 *   - Measurement validity reporting
 */

#include "main.h"
#include "bms_adc.h"
#include "bms_data.h"
#include "bms_config.h"

#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* External variables                                                          */
/* -------------------------------------------------------------------------- */

extern ADC_HandleTypeDef hadc1;

/* -------------------------------------------------------------------------- */
/* Private variables                                                          */
/* -------------------------------------------------------------------------- */

/* Consecutive invalid measurements */
static uint32_t voltage_invalid_count = 0U;
static uint32_t current_invalid_count = 0U;

/* Consecutive valid measurements */
static uint32_t voltage_valid_count = 0U;
static uint32_t current_valid_count = 0U;

/* -------------------------------------------------------------------------- */
/* Private functions                                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief Convert raw ADC count to ADC input voltage.
 *
 * ADC range:
 *   0    -> 0 V
 *   4095 -> ADC_REFERENCE_VOLTAGE
 */
static float BMS_ADC_CountToVoltage(uint32_t adc_count)
{
    if (adc_count > (uint32_t)ADC_MAX_VALUE)
    {
        adc_count = (uint32_t)ADC_MAX_VALUE;
    }

    return ((float)adc_count * ADC_REFERENCE_VOLTAGE) /
           ADC_MAX_VALUE;
}

/**
 * @brief Convert ADC pin voltage to battery-pack voltage.
 *
 * The scale factor is configured in bms_config.h.
 */
static float BMS_ADC_ConvertBatteryVoltage(float adc_voltage)
{
    return adc_voltage * BATTERY_VOLTAGE_SCALE;
}

/**
 * @brief Convert ADC pin voltage to battery current.
 *
 * Current = (Sensor Voltage - Offset) * Scale
 *
 * NOTE:
 * The scale and zero-current offset are provisional until the actual
 * current sensor is calibrated.
 */
static float BMS_ADC_ConvertBatteryCurrent(float adc_voltage)
{
    float corrected_voltage;

    corrected_voltage =
        adc_voltage - CURRENT_SENSOR_OFFSET_VOLTAGE;

    return corrected_voltage * CURRENT_SENSOR_SCALE;
}

/**
 * @brief Read one complete ADC1 conversion sequence.
 *
 * ADC1:
 *   Rank 1 -> Battery voltage
 *   Rank 2 -> Battery current
 */
static HAL_StatusTypeDef BMS_ADC_ReadSequence(
    uint32_t *voltage_adc,
    uint32_t *current_adc)
{
    if ((voltage_adc == NULL) ||
        (current_adc == NULL))
    {
        return HAL_ERROR;
    }

    /* Start ADC */
    if (HAL_ADC_Start(&hadc1) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* ---------------------------------------------------------- */
    /* Rank 1: battery voltage                                   */
    /* ---------------------------------------------------------- */

    if (HAL_ADC_PollForConversion(&hadc1, 100U) != HAL_OK)
    {
        (void)HAL_ADC_Stop(&hadc1);
        return HAL_ERROR;
    }

    *voltage_adc = HAL_ADC_GetValue(&hadc1);

    /* ---------------------------------------------------------- */
    /* Rank 2: battery current                                   */
    /* ---------------------------------------------------------- */

    if (HAL_ADC_PollForConversion(&hadc1, 100U) != HAL_OK)
    {
        (void)HAL_ADC_Stop(&hadc1);
        return HAL_ERROR;
    }

    *current_adc = HAL_ADC_GetValue(&hadc1);

    /* Stop ADC */
    if (HAL_ADC_Stop(&hadc1) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* ---------------------------------------------------------- */
    /* Raw ADC validation                                        */
    /* ---------------------------------------------------------- */

    if (*voltage_adc > (uint32_t)BMS_ADC_MAX_VALID)
    {
        return HAL_ERROR;
    }

    if (*current_adc > (uint32_t)BMS_ADC_MAX_VALID)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief Read and average multiple ADC1 conversion sequences.
 */
static HAL_StatusTypeDef BMS_ADC_ReadFiltered(
    uint32_t *voltage_average,
    uint32_t *current_average)
{
    uint32_t voltage_sum = 0U;
    uint32_t current_sum = 0U;

    uint32_t voltage_sample = 0U;
    uint32_t current_sample = 0U;

    uint32_t i;

    if ((voltage_average == NULL) ||
        (current_average == NULL))
    {
        return HAL_ERROR;
    }

    /*
     * Collect configured number of samples.
     */
    for (i = 0U;
         i < BMS_ADC_FILTER_SAMPLES;
         i++)
    {
        if (BMS_ADC_ReadSequence(
                &voltage_sample,
                &current_sample) != HAL_OK)
        {
            return HAL_ERROR;
        }

        voltage_sum += voltage_sample;
        current_sum += current_sample;
    }

    /*
     * Calculate integer averages.
     */
    *voltage_average =
        voltage_sum / BMS_ADC_FILTER_SAMPLES;

    *current_average =
        current_sum / BMS_ADC_FILTER_SAMPLES;

    return HAL_OK;
}

/**
 * @brief Process battery-voltage validity and sensor status.
 */
static void BMS_ADC_ProcessVoltage(float battery_voltage)
{
    if ((battery_voltage >= BMS_VALID_VOLTAGE_MIN) &&
        (battery_voltage <= BMS_VALID_VOLTAGE_MAX))
    {
        /* Store valid value */
        BMS_Data.battery_voltage = battery_voltage;

        BMS_Data.voltage_valid =
            BMS_MEASUREMENT_VALID;

        /* Reset invalid counter */
        voltage_invalid_count = 0U;

        /* Count consecutive valid readings */
        if (voltage_valid_count <
            BMS_REQUIRED_VALID_MEASUREMENTS)
        {
            voltage_valid_count++;
        }

        /*
         * Clear sensor fault only after enough consecutive
         * valid measurements are available.
         */
        if (voltage_valid_count >=
            BMS_REQUIRED_VALID_MEASUREMENTS)
        {
            BMS_Data.voltage_sensor_fault = 0U;
        }
    }
    else
    {
        /* Invalid measurement */
        BMS_Data.voltage_valid =
            BMS_MEASUREMENT_INVALID;

        voltage_valid_count = 0U;

        /*
         * Count consecutive invalid readings.
         */
        if (voltage_invalid_count <
            BMS_MAX_INVALID_MEASUREMENTS)
        {
            voltage_invalid_count++;
        }

        /*
         * Declare persistent sensor fault.
         */
        if (voltage_invalid_count >=
            BMS_MAX_INVALID_MEASUREMENTS)
        {
            BMS_Data.voltage_sensor_fault = 1U;
        }
    }
}

/**
 * @brief Process battery-current validity and sensor status.
 */
static void BMS_ADC_ProcessCurrent(float battery_current)
{
    if ((battery_current >= BMS_VALID_CURRENT_MIN) &&
        (battery_current <= BMS_VALID_CURRENT_MAX))
    {
        /* Store valid value */
        BMS_Data.battery_current = battery_current;

        BMS_Data.current_valid =
            BMS_MEASUREMENT_VALID;

        /* Reset invalid counter */
        current_invalid_count = 0U;

        /* Count consecutive valid readings */
        if (current_valid_count <
            BMS_REQUIRED_VALID_MEASUREMENTS)
        {
            current_valid_count++;
        }

        /*
         * Clear sensor fault only after enough consecutive
         * valid measurements are available.
         */
        if (current_valid_count >=
            BMS_REQUIRED_VALID_MEASUREMENTS)
        {
            BMS_Data.current_sensor_fault = 0U;
        }
    }
    else
    {
        /* Invalid measurement */
        BMS_Data.current_valid =
            BMS_MEASUREMENT_INVALID;

        current_valid_count = 0U;

        /*
         * Count consecutive invalid readings.
         */
        if (current_invalid_count <
            BMS_MAX_INVALID_MEASUREMENTS)
        {
            current_invalid_count++;
        }

        /*
         * Declare persistent sensor fault.
         */
        if (current_invalid_count >=
            BMS_MAX_INVALID_MEASUREMENTS)
        {
            BMS_Data.current_sensor_fault = 1U;
        }
    }
}

/* -------------------------------------------------------------------------- */
/* Public functions                                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize BMS ADC software.
 *
 * Hardware ADC initialization is performed by CubeMX-generated
 * MX_ADC1_Init() in adc.c/main.c.
 */
void BMS_ADC_Init(void)
{
    /* Reset diagnostic counters */
    voltage_invalid_count = 0U;
    current_invalid_count = 0U;

    voltage_valid_count = 0U;
    current_valid_count = 0U;

    /* Initialize measurement values */
    BMS_Data.battery_voltage = 0.0f;
    BMS_Data.battery_current = 0.0f;

    /* Measurements start as invalid until actual ADC data is received */
    BMS_Data.voltage_valid =
        BMS_MEASUREMENT_INVALID;

    BMS_Data.current_valid =
        BMS_MEASUREMENT_INVALID;

    /* Reset sensor-fault status */
    BMS_Data.voltage_sensor_fault = 0U;
    BMS_Data.current_sensor_fault = 0U;

    /* Reset diagnostic counter */
    BMS_Data.adc_update_count = 0U;
}

/**
 * @brief Update battery voltage and current measurements.
 */
void BMS_ADC_Update(void)
{
    uint32_t voltage_adc = 0U;
    uint32_t current_adc = 0U;

    float voltage_pin;
    float current_pin;

    float battery_voltage;
    float battery_current;

    /* ---------------------------------------------------------- */
    /* Read filtered ADC measurements                             */
    /* ---------------------------------------------------------- */

    if (BMS_ADC_ReadFiltered(
            &voltage_adc,
            &current_adc) != HAL_OK)
    {
        /*
         * ADC acquisition failure is considered a sensor fault.
         *
         * Do not silently retain an old "valid" measurement.
         */
        BMS_Data.voltage_valid =
            BMS_MEASUREMENT_INVALID;

        BMS_Data.current_valid =
            BMS_MEASUREMENT_INVALID;

        BMS_Data.voltage_sensor_fault = 1U;
        BMS_Data.current_sensor_fault = 1U;

        voltage_invalid_count =
            BMS_MAX_INVALID_MEASUREMENTS;

        current_invalid_count =
            BMS_MAX_INVALID_MEASUREMENTS;

        voltage_valid_count = 0U;
        current_valid_count = 0U;

        BMS_Data.adc_update_count++;

        return;
    }

    /* ---------------------------------------------------------- */
    /* Convert ADC counts to pin voltage                          */
    /* ---------------------------------------------------------- */

    voltage_pin =
        BMS_ADC_CountToVoltage(voltage_adc);

    current_pin =
        BMS_ADC_CountToVoltage(current_adc);

    /* ---------------------------------------------------------- */
    /* Convert to physical units                                 */
    /* ---------------------------------------------------------- */

    battery_voltage =
        BMS_ADC_ConvertBatteryVoltage(
            voltage_pin);

    battery_current =
        BMS_ADC_ConvertBatteryCurrent(
            current_pin);

    /* ---------------------------------------------------------- */
    /* Process battery voltage                                   */
    /* ---------------------------------------------------------- */

    BMS_ADC_ProcessVoltage(
        battery_voltage);

    /* ---------------------------------------------------------- */
    /* Process battery current                                   */
    /* ---------------------------------------------------------- */

    BMS_ADC_ProcessCurrent(
        battery_current);

    /* ---------------------------------------------------------- */
    /* Diagnostic counter                                         */
    /* ---------------------------------------------------------- */

    BMS_Data.adc_update_count++;
}
