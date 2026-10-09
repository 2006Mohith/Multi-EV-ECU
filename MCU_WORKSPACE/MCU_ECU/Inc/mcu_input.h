#ifndef MCU_INPUT_H
#define MCU_INPUT_H

#include <stdint.h>

/* ============================================================================
 * MCU Input Module
 * ============================================================================
 *
 * START   -> PB10
 * STOP    -> PB11
 * FORWARD -> PB12
 * REVERSE -> PB13
 *
 * Buttons use internal pull-up resistors:
 *
 * Released = GPIO HIGH
 * Pressed  = GPIO LOW
 *
 * ========================================================================== */

/* ============================================================================
 * Initialization
 * ========================================================================== */

void MCU_Input_Init(void);

/* ============================================================================
 * Periodic input update
 * ========================================================================== */

void MCU_Input_Update(void);

/* ============================================================================
 * Individual button status
 * ========================================================================== */

uint8_t MCU_Input_IsStartPressed(void);

uint8_t MCU_Input_IsStopPressed(void);

uint8_t MCU_Input_IsForwardPressed(void);

uint8_t MCU_Input_IsReversePressed(void);

/* ============================================================================
 * Input validation
 * ========================================================================== */

uint8_t MCU_Input_IsDirectionConflict(void);

uint8_t MCU_Input_AreInputsValid(void);

/* ============================================================================
 * Direction request
 * ========================================================================== */

uint8_t MCU_Input_GetRequestedDirection(void);

/* ============================================================================
 * Clear button states
 * ========================================================================== */

void MCU_Input_Clear(void);

#endif /* MCU_INPUT_H */
