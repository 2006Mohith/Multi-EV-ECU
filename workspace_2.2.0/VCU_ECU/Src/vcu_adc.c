#include "vcu_adc.h"

#include "adc.h"

/* =========================================================
 * PRIVATE FUNCTIONS
 * ========================================================= */

static uint8_t VCU_ADC_IsValid(uint16_t value)
{
    if (value > VCU_ADC_MAX_VALID_VALUE)
    {
        return 0U;
    }

    return 1U;
}

/* =========================================================
 * ADC INITIALIZATION
 * ========================================================= */

void VCU_ADC_Init(void)
{
    VCU_Data.input.analog.throttle_raw = 0U;
    VCU_Data.input.analog.brake_raw = 0U;

    VCU_Data.input.analog.aux1_raw = 0U;
    VCU_Data.input.analog.aux2_raw = 0U;

    VCU_Data.input.analog.throttle_percent = 0.0f;
    VCU_Data.input.analog.brake_percent = 0.0f;

    VCU_Data.input.analog.throttle_valid = VCU_INPUT_VALID;
    VCU_Data.input.analog.brake_valid = VCU_INPUT_VALID;
}

/* =========================================================
 * ADC UPDATE
 * ========================================================= */

uint8_t VCU_ADC_Update(void)
{
    uint16_t raw_values[VCU_ADC_CHANNEL_COUNT];

    uint32_t sum = 0U;
    uint8_t i;

    /*
     * ADC1 is configured by CubeMX as:
     *
     * Rank 1 → PA0 / ADC1_IN0 → THROTTLE
     * Rank 2 → PA1 / ADC1_IN1 → BRAKE
     * Rank 3 → PA2 / ADC1_IN2 → AUX1
     * Rank 4 → PA3 / ADC1_IN3 → AUX2
     */

    if (HAL_ADC_Start(&hadc1) != HAL_OK)
    {
        VCU_Data.input.analog.throttle_valid = VCU_INPUT_INVALID;
        VCU_Data.input.analog.brake_valid = VCU_INPUT_INVALID;

        return 0U;
    }

    for (i = 0U; i < VCU_ADC_CHANNEL_COUNT; i++)
    {
        if (HAL_ADC_PollForConversion(&hadc1, 10U) != HAL_OK)
        {
            (void)HAL_ADC_Stop(&hadc1);

            VCU_Data.input.analog.throttle_valid = VCU_INPUT_INVALID;
            VCU_Data.input.analog.brake_valid = VCU_INPUT_INVALID;

            return 0U;
        }

        raw_values[i] = (uint16_t)HAL_ADC_GetValue(&hadc1);
    }

    (void)HAL_ADC_Stop(&hadc1);

    /*
     * Verify all acquired ADC values.
     */
    for (i = 0U; i < VCU_ADC_CHANNEL_COUNT; i++)
    {
        if (!VCU_ADC_IsValid(raw_values[i]))
        {
            VCU_Data.input.analog.throttle_valid = VCU_INPUT_INVALID;
            VCU_Data.input.analog.brake_valid = VCU_INPUT_INVALID;

            return 0U;
        }

        sum += raw_values[i];
    }

    /*
     * 'sum' prevents the compiler from treating the raw-value
     * validation loop as unnecessary. It is intentionally
     * unused after validation.
     */
    (void)sum;

    /* Store raw ADC measurements. */
    VCU_Data.input.analog.throttle_raw = raw_values[VCU_ADC_THROTTLE_CHANNEL];
    VCU_Data.input.analog.brake_raw = raw_values[VCU_ADC_BRAKE_CHANNEL];

    VCU_Data.input.analog.aux1_raw = raw_values[VCU_ADC_AUX1_CHANNEL];
    VCU_Data.input.analog.aux2_raw = raw_values[VCU_ADC_AUX2_CHANNEL];

    /* Convert throttle and brake to percentage. */
    VCU_Data.input.analog.throttle_percent =
        VCU_ADC_ThrottleToPercent(
            VCU_Data.input.analog.throttle_raw);

    VCU_Data.input.analog.brake_percent =
        VCU_ADC_BrakeToPercent(
            VCU_Data.input.analog.brake_raw);

    VCU_Data.input.analog.throttle_valid = VCU_INPUT_VALID;
    VCU_Data.input.analog.brake_valid = VCU_INPUT_VALID;

    return 1U;
}

/* =========================================================
 * RAW ADC → VOLTAGE
 * ========================================================= */

float VCU_ADC_RawToVoltage(uint16_t raw_value)
{
    if (raw_value > VCU_ADC_MAX_VALUE)
    {
        raw_value = VCU_ADC_MAX_VALUE;
    }

    return ((float)raw_value * VCU_ADC_REFERENCE_VOLTAGE)
           / (float)VCU_ADC_MAX_VALUE;
}

/* =========================================================
 * THROTTLE CONVERSION
 * ========================================================= */

float VCU_ADC_ThrottleToPercent(uint16_t raw_value)
{
    float percent;

    if (raw_value > VCU_ADC_MAX_VALUE)
    {
        raw_value = VCU_ADC_MAX_VALUE;
    }

    percent = ((float)raw_value * VCU_THROTTLE_MAX_PERCENT)
              / (float)VCU_ADC_MAX_VALUE;

    if (percent < VCU_THROTTLE_MIN_PERCENT)
    {
        percent = VCU_THROTTLE_MIN_PERCENT;
    }

    if (percent > VCU_THROTTLE_MAX_PERCENT)
    {
        percent = VCU_THROTTLE_MAX_PERCENT;
    }

    if (percent < VCU_THROTTLE_DEADZONE_PERCENT)
    {
        percent = 0.0f;
    }

    return percent;
}

/* =========================================================
 * BRAKE CONVERSION
 * ========================================================= */

float VCU_ADC_BrakeToPercent(uint16_t raw_value)
{
    float percent;

    if (raw_value > VCU_ADC_MAX_VALUE)
    {
        raw_value = VCU_ADC_MAX_VALUE;
    }

    percent = ((float)raw_value * VCU_BRAKE_MAX_PERCENT)
              / (float)VCU_ADC_MAX_VALUE;

    if (percent < VCU_BRAKE_MIN_PERCENT)
    {
        percent = VCU_BRAKE_MIN_PERCENT;
    }

    if (percent > VCU_BRAKE_MAX_PERCENT)
    {
        percent = VCU_BRAKE_MAX_PERCENT;
    }

    if (percent < VCU_BRAKE_DEADZONE_PERCENT)
    {
        percent = 0.0f;
    }

    return percent;
}
