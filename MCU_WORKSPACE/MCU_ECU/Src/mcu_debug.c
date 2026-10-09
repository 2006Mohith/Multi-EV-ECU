#include "main.h"
#include "usart.h"
#include "mcu_config.h"
#include "mcu_data.h"
#include "mcu_debug.h"

/* ============================================================================
 * MCU Debug UART
 * ============================================================================
 *
 * USART1:
 *
 * PA9  -> TX
 * PA10 -> RX
 *
 * 115200 baud, 8N1
 *
 * Debug timing is kept locally in this module so no additional fields
 * are required in MCU_Data_t.
 *
 * ========================================================================== */

#define MCU_DEBUG_PERIOD_MS    1000U

/* ============================================================================
 * Private variables
 * ========================================================================== */

static uint8_t mcu_debug_initialized = MCU_FALSE;
static uint8_t mcu_debug_update_requested = MCU_TRUE;
static uint32_t mcu_debug_last_update_time = 0U;

/* ============================================================================
 * Private helper functions
 * ========================================================================== */

static void MCU_Debug_SendUInt8(uint8_t value)
{
    char buffer[4];
    uint8_t index = 0U;

    if (value >= 100U)
    {
        buffer[index++] = (char)('0' + (value / 100U));
        value = (uint8_t)(value % 100U);

        buffer[index++] = (char)('0' + (value / 10U));
        buffer[index++] = (char)('0' + (value % 10U));
    }
    else if (value >= 10U)
    {
        buffer[index++] = (char)('0' + (value / 10U));
        buffer[index++] = (char)('0' + (value % 10U));
    }
    else
    {
        buffer[index++] = (char)('0' + value);
    }

    buffer[index] = '\0';

    MCU_Debug_SendString(buffer);
}

/* -------------------------------------------------------------------------- */

static void MCU_Debug_SendUInt16(uint16_t value)
{
    char buffer[6];
    uint8_t index = 0U;

    if (value >= 10000U)
    {
        buffer[index++] = (char)('0' + ((value / 10000U) % 10U));
        buffer[index++] = (char)('0' + ((value / 1000U) % 10U));
        buffer[index++] = (char)('0' + ((value / 100U) % 10U));
        buffer[index++] = (char)('0' + ((value / 10U) % 10U));
        buffer[index++] = (char)('0' + (value % 10U));
    }
    else if (value >= 1000U)
    {
        buffer[index++] = (char)('0' + ((value / 1000U) % 10U));
        buffer[index++] = (char)('0' + ((value / 100U) % 10U));
        buffer[index++] = (char)('0' + ((value / 10U) % 10U));
        buffer[index++] = (char)('0' + (value % 10U));
    }
    else if (value >= 100U)
    {
        buffer[index++] = (char)('0' + ((value / 100U) % 10U));
        buffer[index++] = (char)('0' + ((value / 10U) % 10U));
        buffer[index++] = (char)('0' + (value % 10U));
    }
    else if (value >= 10U)
    {
        buffer[index++] = (char)('0' + ((value / 10U) % 10U));
        buffer[index++] = (char)('0' + (value % 10U));
    }
    else
    {
        buffer[index++] = (char)('0' + value);
    }

    buffer[index] = '\0';

    MCU_Debug_SendString(buffer);
}

/* -------------------------------------------------------------------------- */

static void MCU_Debug_SendStateName(uint8_t state)
{
    switch (state)
    {
        case MCU_STATE_INIT:
            MCU_Debug_SendString("INIT");
            break;

        case MCU_STATE_DISABLED:
            MCU_Debug_SendString("DISABLED");
            break;

        case MCU_STATE_READY:
            MCU_Debug_SendString("READY");
            break;

        case MCU_STATE_RUN:
            MCU_Debug_SendString("RUN");
            break;

        case MCU_STATE_BRAKE:
            MCU_Debug_SendString("BRAKE");
            break;

        case MCU_STATE_WARNING:
            MCU_Debug_SendString("WARNING");
            break;

        case MCU_STATE_FAULT:
            MCU_Debug_SendString("FAULT");
            break;

        case MCU_STATE_SHUTDOWN:
            MCU_Debug_SendString("SHUTDOWN");
            break;

        default:
            MCU_Debug_SendString("UNKNOWN");
            break;
    }
}

