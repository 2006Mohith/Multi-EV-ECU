#ifndef VCU_DEBUG_H
#define VCU_DEBUG_H

#include <stdint.h>

/* =========================================================
 * DEBUG INITIALIZATION
 * ========================================================= */

void VCU_Debug_Init(void);

/* =========================================================
 * DEBUG OUTPUT
 * ========================================================= */

/*
 * Send a null-terminated string through USART1.
 */
void VCU_Debug_SendString(const char *message);

/*
 * Send a signed integer value with a label.
 */
void VCU_Debug_SendInt(const char *label, int32_t value);

/*
 * Send an unsigned integer value with a label.
 */
void VCU_Debug_SendUInt(const char *label, uint32_t value);

/*
 * Send a floating-point value with a label.
 *
 * The implementation will avoid printf-style floating-point
 * formatting to keep the STM32F103 firmware small.
 */
void VCU_Debug_SendFloat(const char *label, float value);

#endif /* VCU_DEBUG_H */
