#ifndef MCU_SYSTEM_H
#define MCU_SYSTEM_H

#include <stdint.h>

/* ============================================================================
 * MCU System Integration Module
 * ============================================================================
 *
 * This module coordinates all MCU software subsystems.
 *
 * Initialization:
 *
 *   Data
 *   Motor
 *   Hall
 *   ADC
 *   CAN
 *   Input
 *   Fault
 *   LCD
 *   Debug
 *   State machine
 *
 * Periodic execution:
 *
 *   ADC
 *   Hall
 *   Input
 *   CAN
 *   Fault monitoring
 *   State machine
 *   Motor
 *   LCD
 *   Debug
 *
 * ========================================================================== */

/* ============================================================================
 * Initialization
 * ========================================================================== */

void MCU_System_Init(void);

/* ============================================================================
 * Main periodic system task
 * ========================================================================== */

void MCU_System_Run(void);

/* ============================================================================
 * Individual system tasks
 * ========================================================================== */

void MCU_System_TaskADC(void);

void MCU_System_TaskHall(void);

void MCU_System_TaskInput(void);

void MCU_System_TaskCAN(void);

void MCU_System_TaskFaults(void);

void MCU_System_TaskState(void);

void MCU_System_TaskMotor(void);

void MCU_System_TaskLCD(void);

void MCU_System_TaskDebug(void);

/* ============================================================================
 * System supervision
 * ========================================================================== */

void MCU_System_Supervise(void);

/* ============================================================================
 * System status
 * ========================================================================== */

uint8_t MCU_System_IsInitialized(void);

uint8_t MCU_System_IsHealthy(void);

uint32_t MCU_System_GetUptime(void);

/* ============================================================================
 * Shutdown
 * ========================================================================== */

void MCU_System_Shutdown(void);

#endif /* MCU_SYSTEM_H */
