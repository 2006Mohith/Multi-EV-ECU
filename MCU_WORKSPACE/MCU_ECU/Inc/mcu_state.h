#ifndef MCU_STATE_H
#define MCU_STATE_H

#include <stdint.h>

/* ============================================================================
 * MCU State Management Module
 * ============================================================================
 *
 * States:
 *
 * INIT     -> Firmware initialization
 * DISABLED -> Motor system disabled
 * READY    -> System healthy and ready to run
 * RUN      -> Motor running
 * BRAKE    -> Controlled braking
 * WARNING  -> Warning condition
 * FAULT    -> Fault condition
 * SHUTDOWN -> System shutdown
 *
 * The state manager does not directly perform low-level GPIO control.
 * It decides the permitted operating state and motor command.
 *
 * ========================================================================== */

/* ============================================================================
 * Initialization
 * ========================================================================== */

void MCU_State_Init(void);

/* ============================================================================
 * Periodic state-machine update
 * ========================================================================== */

void MCU_State_Update(void);

/* ============================================================================
 * Individual state handlers
 * ========================================================================== */

void MCU_State_ProcessInit(void);

void MCU_State_ProcessDisabled(void);

void MCU_State_ProcessReady(void);

void MCU_State_ProcessRun(void);

void MCU_State_ProcessBrake(void);

void MCU_State_ProcessWarning(void);

void MCU_State_ProcessFault(void);

void MCU_State_ProcessShutdown(void);

/* ============================================================================
 * State transition
 * ========================================================================== */

void MCU_State_SetState(uint8_t new_state);

uint8_t MCU_State_GetState(void);

/* ============================================================================
 * State status
 * ========================================================================== */

uint8_t MCU_State_IsInit(void);

uint8_t MCU_State_IsDisabled(void);

uint8_t MCU_State_IsReady(void);

uint8_t MCU_State_IsRunning(void);

uint8_t MCU_State_IsBraking(void);

uint8_t MCU_State_IsWarning(void);

uint8_t MCU_State_IsFault(void);

uint8_t MCU_State_IsShutdown(void);

/* ============================================================================
 * Safety checks
 * ========================================================================== */

uint8_t MCU_State_IsSystemHealthy(void);

uint8_t MCU_State_CanRun(void);

uint8_t MCU_State_CanRecover(void);

/* ============================================================================
 * Fault / shutdown requests
 * ========================================================================== */

void MCU_State_RequestFault(void);

void MCU_State_RequestShutdown(void);

void MCU_State_RequestRecovery(void);

/* ============================================================================
 * Run / stop requests
 * ========================================================================== */

void MCU_State_RequestRun(void);

void MCU_State_RequestStop(void);

void MCU_State_RequestBrake(void);

#endif /* MCU_STATE_H */
