#ifndef VCU_STATE_H
#define VCU_STATE_H

#include <stdint.h>

#include "vcu_data.h"

/* =========================================================
 * VCU STATE MANAGEMENT
 * ========================================================= */

/*
 * Initialize the VCU state machine.
 */
void VCU_State_Init(void);

/*
 * Execute one state-machine update.
 *
 * This function evaluates:
 * - ignition state
 * - emergency input
 * - input plausibility
 * - BMS availability
 * - MCU availability
 * - throttle / brake requests
 *
 * and updates VCU_Data.system_state.
 */
void VCU_State_Update(void);

/* =========================================================
 * STATE REQUESTS
 * ========================================================= */

/*
 * Request VCU enable.
 */
void VCU_State_RequestEnable(void);

/*
 * Request VCU disable.
 */
void VCU_State_RequestDisable(void);

/*
 * Request shutdown.
 */
void VCU_State_RequestShutdown(void);

/*
 * Request fault recovery.
 */
void VCU_State_RequestRecovery(void);

/* =========================================================
 * STATE INFORMATION
 * ========================================================= */

/*
 * Return the current VCU system state.
 */
VCU_SystemState_t VCU_State_Get(void);

/*
 * Returns 1 when the VCU is allowed to drive.
 */
uint8_t VCU_State_IsDriveAllowed(void);

/*
 * Returns 1 when the VCU is in a fault state.
 */
uint8_t VCU_State_IsFaultActive(void);

/*
 * Returns 1 when the VCU is in a warning state.
 */
uint8_t VCU_State_IsWarningActive(void);

#endif /* VCU_STATE_H */