/* ============================================================================
 * Initialization
 * ========================================================================== */

void MCU_Debug_Init(void)
{
    mcu_debug_initialized = MCU_TRUE;
    mcu_debug_update_requested = MCU_TRUE;
    mcu_debug_last_update_time = HAL_GetTick();
}

/* ============================================================================
 * Periodic update
 * ========================================================================== */

void MCU_Debug_Update(void)
{
    uint32_t now;

    if (!mcu_debug_initialized)
    {
        return;
    }

    now = HAL_GetTick();

    if (!mcu_debug_update_requested)
    {
        if ((now - mcu_debug_last_update_time) < MCU_DEBUG_PERIOD_MS)
        {
            return;
        }
    }

    mcu_debug_last_update_time = now;
    mcu_debug_update_requested = MCU_FALSE;

    MCU_Debug_PrintSystemStatus();
    MCU_Debug_PrintMotorStatus();
    MCU_Debug_PrintCANStatus();
    MCU_Debug_PrintFaultStatus();
    MCU_Debug_PrintADCStatus();
    MCU_Debug_PrintHallStatus();

    MCU_Debug_SendString("\r\n");
}

/* ============================================================================
 * Send character
 * ========================================================================== */

void MCU_Debug_SendChar(char character)
{
    if (!mcu_debug_initialized)
    {
        return;
    }

    (void)HAL_UART_Transmit(
        &huart1,
        (uint8_t *)&character,
        1U,
        100U);
}

/* ============================================================================
 * Send string
 * ========================================================================== */

void MCU_Debug_SendString(const char *text)
{
    uint16_t length = 0U;

    if (!mcu_debug_initialized || (text == NULL))
    {
        return;
    }

    while ((text[length] != '\0') && (length < 255U))
    {
        length++;
    }

    if (length == 0U)
    {
        return;
    }

    (void)HAL_UART_Transmit(
        &huart1,
        (uint8_t *)text,
        length,
        100U);
}

/* ============================================================================
 * Send unsigned 32-bit integer
 * ========================================================================== */

void MCU_Debug_SendUInt32(uint32_t value)
{
    char buffer[11];
    uint8_t index = 0U;
    uint8_t i;

    if (!mcu_debug_initialized)
    {
        return;
    }

    if (value == 0U)
    {
        MCU_Debug_SendChar('0');
        return;
    }

    while ((value > 0U) && (index < 10U))
    {
        buffer[index++] =
            (char)('0' + (value % 10U));

        value /= 10U;
    }

    for (i = index; i > 0U; i--)
    {
        MCU_Debug_SendChar(buffer[i - 1U]);
    }
}

/* ============================================================================
 * Send signed 32-bit integer
 * ========================================================================== */

void MCU_Debug_SendInt32(int32_t value)
{
    uint32_t magnitude;

    if (value < 0)
    {
        MCU_Debug_SendChar('-');

        magnitude =
            (uint32_t)(-(value + 1)) + 1U;
    }
    else
    {
        magnitude = (uint32_t)value;
    }

    MCU_Debug_SendUInt32(magnitude);
}

/* ============================================================================
 * Print system status
 * ========================================================================== */

void MCU_Debug_PrintSystemStatus(void)
{
    MCU_Debug_SendString("[SYSTEM] State=");

    MCU_Debug_SendStateName(
        MCU_Data.motor.state);

    MCU_Debug_SendString(" Healthy=");

    MCU_Debug_SendUInt8(
        MCU_Data.system_healthy);

    MCU_Debug_SendString("\r\n");
}

/* ============================================================================
 * Print motor status
 * ========================================================================== */

void MCU_Debug_PrintMotorStatus(void)
{
    MCU_Debug_SendString("[MOTOR] Enable=");

    MCU_Debug_SendUInt8(
        MCU_Data.motor.enabled);

    MCU_Debug_SendString(" Dir=");

    if (MCU_Data.motor.direction == MCU_DIRECTION_FORWARD)
    {
        MCU_Debug_SendString("FWD");
    }
    else if (MCU_Data.motor.direction == MCU_DIRECTION_REVERSE)
    {
        MCU_Debug_SendString("REV");
    }
    else
    {
        MCU_Debug_SendString("N");
    }

    MCU_Debug_SendString(" PWM=");

    MCU_Debug_SendUInt8(
        MCU_Data.motor.pwm_percent);

    MCU_Debug_SendString("% RPM=");

    MCU_Debug_SendUInt32(
        MCU_Data.hall.rpm);

    MCU_Debug_SendString("\r\n");
}

