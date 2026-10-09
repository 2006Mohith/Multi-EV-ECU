#ifndef MCU_HALL_H
#define MCU_HALL_H

#include <stdint.h>
#include "mcu_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Hall Sensor / RPM Measurement Module
 *
 * Hardware:
 * PA0 -> TIM2_CH1 -> Hall sensor digital output
 *
 * TIM2 configuration:
 * Prescaler = 71
 * Counter frequency = 1 MHz
 * 1 timer count = 1 us
 *
 * ========================================================================== */

/* ============================================================================
 * Initialization
 * ========================================================================== */

/**
 * @brief Initialize the Hall sensor measurement module.
 *
 * Starts TIM2 input capture on channel 1.
 */
void MCU_Hall_Init(void);

/* ============================================================================
 * Periodic processing
 * ========================================================================== */

/**
 * @brief Process Hall sensor data and update RPM validity.
 *
 * This function should be called periodically from the main control loop.
 */
void MCU_Hall_Update(void);

/* ============================================================================
 * Input capture callback
 * ========================================================================== */

/**
 * @brief Process a new Hall input-capture event.
 *
 * Called from HAL_TIM_IC_CaptureCallback().
 *
 * @param htim Timer handle that generated the capture event.
 */
void MCU_Hall_CaptureCallback(TIM_HandleTypeDef *htim);

/* ============================================================================
 * RPM functions
 * ========================================================================== */

/**
 * @brief Calculate RPM from the measured Hall pulse period.
 *
 * @param period_us Time between two Hall pulses in microseconds.
 *
 * @return Calculated motor RPM.
 */
uint32_t MCU_Hall_CalculateRPM(uint32_t period_us);

/**
 * @brief Return the latest calculated motor RPM.
 *
 * @return Motor RPM.
 */
uint32_t MCU_Hall_GetRPM(void);

/* ============================================================================
 * Status functions
 * ========================================================================== */

/**
 * @brief Return whether a valid Hall pulse has been received.
 *
 * @return 1 if valid pulse data exists, otherwise 0.
 */
uint8_t MCU_Hall_IsValid(void);

/**
 * @brief Return whether the Hall sensor has timed out.
 *
 * @return 1 if timed out, otherwise 0.
 */
uint8_t MCU_Hall_IsTimeout(void);

/**
 * @brief Return the number of Hall pulses detected.
 *
 * @return Total pulse count.
 */
uint32_t MCU_Hall_GetPulseCount(void);

/* ============================================================================
 * Reset
 * ========================================================================== */

/**
 * @brief Reset Hall measurement data.
 *
 * Clears pulse timing and RPM values.
 */
void MCU_Hall_Reset(void);

#ifdef __cplusplus
}
#endif

#endif /* MCU_HALL_H */
