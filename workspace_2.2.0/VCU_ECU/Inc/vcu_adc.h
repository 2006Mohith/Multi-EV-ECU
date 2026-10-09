#ifndef VCU_ADC_H
#define VCU_ADC_H

#include <stdint.h>

#include "vcu_data.h"

/* =========================================================
 * ADC INITIALIZATION / UPDATE
 * ========================================================= */

void VCU_ADC_Init(void);

/*
 * Starts one ADC conversion sequence and updates:
 *   - throttle_raw
 *   - brake_raw
 *   - aux1_raw
 *   - aux2_raw
 *
 * Returns 1 when a complete valid sequence was acquired.
 * Returns 0 when the acquisition failed.
 */
uint8_t VCU_ADC_Update(void);

/* =========================================================
 * ADC VALUE CONVERSION
 * ========================================================= */

/*
 * Convert a raw ADC value (0...4095) to an input voltage.
 */
float VCU_ADC_RawToVoltage(uint16_t raw_value);

/*
 * Convert throttle ADC value to percentage.
 */
float VCU_ADC_ThrottleToPercent(uint16_t raw_value);

/*
 * Convert brake ADC value to percentage.
 */
float VCU_ADC_BrakeToPercent(uint16_t raw_value);

#endif /* VCU_ADC_H */