/* ============================================================================
 * Print CAN status
 * ========================================================================== */

void MCU_Debug_PrintCANStatus(void)
{
    MCU_Debug_SendString("[CAN] VCU=");

    MCU_Debug_SendUInt8(
        MCU_Data.vcu_comm.communication_valid);

    MCU_Debug_SendString(" Cmd=");

    MCU_Debug_SendUInt8(
        MCU_Data.vcu_comm.command_received);

    MCU_Debug_SendString(" Counter=");

    MCU_Debug_SendUInt8(
        MCU_Data.vcu_comm.last_command_counter);

    MCU_Debug_SendString(" TimeoutCount=");

    MCU_Debug_SendUInt32(
        MCU_Data.vcu_comm.timeout_count);

    MCU_Debug_SendString("\r\n");
}

/* ============================================================================
 * Print fault status
 * ========================================================================== */

void MCU_Debug_PrintFaultStatus(void)
{
    MCU_Debug_SendString("[FAULT] Active=");

    MCU_Debug_SendUInt8(
        MCU_Data.fault.active);

    MCU_Debug_SendString(" Latched=");

    MCU_Debug_SendUInt8(
        MCU_Data.fault.latched);

    MCU_Debug_SendString(" Code=");

    MCU_Debug_SendUInt8(
        MCU_Data.fault.code);

    MCU_Debug_SendString(" Severity=");

    MCU_Debug_SendUInt8(
        MCU_Data.fault.severity);

    MCU_Debug_SendString("\r\n");
}

/* ============================================================================
 * Print ADC status
 * ========================================================================== */

void MCU_Debug_PrintADCStatus(void)
{
    uint32_t voltage_mV;

    /*
     * Convert stored floating-point voltage to millivolts for display.
     * No %f / printf is used.
     */
    if (MCU_Data.adc.pot_voltage <= 0.0f)
    {
        voltage_mV = 0U;
    }
    else
    {
        voltage_mV =
            (uint32_t)(MCU_Data.adc.pot_voltage * 1000.0f);
    }

    MCU_Debug_SendString("[ADC] Raw=");

    MCU_Debug_SendUInt16(
        MCU_Data.adc.pot_raw);

    MCU_Debug_SendString(" Voltage_mV=");

    MCU_Debug_SendUInt32(
        voltage_mV);

    MCU_Debug_SendString(" Percent=");

    MCU_Debug_SendUInt8(
        MCU_Data.adc.pot_percent);

    MCU_Debug_SendString("% Valid=");

    MCU_Debug_SendUInt8(
        MCU_Data.adc.valid);

    MCU_Debug_SendString("\r\n");
}

/* ============================================================================
 * Print Hall status
 * ========================================================================== */

void MCU_Debug_PrintHallStatus(void)
{
    MCU_Debug_SendString("[HALL] RPM=");

    MCU_Debug_SendUInt32(
        MCU_Data.hall.rpm);

    MCU_Debug_SendString(" Period_us=");

    MCU_Debug_SendUInt32(
        MCU_Data.hall.pulse_period_us);

    MCU_Debug_SendString(" Pulses=");

    MCU_Debug_SendUInt32(
        MCU_Data.hall.pulse_count);

    MCU_Debug_SendString(" Valid=");

    MCU_Debug_SendUInt8(
        MCU_Data.hall.signal_valid);

    MCU_Debug_SendString(" Timeout=");

    MCU_Debug_SendUInt8(
        MCU_Data.hall.timeout);

    MCU_Debug_SendString("\r\n");
}

/* ============================================================================
 * Request debug update
 * ========================================================================== */

void MCU_Debug_RequestUpdate(void)
{
    mcu_debug_update_requested = MCU_TRUE;
}

/* ============================================================================
 * Debug ready status
 * ========================================================================== */

uint8_t MCU_Debug_IsReady(void)
{
    return mcu_debug_initialized;
}
