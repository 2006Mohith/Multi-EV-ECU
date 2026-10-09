#ifndef VCU_INPUT_H
#define VCU_INPUT_H

#include <stdint.h>

#include "vcu_data.h"

/* =========================================================
 * INPUT INITIALIZATION
 * ========================================================= */

void VCU_Input_Init(void);

/* =========================================================
 * DIGITAL INPUT UPDATE
 * ========================================================= */

/*
 * Reads:
 *   PB10 → IGNITION_INPUT
 *   PB11 → EMERGENCY_INPUT
 *   PB12 → FORWARD_INPUT
 *   PB13 → REVERSE_INPUT
 */
void VCU_Input_UpdateDigital(void);

/* =========================================================
 * DRIVER INPUT PROCESSING
 * ========================================================= */

/*
 * Determines:
 *   - Vehicle direction
 *   - Accelerator request
 *   - Brake request
 *   - Enable request
 */
void VCU_Input_Process(void);

/* =========================================================
 * SAFETY / PLAUSIBILITY
 * ========================================================= */

/*
 * Returns 1 when the current input combination is valid.
 * Returns 0 when a safety/plausibility problem exists.
 */
uint8_t VCU_Input_IsPlausible(void);

/*
 * Returns 1 when both forward and reverse are active.
 */
uint8_t VCU_Input_IsDirectionConflict(void);

/*
 * Returns 1 when emergency input is active.
 */
uint8_t VCU_Input_IsEmergencyActive(void);

/*
 * Returns 1 when ignition input is active.
 */
uint8_t VCU_Input_IsIgnitionActive(void);

#endif /* VCU_INPUT_H */
