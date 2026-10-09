#ifndef MCU_FAULTS_H
#define MCU_FAULTS_H

#include <stdint.h>

/* ============================================================================
 * MCU Fault Management Module
 * ============================================================================
 *
 * Fault sources:
 *
 * CAN
 * VCU communication
 * VCU command
 * Hall sensor
 * Hall timeout
 * Direction conflict
 * Motor driver
 * Potentiometer
 * Buttons
 * System
 *
 * The fault module updates MCU_Data.fault and requests the state machine
 * to enter FAULT when a serious fault is detected.
 *
 * ========================================================================== */

/* ============================================================================
 * Initialization
 * ========================================================================== */

void MCU_Faults_Init(void);

/* ============================================================================
 * Periodic fault monitoring
 * ========================================================================== */

void MCU_Faults_Update(void);

/* ============================================================================
 * Fault evaluation
 * ========================================================================== */

void MCU_Faults_Evaluate(void);

/* ============================================================================
 * Individual fault setters
 * ========================================================================== */

void MCU_Faults_SetCANFault(void);

void MCU_Faults_SetVCUCommFault(void);

void MCU_Faults_SetCommandFault(void);

void MCU_Faults_SetHallFault(void);

void MCU_Faults_SetHallTimeoutFault(void);

void MCU_Faults_SetDirectionFault(void);

void MCU_Faults_SetMotorDriverFault(void);

void MCU_Faults_SetPotentiometerFault(void);

void MCU_Faults_SetButtonFault(void);

void MCU_Faults_SetSystemFault(void);

/* ============================================================================
 * Fault clearing
 * ========================================================================== */

void MCU_Faults_ClearCANFault(void);

void MCU_Faults_ClearVCUCommFault(void);

void MCU_Faults_ClearCommandFault(void);

void MCU_Faults_ClearHallFault(void);

void MCU_Faults_ClearHallTimeoutFault(void);

void MCU_Faults_ClearDirectionFault(void);

void MCU_Faults_ClearMotorDriverFault(void);

void MCU_Faults_ClearPotentiometerFault(void);

void MCU_Faults_ClearButtonFault(void);

void MCU_Faults_ClearSystemFault(void);

/* ============================================================================
 * Fault reset
 * ========================================================================== */

void MCU_Faults_RequestReset(void);

void MCU_Faults_Reset(void);

/* ============================================================================
 * Fault status
 * ========================================================================== */

uint8_t MCU_Faults_IsActive(void);

uint8_t MCU_Faults_IsLatched(void);

uint8_t MCU_Faults_GetCode(void);

uint8_t MCU_Faults_GetSeverity(void);

/* ============================================================================
 * Recovery
 * ========================================================================== */

uint8_t MCU_Faults_CanRecover(void);

uint8_t MCU_Faults_ProcessRecovery(void);

/* ============================================================================
 * Fault confirmation
 * ========================================================================== */

uint8_t MCU_Faults_GetConfirmationCount(void);

uint8_t MCU_Faults_GetRecoveryCount(void);

#endif /* MCU_FAULTS_H */
