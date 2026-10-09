/*
 * mcu_adc.c
 *
 *  Created on: 21-Sept-2026
 *      Author: MohithK
 */


#include "main.h"
#include "adc.h"
#include "mcu_config.h"
#include "mcu_data.h"
#include "mcu_adc.h"

/* ============================================================================
 * ADC / Potentiometer implementation
 *
 * Hardware:
 * PA1 -> ADC1_IN1 -> Potentiometer wiper
 *
 * ADC:
 * 12-bit
 * Software trigger
 * Single conversion
 * Right aligned
 * ========================================================================== */

/* ============================================================================
 * Initialization
 * ========================================================================== */

void MCU_ADC_Init(void)
{
    MCU_Data.adc.pot_raw = 0U;
    MCU_Data.adc.pot_voltage = 0.0f;
    MCU_Data.adc.pot_percent = 0U;
    MCU_Data.adc.valid = MCU_FALSE;

    MCU_Data.task.adc_ok = MCU_FALSE;
}

/* ============================================================================
 * ADC update
 * ========================================================================== */

void MCU_ADC_Update(void)
{
    uint32_t adc_value;

    /*
     * Start one ADC conversion.
     */
    if (HAL_ADC_Start(&hadc1) != HAL_OK)
    {
        MCU_Data.adc.valid = MCU_FALSE;
        MCU_Data.task.adc_ok = MCU_FALSE;
        MCU_Data.fault.potentiometer_fault = MCU_TRUE;

        return;
    }

    /*
     * Wait for the conversion to complete.
     */
    if (HAL_ADC_PollForConversion(&hadc1, 10U) != HAL_OK)
    {
        (void)HAL_ADC_Stop(&hadc1);

        MCU_Data.adc.valid = MCU_FALSE;
        MCU_Data.task.adc_ok = MCU_FALSE;
        MCU_Data.fault.potentiometer_fault = MCU_TRUE;

        return;
    }

    /*
     * Read the converted value.
     */
    adc_value = HAL_ADC_GetValue(&hadc1);

    /*
     * Stop ADC after the single conversion.
     */
    (void)HAL_ADC_Stop(&hadc1);

    /*
     * Store raw value.
     */
    MCU_Data.adc.pot_raw = (uint16_t)adc_value;

    /*
     * Convert to voltage.
     */
    MCU_Data.adc.pot_voltage =
        MCU_ADC_RawToVoltage(MCU_Data.adc.pot_raw);

    /*
     * Convert to percentage.
     */
    MCU_Data.adc.pot_percent =
        MCU_ADC_RawToPercent(MCU_Data.adc.pot_raw);

    /*
     * Mark ADC data as valid.
     */
    MCU_Data.adc.valid = MCU_TRUE;
    MCU_Data.task.adc_ok = MCU_TRUE;

    /*
     * A successful reading clears the local ADC fault indication.
     */
    MCU_Data.fault.potentiometer_fault = MCU_FALSE;
}

/* ============================================================================
 * Raw ADC -> voltage
 * ========================================================================== */

float MCU_ADC_RawToVoltage(uint16_t raw_value)
{
    float voltage;

    if (raw_value > (uint16_t)MCU_ADC_MAX_VALUE)
    {
        raw_value = (uint16_t)MCU_ADC_MAX_VALUE;
    }

    voltage =
        ((float)raw_value * MCU_ADC_REFERENCE_VOLTAGE) /
        MCU_ADC_MAX_VALUE;

    return voltage;
}

/* ============================================================================
 * Raw ADC -> percentage
 * ========================================================================== */

uint8_t MCU_ADC_RawToPercent(uint16_t raw_value)
{
    uint32_t percent;

    if (raw_value >= (uint16_t)MCU_ADC_MAX_VALUE)
    {
        return MCU_PERCENT_MAX;
    }

    percent =
        ((uint32_t)raw_value * MCU_PERCENT_MAX) /
        (uint32_t)MCU_ADC_MAX_VALUE;

    if (percent > MCU_PERCENT_MAX)
    {
        percent = MCU_PERCENT_MAX;
    }

    return (uint8_t)percent;
}

/* ============================================================================
 * Getter: raw ADC
 * ========================================================================== */

uint16_t MCU_ADC_GetRaw(void)
{
    return MCU_Data.adc.pot_raw;
}

/* ============================================================================
 * Getter: voltage
 * ========================================================================== */

float MCU_ADC_GetVoltage(void)
{
    return MCU_Data.adc.pot_voltage;
}

/* ============================================================================
 * Getter: percentage
 * ========================================================================== */

uint8_t MCU_ADC_GetPercent(void)
{
    return MCU_Data.adc.pot_percent;
}

/* ============================================================================
 * ADC validity
 * ========================================================================== */

uint8_t MCU_ADC_IsValid(void)
{
    return MCU_Data.adc.valid;
}

/* ============================================================================
 * Percentage -> PWM compare
 * ========================================================================== */

uint16_t MCU_ADC_PercentToPWMCompare(uint8_t percent)
{
    uint32_t compare_value;

    if (percent >= MCU_PWM_MAX_PERCENT)
    {
        return (uint16_t)MCU_PWM_PERIOD;
    }

    compare_value =
        ((uint32_t)percent * (uint32_t)MCU_PWM_PERIOD) /
        100U;

    return (uint16_t)compare_value;
}
