#ifndef VCU_FAULTS_H
#define VCU_FAULTS_H

#include <stdint.h>

#include "vcu_data.h"

/* =========================================================
 * FAULT MANAGER
 * ========================================================= */

/*
 * Initialize the VCU fault manager.
 */
void VCU_Faults_Init(void);

/*
 * Evaluate all currently available VCU fault conditions.
 */
void VCU_Faults_Update(void);

/* =========================================================
 * FAULT CONTROL
 * ========================================================= */

/*
 * Clear recoverable VCU faults.
 */
void VCU_Faults_Clear(void);

/*
 * Determine whether the VCU is safe for fault recovery.
 *
 * Returns:
 *   1 = recovery allowed
 *   0 = recovery not allowed
 */
uint8_t VCU_Faults_CanRecover(void);

/* =========================================================
 * INDIVIDUAL FAULT CHECKS
 * ========================================================= */

uint8_t VCU_Faults_CheckBMSCommunication(void);

uint8_t VCU_Faults_CheckBMSCritical(void);

uint8_t VCU_Faults_CheckMCUCommunication(void);

uint8_t VCU_Faults_CheckMCUCritical(void);

uint8_t VCU_Faults_CheckThrottle(void);

uint8_t VCU_Faults_CheckBrake(void);

uint8_t VCU_Faults_CheckDirection(void);

uint8_t VCU_Faults_CheckEmergency(void);

uint8_t VCU_Faults_CheckIgnition(void);

/* =========================================================
 * FAULT INFORMATION
 * ========================================================= */

/*
 * Returns the currently active fault code.
 */
uint8_t VCU_Faults_GetActiveCode(void);

/*
 * Returns the currently active fault severity.
 */
uint8_t VCU_Faults_GetSeverity(void);

/*
 * Returns 1 if any fault is active.
 */
uint8_t VCU_Faults_IsActive(void);

#endif /* VCU_FAULTS_H */
