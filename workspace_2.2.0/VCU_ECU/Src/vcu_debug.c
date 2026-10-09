/*
 * vcu_debug.c
 *
 *  Created on: 15-Sept-2026
 *      Author: MohithK
 */


#include "vcu_debug.h"

#include "usart.h"

/* =========================================================
 * PRIVATE HELPERS
 * ========================================================= */

static void VCU_Debug_SendChar(char character)
{
    uint8_t data;

    data = (uint8_t)character;

    (void)HAL_UART_Transmit(
        &huart1,
        &data,
        1U,
        100U);
}

static void VCU_Debug_SendUnsignedValue(uint32_t value)
{
    char buffer[11];
    uint8_t index;

    index = 0U;

    if (value == 0U)
    {
        VCU_Debug_SendChar('0');
        return;
    }

    while ((value > 0U) && (index < 10U))
    {
        buffer[index] = (char)('0' + (value % 10U));
        value /= 10U;
        index++;
    }

    while (index > 0U)
    {
        index--;
        VCU_Debug_SendChar(buffer[index]);
    }
}

/* =========================================================
 * INITIALIZATION
 * ========================================================= */

void VCU_Debug_Init(void)
{
    /*
     * USART1 is initialized by CubeMX through:
     *
     * MX_USART1_UART_Init()
     *
     * No additional peripheral initialization is required.
     */
}

/* =========================================================
 * SEND STRING
 * ========================================================= */

void VCU_Debug_SendString(const char *message)
{
    if (message == NULL)
    {
        return;
    }

    while (*message != '\0')
    {
        VCU_Debug_SendChar(*message);
        message++;
    }
}

/* =========================================================
 * SEND SIGNED INTEGER
 * ========================================================= */

void VCU_Debug_SendInt(const char *label,
                       int32_t value)
{
    uint32_t magnitude;

    if (label != NULL)
    {
        VCU_Debug_SendString(label);
    }

    if (value < 0)
    {
        VCU_Debug_SendChar('-');

        /*
         * Avoid signed overflow when value is INT32_MIN.
         */
        magnitude = (uint32_t)(-(value + 1));
        magnitude += 1U;
    }
    else
    {
        magnitude = (uint32_t)value;
    }

    VCU_Debug_SendUnsignedValue(magnitude);

    VCU_Debug_SendString("\r\n");
}

/* =========================================================
 * SEND UNSIGNED INTEGER
 * ========================================================= */

void VCU_Debug_SendUInt(const char *label,
                        uint32_t value)
{
    if (label != NULL)
    {
        VCU_Debug_SendString(label);
    }

    VCU_Debug_SendUnsignedValue(value);

    VCU_Debug_SendString("\r\n");
}

/* =========================================================
 * SEND FLOAT
 * ========================================================= */

void VCU_Debug_SendFloat(const char *label,
                         float value)
{
    uint32_t integer_part;
    uint32_t fractional_part;
    uint8_t negative;

    negative = 0U;

    if (label != NULL)
    {
        VCU_Debug_SendString(label);
    }

    /*
     * Handle negative values without relying on printf("%f").
     */
    if (value < 0.0f)
    {
        negative = 1U;
        value = -value;
    }

    if (negative != 0U)
    {
        VCU_Debug_SendChar('-');
    }

    integer_part = (uint32_t)value;

    fractional_part =
        (uint32_t)((value - (float)integer_part) * 100.0f);

    /*
     * Limit rounding overflow.
     */
    if (fractional_part >= 100U)
    {
        integer_part += 1U;
        fractional_part = 0U;
    }

    VCU_Debug_SendUnsignedValue(integer_part);

    VCU_Debug_SendChar('.');

    VCU_Debug_SendChar(
        (char)('0' + ((fractional_part / 10U) % 10U)));

    VCU_Debug_SendChar(
        (char)('0' + (fractional_part % 10U)));

    VCU_Debug_SendString("\r\n");
}
