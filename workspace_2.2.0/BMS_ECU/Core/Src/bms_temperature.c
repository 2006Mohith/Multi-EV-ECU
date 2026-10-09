/*
 * bms_temperature.c
 *
 * BMS temperature measurement and processing module.
 *
 * ADC2:
 *   PA2 -> Temperature sensor
 *
 * Responsibilities:
 *   - ADC2 acquisition
 *   - Multi-sample averaging
 *   - Sensor-voltage conversion
 *   - Temperature conversion
 *   - Plausibility checking
 *   - Sensor-fault detection
 *   - Measurement validity
 *   - Minimum/maximum temperature tracking
 *
 * NOTE:
 * The temperature conversion equation is intentionally kept as the
 * currently configured placeholder until the actual temperature sensor
 * and interface circuit are known and calibrated.
 */

#include "main.h"
#include "bms_temperature.h"
#include "bms_data.h"
#include "bms_config.h"

#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* External variables                                                          */
/* -------------------------------------------------------------------------- */

extern ADC_HandleTypeDef hadc2;

/* -------------------------------------------------------------------------- */
/* Private variables                                                          */
/* -------------------------------------------------------------------------- */

static uint32_t temperature_invalid_count = 0U;
static uint32_t temperature_valid_count = 0U;

/* -------------------------------------------------------------------------- */
/* Private functions                                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief Convert ADC count to ADC input voltage.
 */
static float BMS_Temperature_ADCToVoltage(uint32_t adc_count)
{
    /*
     * Clamp raw ADC value to the configured ADC range.
     */
    if (adc_count > (uint32_t)BMS_ADC_MAX_VALID)
    {
        adc_count = (uint32_t)BMS_ADC_MAX_VALID;
    }

    return ((float)adc_count * ADC_REFERENCE_VOLTAGE) /
           ADC_MAX_VALUE;
}

/**
 * @brief Convert sensor voltage to temperature.
 *
 * CURRENTLY CONFIGURED PLACEHOLDER:
 *
 *   0.0 V -> 0 °C
 *   3.3 V -> 100 °C
 *
 * This must be replaced once the actual sensor/interface circuit
 * is known.
 */
static float BMS_Temperature_ConvertSensorVoltage(
    float sensor_voltage)
{
    /*
     * Keep the sensor voltage inside the ADC physical range.
     */
    if (sensor_voltage < 0.0f)
    {
        sensor_voltage = 0.0f;
    }

    if (sensor_voltage > ADC_REFERENCE_VOLTAGE)
    {
        sensor_voltage = ADC_REFERENCE_VOLTAGE;
    }

    return (sensor_voltage / ADC_REFERENCE_VOLTAGE) * 100.0f;
}

/**
 * @brief Read one ADC2 sample.
 */
