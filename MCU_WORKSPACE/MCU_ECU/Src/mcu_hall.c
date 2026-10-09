/*
 * mcu_hall.c
 *
 *  Created on: 21-Sept-2026
 *      Author: MohithK
 */


#include "main.h"
#include "tim.h"
#include "mcu_config.h"
#include "mcu_data.h"
#include "mcu_hall.h"

/* ============================================================================
 * Hall Sensor / RPM Measurement
 *
 * PA0 -> TIM2_CH1
 *
 * TIM2:
 *   Timer clock = 72 MHz
 *   Prescaler   = 71
 *   Counter     = 1 MHz
 *   1 count     = 1 us
 * ========================================================================== */

/* ============================================================================
 * Private helpers
 * ========================================================================== */

static uint32_t MCU_Hall_GetElapsedTicks(uint32_t previous,
                                         uint32_t current)
{
    /*
     * TIM2 is a 16-bit up-counter.
     * Handle timer wrap-around explicitly.
     */
    if (current >= previous)
    {
        return (current - previous);
    }

    return ((65536UL - previous) + current);
}

/* ============================================================================
 * Initialization
 * ========================================================================== */

void MCU_Hall_Init(void)
{
    MCU_Hall_Reset();

    /*
     * Start TIM2 input capture on channel 1.
     */
    if (HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    MCU_Data.task.hall_ok = MCU_TRUE;
}

/* ============================================================================
 * Periodic update
 * ========================================================================== */

void MCU_Hall_Update(void)
{
    uint32_t now;
    uint32_t elapsed_since_capture;

    /*
     * If we haven't received a Hall pulse yet, wait for one.
     */
    if (MCU_Data.hall.pulse_received == MCU_FALSE)
    {
        MCU_Data.hall.timeout = MCU_TRUE;
        MCU_Data.hall.signal_valid = MCU_FALSE;
        MCU_Data.hall.rpm = 0U;

        return;
    }

    /*
     * Check how long it has been since the most recent Hall pulse.
     */
    now = HAL_GetTick();

    elapsed_since_capture =
        now - MCU_Data.hall.last_capture;

    /*
     * No Hall pulse for the configured timeout period:
     * motor may be stopped, or Hall signal may be missing.
     */
    if (elapsed_since_capture > MCU_HALL_TIMEOUT_MS)
    {
        MCU_Data.hall.timeout = MCU_TRUE;
        MCU_Data.hall.signal_valid = MCU_FALSE;
        MCU_Data.hall.rpm = 0U;

        return;
    }

    MCU_Data.hall.timeout = MCU_FALSE;
    MCU_Data.hall.signal_valid = MCU_TRUE;
}

/* ============================================================================
 * Input capture callback processing
 * ========================================================================== */

void MCU_Hall_CaptureCallback(TIM_HandleTypeDef *htim)
{
    uint32_t capture_value;
    uint32_t period_us;
    uint32_t rpm;

    if (htim == NULL)
    {
        return;
    }

    if ((htim->Instance != TIM2) ||
        (htim->Channel != HAL_TIM_ACTIVE_CHANNEL_1))
    {
        return;
    }

    /*
     * Read the new captured timer value.
     */
    capture_value = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

    /*
     * First valid capture establishes the reference point.
     */
    if (MCU_Data.hall.pulse_received == MCU_FALSE)
    {
        MCU_Data.hall.last_capture = capture_value;
        MCU_Data.hall.previous_capture = capture_value;

        MCU_Data.hall.pulse_received = MCU_TRUE;
        MCU_Data.hall.signal_valid = MCU_TRUE;
        MCU_Data.hall.timeout = MCU_FALSE;

        MCU_Data.hall.pulse_count++;

        /*
         * Save system time for Hall timeout supervision.
         */
        /* The variable is reused as a millisecond timestamp. */
        MCU_Data.hall.last_capture = HAL_GetTick();

        return;
    }

    /*
     * At this point the software variable last_capture contains
     * a millisecond timestamp, while previous_capture contains
     * the timer capture value from the previous edge.
     *
     * We therefore use previous_capture for timer-period measurement.
     */
    period_us = MCU_Hall_GetElapsedTicks(
                    MCU_Data.hall.previous_capture,
                    capture_value);

    /*
     * Reject a zero-period capture.
     */
    if (period_us == 0U)
    {
        return;
    }

    /*
     * Save current timer capture for the next edge.
     */
    MCU_Data.hall.previous_capture = capture_value;

    /*
     * Save millisecond timestamp for timeout supervision.
     */
    MCU_Data.hall.last_capture = HAL_GetTick();

    MCU_Data.hall.pulse_period_us = period_us;

    /*
     * Calculate RPM.
     */
    rpm = MCU_Hall_CalculateRPM(period_us);

    if (rpm > MCU_RPM_MAX)
    {
        rpm = MCU_RPM_MAX;
    }

    MCU_Data.hall.rpm = rpm;

    MCU_Data.hall.pulse_received = MCU_TRUE;
    MCU_Data.hall.signal_valid = MCU_TRUE;
    MCU_Data.hall.timeout = MCU_FALSE;

    MCU_Data.hall.pulse_count++;
}

/* ============================================================================
 * RPM calculation
 * ========================================================================== */

uint32_t MCU_Hall_CalculateRPM(uint32_t period_us)
{
    uint32_t rpm;

    if (period_us == 0U)
    {
        return 0U;
    }

    /*
     * RPM formula:
     *
     * RPM =
     *   60,000,000
     *   -----------------------------
     *   period_us × pulses_per_rev
     *
     * For the current provisional configuration:
     * pulses_per_rev = 1.
     */
    rpm = 60000000UL /
          (period_us * MCU_HALL_PULSES_PER_REV);

    return rpm;
}

/* ============================================================================
 * RPM getter
 * ========================================================================== */

uint32_t MCU_Hall_GetRPM(void)
{
    return MCU_Data.hall.rpm;
}

/* ============================================================================
 * Hall validity
 * ========================================================================== */

uint8_t MCU_Hall_IsValid(void)
{
    if ((MCU_Data.hall.signal_valid != MCU_FALSE) &&
        (MCU_Data.hall.timeout == MCU_FALSE))
    {
        return MCU_TRUE;
    }

    return MCU_FALSE;
}

/* ============================================================================
 * Hall timeout
 * ========================================================================== */

uint8_t MCU_Hall_IsTimeout(void)
{
    return MCU_Data.hall.timeout;
}

/* ============================================================================
 * Pulse counter
 * ========================================================================== */

uint32_t MCU_Hall_GetPulseCount(void)
{
    return MCU_Data.hall.pulse_count;
}

/* ============================================================================
 * Reset
 * ========================================================================== */

void MCU_Hall_Reset(void)
{
    MCU_Data.hall.last_capture = 0U;
    MCU_Data.hall.previous_capture = 0U;
    MCU_Data.hall.pulse_period_us = 0U;
    MCU_Data.hall.rpm = 0U;

    MCU_Data.hall.pulse_received = MCU_FALSE;
    MCU_Data.hall.signal_valid = MCU_FALSE;
    MCU_Data.hall.timeout = MCU_TRUE;

    MCU_Data.hall.pulse_count = 0U;
}
