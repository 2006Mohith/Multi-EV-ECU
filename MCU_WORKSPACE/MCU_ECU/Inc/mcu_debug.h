#ifndef MCU_DEBUG_H
#define MCU_DEBUG_H

#include <stdint.h>

/* ============================================================================
 * MCU Debug Interface
 * ============================================================================
 *
 * USART1:
 *
 * PA9  -> TX
 * PA10 -> RX
 *
 * Configuration:
 *
 * 115200 baud
 * 8 data bits
 * No parity
 * 1 stop bit
 *
 * ========================================================================== */

/* ============================================================================
 * Initialization
 * ========================================================================== */

void MCU_Debug_Init(void);

/* ============================================================================
 * Periodic debug update
 * ========================================================================== */

void MCU_Debug_Update(void);

/* ============================================================================
 * Basic transmit functions
 * ========================================================================== */

void MCU_Debug_SendChar(char character);

void MCU_Debug_SendString(const char *text);

/* ============================================================================
 * Numeric transmit functions
 * ========================================================================== */

void MCU_Debug_SendUInt32(uint32_t value);

void MCU_Debug_SendInt32(int32_t value);

/* ============================================================================
 * Formatted application status
 * ========================================================================== */

void MCU_Debug_PrintSystemStatus(void);

void MCU_Debug_PrintMotorStatus(void);

void MCU_Debug_PrintCANStatus(void);

void MCU_Debug_PrintFaultStatus(void);

void MCU_Debug_PrintADCStatus(void);

void MCU_Debug_PrintHallStatus(void);

/* ============================================================================
 * Debug control
 * ========================================================================== */

void MCU_Debug_RequestUpdate(void);

uint8_t MCU_Debug_IsReady(void);

#endif /* MCU_DEBUG_H */