static HAL_StatusTypeDef BMS_Temperature_ReadSample(
    uint32_t *adc_value)
{
    if (adc_value == NULL)
    {
        return HAL_ERROR;
    }

    /* Start ADC2 */
    if (HAL_ADC_Start(&hadc2) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Wait for conversion */
    if (HAL_ADC_PollForConversion(
            &hadc2,
            100U) != HAL_OK)
    {
        (void)HAL_ADC_Stop(&hadc2);

        return HAL_ERROR;
    }

    /* Read conversion result */
    *adc_value =
        HAL_ADC_GetValue(&hadc2);

    /* Stop ADC2 */
    if (HAL_ADC_Stop(&hadc2) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Validate raw ADC result */
    if (*adc_value > (uint32_t)BMS_ADC_MAX_VALID)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief Read and average multiple temperature ADC samples.
 */
static HAL_StatusTypeDef BMS_Temperature_ReadFiltered(
    uint32_t *adc_average)
{
    uint32_t sum = 0U;
    uint32_t sample = 0U;
    uint32_t i;

    if (adc_average == NULL)
    {
        return HAL_ERROR;
    }

    /*
     * Collect the configured number of samples.
     */
    for (i = 0U;
         i < BMS_ADC_FILTER_SAMPLES;
         i++)
    {
        if (BMS_Temperature_ReadSample(
                &sample) != HAL_OK)
        {
            return HAL_ERROR;
        }

        sum += sample;
    }

    /*
     * Calculate average.
     */
    *adc_average =
        sum / BMS_ADC_FILTER_SAMPLES;

    return HAL_OK;
}

/**
 * @brief Process a valid temperature measurement.
 */
static void BMS_Temperature_ProcessValid(
    float temperature)
{
    /*
     * Store latest temperature.
     */
    BMS_Data.battery_temperature =
        temperature;

    /*
     * Mark measurement valid.
     */
    BMS_Data.temperature_valid =
        BMS_MEASUREMENT_VALID;

    /*
     * Reset invalid counter.
     */
    temperature_invalid_count = 0U;

    /*
     * Count consecutive valid measurements.
     */
    if (temperature_valid_count <
        BMS_REQUIRED_VALID_MEASUREMENTS)
    {
        temperature_valid_count++;
    }

    /*
     * Clear sensor fault only after enough consecutive
     * valid measurements.
     */
    if (temperature_valid_count >=
        BMS_REQUIRED_VALID_MEASUREMENTS)
    {
        BMS_Data.temperature_sensor_fault = 0U;
    }

    /*
     * Track observed minimum temperature.
     */
    if (temperature <
        BMS_Data.temperature_min)
    {
        BMS_Data.temperature_min =
            temperature;
    }

    /*
     * Track observed maximum temperature.
     */
    if (temperature >
        BMS_Data.temperature_max)
    {
        BMS_Data.temperature_max =
            temperature;
    }
}

/**
 * @brief Process an invalid temperature measurement.
 */
static void BMS_Temperature_ProcessInvalid(void)
{
    /*
     * Mark temperature invalid.
     */
    BMS_Data.temperature_valid =
        BMS_MEASUREMENT_INVALID;

    /*
     * Reset valid counter.
     */
    temperature_valid_count = 0U;

    /*
     * Count consecutive invalid measurements.
     */
    if (temperature_invalid_count <
        BMS_MAX_INVALID_MEASUREMENTS)
    {
        temperature_invalid_count++;
    }

    /*
     * Persistent invalid measurements cause a sensor fault.
     */
    if (temperature_invalid_count >=
        BMS_MAX_INVALID_MEASUREMENTS)
    {
        BMS_Data.temperature_sensor_fault = 1U;
    }
}

/* -------------------------------------------------------------------------- */
/* Public functions                                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize temperature module.
 */
void BMS_Temperature_Init(void)
{
    /*
     * Reset diagnostic counters.
     */
    temperature_invalid_count = 0U;
    temperature_valid_count = 0U;

    /*
     * Initialize temperature value.
     */
    BMS_Data.battery_temperature =
        BMS_DEFAULT_TEMPERATURE;

    /*
     * Measurement starts invalid until the ADC
     * produces valid measurements.
     */
    BMS_Data.temperature_valid =
        BMS_MEASUREMENT_INVALID;

    /*
     * No sensor fault initially.
     */
    BMS_Data.temperature_sensor_fault = 0U;

    /*
     * Initialize observed temperature range.
     */
    BMS_Data.temperature_min =
        BMS_DEFAULT_TEMPERATURE;

    BMS_Data.temperature_max =
        BMS_DEFAULT_TEMPERATURE;
}

/**
 * @brief Read and update battery temperature.
 */
void BMS_Temperature_Update(void)
{
    uint32_t adc_average = 0U;

    float sensor_voltage;
    float temperature;

    /*
     * --------------------------------------------------------------
     * Read filtered ADC2 measurement
     * --------------------------------------------------------------
     */
    if (BMS_Temperature_ReadFiltered(
            &adc_average) != HAL_OK)
    {
        BMS_Temperature_ProcessInvalid();

        return;
    }

    /*
     * --------------------------------------------------------------
     * Convert ADC counts to sensor voltage
     * --------------------------------------------------------------
     */
    sensor_voltage =
        BMS_Temperature_ADCToVoltage(
            adc_average);

    /*
     * --------------------------------------------------------------
     * Convert sensor voltage to temperature
     * --------------------------------------------------------------
     */
    temperature =
        BMS_Temperature_ConvertSensorVoltage(
            sensor_voltage);

    /*
     * --------------------------------------------------------------
     * Temperature plausibility check
     * --------------------------------------------------------------
     */
    if ((temperature < BMS_VALID_TEMPERATURE_MIN) ||
        (temperature > BMS_VALID_TEMPERATURE_MAX))
    {
        BMS_Temperature_ProcessInvalid();

        return;
    }

    /*
     * --------------------------------------------------------------
     * Store valid result
     * --------------------------------------------------------------
     */
    BMS_Temperature_ProcessValid(
        temperature);
}
