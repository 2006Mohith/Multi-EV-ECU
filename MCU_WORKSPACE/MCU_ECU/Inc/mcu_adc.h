#ifndef MCU_ADC_H
#define MCU_ADC_H

#include <stdint.h>
#include "mcu_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * MCU ADC / Potentiometer Module
 *
 * Hardware:
 * PA1 -> ADC1_IN1 -> Potentiometer wiper
 *
 * ADC configuration:
 * - 12-bit
 * - Right aligned
 * - Software trigger
 * - Single conversion
 * - Sampling time = 55.5 cycles
 * ========================================================================== */

/* ============================================================================
 * Initialization
 * ========================================================================== */

/**
 * @brief Initialize the ADC/potentiometer software module.
 */
void MCU_ADC_Init(void);

/* ============================================================================
 * ADC update
 * ========================================================================== */

/**
 * @brief Read the potentiometer and update MCU_Data.
 *
 * Performs one ADC conversion and converts the result into:
 * - Raw ADC value
 * - Potentiometer voltage
 * - Potentiometer percentage
 */
void MCU_ADC_Update(void);

/* ============================================================================
 * Conversion functions
 * ========================================================================== */

/**
 * @brief Convert ADC raw value to voltage.
 *
 * @param raw_value 12-bit ADC value from 0 to 4095.
 *
 * @return Measured voltage in volts.
 */
float MCU_ADC_RawToVoltage(uint16_t raw_value);

/**
 * @brief Convert ADC raw value to percentage.
 *
 * @param raw_value 12-bit ADC value from 0 to 4095.
 *
 * @return Percentage from 0 to 100.
 */
uint8_t MCU_ADC_RawToPercent(uint16_t raw_value);

/* ============================================================================
 * Getter functions
 * ========================================================================== */

/**
 * @brief Get the latest raw potentiometer ADC value.
 *
 * @return ADC value from 0 to 4095.
 */
uint16_t MCU_ADC_GetRaw(void);

/**
 * @brief Get the latest potentiometer voltage.
 *
 * @return Voltage in volts.
 */
float MCU_ADC_GetVoltage(void);

/**
 * @brief Get the latest potentiometer percentage.
 *
 * @return Percentage from 0 to 100.
 */
uint8_t MCU_ADC_GetPercent(void);

/**
 * @brief Check whether the latest ADC reading is valid.
 *
 * @return 1 if valid, otherwise 0.
 */
uint8_t MCU_ADC_IsValid(void);

/* ============================================================================
 * Utility
 * ========================================================================== */

/**
 * @brief Convert a percentage into a PWM compare value.
 *
 * @param percent Percentage from 0 to 100.
 *
 * @return TIM1 compare value.
 */
uint16_t MCU_ADC_PercentToPWMCompare(uint8_t percent);

#ifdef __cplusplus
}
#endif

#endif /* MCU_ADC_H */
