/*
 * mcu_lcd.c
 *
 *  Created on: 21-Sept-2026
 *      Author: MohithK
 */


#include "main.h"
#include "i2c.h"
#include "mcu_config.h"
#include "mcu_data.h"
#include "mcu_lcd.h"

/* ============================================================================
 * MCU LCD Driver
 * ============================================================================
 *
 * Hardware:
 *
 * PB6 -> I2C1 SCL
 * PB7 -> I2C1 SDA
 *
 * Assumed common PCF8574 16x2 backpack mapping:
 *
 * P0 -> RS
 * P1 -> RW
 * P2 -> EN
 * P3 -> Backlight
 * P4 -> D4
 * P5 -> D5
 * P6 -> D6
 * P7 -> D7
 *
 * LCD address is taken from mcu_config.h.
 *
 * ========================================================================== */

/* ============================================================================
 * Private definitions
 * ========================================================================== */

#define MCU_LCD_RS             0x01U
#define MCU_LCD_RW             0x02U
#define MCU_LCD_EN             0x04U
#define MCU_LCD_BACKLIGHT      0x08U

#define MCU_LCD_CMD_CLEAR      0x01U
#define MCU_LCD_CMD_HOME       0x02U
#define MCU_LCD_CMD_FUNCTION   0x28U
#define MCU_LCD_CMD_DISPLAY    0x0CU
#define MCU_LCD_CMD_ENTRY      0x06U

#define MCU_LCD_LINE1          0x80U
#define MCU_LCD_LINE2          0xC0U

/* ============================================================================
 * Private variables
 * ========================================================================== */

static uint8_t mcu_lcd_initialized = MCU_FALSE;
static uint8_t mcu_lcd_backlight = MCU_LCD_BACKLIGHT;

/* ============================================================================
 * Private delay
 * ========================================================================== */

static void MCU_LCD_DelayShort(void)
{
    /*
     * The HD44780 needs a short enable pulse.
     */
    for (volatile uint32_t i = 0U; i < 80U; i++)
    {
        __NOP();
    }
}

/* ============================================================================
 * Low-level write
 * ========================================================================== */

static HAL_StatusTypeDef MCU_LCD_WriteByte(uint8_t value)
{
    return HAL_I2C_Master_Transmit(
        &hi2c1,
        MCU_LCD_I2C_ADDRESS,
        &value,
        1U,
        100U);
}

/* ============================================================================
 * Enable pulse
 * ========================================================================== */

static void MCU_LCD_EnablePulse(uint8_t data)
{
    uint8_t value;

    value = data | MCU_LCD_EN | mcu_lcd_backlight;

    (void)MCU_LCD_WriteByte(value);

    MCU_LCD_DelayShort();

    value = data & (uint8_t)(~MCU_LCD_EN);
    value |= mcu_lcd_backlight;

    (void)MCU_LCD_WriteByte(value);

    MCU_LCD_DelayShort();
}

/* ============================================================================
 * Send 4 bits
 * ========================================================================== */

static void MCU_LCD_SendNibble(uint8_t nibble,
                               uint8_t rs)
{
    uint8_t data;

    data = (uint8_t)((nibble & 0xF0U));

    if (rs)
    {
        data |= MCU_LCD_RS;
    }

    data |= mcu_lcd_backlight;

    MCU_LCD_EnablePulse(data);
}

/* ============================================================================
 * Send command
 * ========================================================================== */

static void MCU_LCD_SendCommand(uint8_t command)
{
    MCU_LCD_SendNibble(command & 0xF0U, MCU_FALSE);
    MCU_LCD_SendNibble((uint8_t)((command << 4) & 0xF0U), MCU_FALSE);

    HAL_Delay(2U);
}

/* ============================================================================
 * Send data
 * ========================================================================== */

static void MCU_LCD_SendData(uint8_t data)
{
    MCU_LCD_SendNibble(data & 0xF0U, MCU_TRUE);
    MCU_LCD_SendNibble((uint8_t)((data << 4) & 0xF0U), MCU_TRUE);
}

/* ============================================================================
 * Set cursor
 * ========================================================================== */

static void MCU_LCD_SetCursor(uint8_t row,
                              uint8_t column)
{
    uint8_t address;

    if (column > 15U)
    {
        column = 15U;
    }

    if (row == 0U)
    {
        address = MCU_LCD_LINE1 + column;
    }
    else
    {
        address = MCU_LCD_LINE2 + column;
    }

    MCU_LCD_SendCommand(address);
}

/* ============================================================================
 * Write string
 * ========================================================================== */

static void MCU_LCD_WriteString(const char *text)
{
    if (text == NULL)
    {
        return;
    }

    while (*text != '\0')
    {
        MCU_LCD_SendData((uint8_t)*text);
        text++;
    }
}

/* ============================================================================
 * Write exactly 16 characters
 * ========================================================================== */

static void MCU_LCD_WriteLine(const char *text)
{
    uint8_t count = 0U;

    if (text == NULL)
    {
        return;
    }

    while ((*text != '\0') && (count < 16U))
    {
        MCU_LCD_SendData((uint8_t)*text);
        text++;
        count++;
    }

    while (count < 16U)
    {
        MCU_LCD_SendData((uint8_t)' ');
        count++;
    }
}

/* ============================================================================
 * Convert decimal value to two digits
 * ========================================================================== */

static void MCU_LCD_Write2Digits(uint8_t value)
{
    uint8_t tens;
    uint8_t units;

    if (value > 99U)
    {
        value = 99U;
    }

    tens = value / 10U;
    units = value % 10U;

    MCU_LCD_SendData((uint8_t)('0' + tens));
    MCU_LCD_SendData((uint8_t)('0' + units));
}

/* ============================================================================
 * Convert decimal value to three digits
 * ========================================================================== */

static void MCU_LCD_Write3Digits(uint16_t value)
{
    uint8_t hundreds;
    uint8_t tens;
    uint8_t units;

    if (value > 999U)
    {
        value = 999U;
    }

    hundreds = (uint8_t)(value / 100U);
    tens = (uint8_t)((value / 10U) % 10U);
    units = (uint8_t)(value % 10U);

    MCU_LCD_SendData((uint8_t)('0' + hundreds));
    MCU_LCD_SendData((uint8_t)('0' + tens));
    MCU_LCD_SendData((uint8_t)('0' + units));
}

/* ============================================================================
 * State name
 * ========================================================================== */

static const char *MCU_LCD_GetStateName(uint8_t state)
{
    switch (state)
    {
        case MCU_STATE_INIT:
            return "INIT";

        case MCU_STATE_DISABLED:
            return "DISABLED";

        case MCU_STATE_READY:
            return "READY";

        case MCU_STATE_RUN:
            return "RUN";

        case MCU_STATE_BRAKE:
            return "BRAKE";

        case MCU_STATE_WARNING:
            return "WARNING";

        case MCU_STATE_FAULT:
            return "FAULT";

        case MCU_STATE_SHUTDOWN:
            return "SHUTDOWN";

        default:
            return "UNKNOWN";
    }
}

/* ============================================================================
 * Direction name
 * ========================================================================== */

static const char *MCU_LCD_GetDirectionName(uint8_t direction)
{
    switch (direction)
    {
        case MCU_DIRECTION_FORWARD:
            return "FWD";

        case MCU_DIRECTION_REVERSE:
            return "REV";

        default:
            return "N";
    }
}

/* ============================================================================
 * Initialization
 * ========================================================================== */

void MCU_LCD_Init(void)
{
    /*
     * Give the LCD backpack time to power up.
     */
    HAL_Delay(50U);

    /*
     * Standard HD44780 4-bit initialization sequence.
     */
    MCU_LCD_SendNibble(0x30U, MCU_FALSE);
    HAL_Delay(5U);

    MCU_LCD_SendNibble(0x30U, MCU_FALSE);
    HAL_Delay(1U);

    MCU_LCD_SendNibble(0x30U, MCU_FALSE);
    HAL_Delay(1U);

    MCU_LCD_SendNibble(0x20U, MCU_FALSE);
    HAL_Delay(1U);

    MCU_LCD_SendCommand(MCU_LCD_CMD_FUNCTION);
    MCU_LCD_SendCommand(MCU_LCD_CMD_DISPLAY);
    MCU_LCD_SendCommand(MCU_LCD_CMD_CLEAR);
    MCU_LCD_SendCommand(MCU_LCD_CMD_ENTRY);

    mcu_lcd_initialized = MCU_TRUE;

    MCU_Data.lcd.page = 0U;
    MCU_Data.lcd.update_required = MCU_TRUE;
    MCU_Data.lcd.last_update_time = HAL_GetTick();

    MCU_Data.task.lcd_ok = MCU_TRUE;
}

/* ============================================================================
 * Periodic LCD update
 * ========================================================================== */

void MCU_LCD_Update(void)
{
    uint32_t now;

    if (!mcu_lcd_initialized)
    {
        return;
    }

    now = HAL_GetTick();

    if (!MCU_Data.lcd.update_required)
    {
        if ((now - MCU_Data.lcd.last_update_time) <
            MCU_LCD_UPDATE_PERIOD_MS)
        {
            return;
        }
    }

    MCU_Data.lcd.last_update_time = now;
    MCU_Data.lcd.update_required = MCU_FALSE;

    /*
     * Page 0:
     *
     * STATE: READY
     * PWM:050 FWD
     */
    if (MCU_Data.lcd.page == 0U)
    {
        MCU_LCD_Clear();

        MCU_LCD_SetCursor(0U, 0U);
        MCU_LCD_WriteString("STATE: ");
        MCU_LCD_WriteString(
            MCU_LCD_GetStateName(
                MCU_Data.motor.state));

        MCU_LCD_SetCursor(1U, 0U);
        MCU_LCD_WriteString("PWM:");
        MCU_LCD_Write3Digits(
            MCU_Data.motor.pwm_percent);

        MCU_LCD_WriteString(" ");
        MCU_LCD_WriteString(
            MCU_LCD_GetDirectionName(
                MCU_Data.motor.direction));

        MCU_Data.lcd.page = 1U;
    }
    else
    {
        /*
         * Page 1:
         *
         * RPM:  123
         * FAULT:00
         */
        MCU_LCD_Clear();

        MCU_LCD_SetCursor(0U, 0U);
        MCU_LCD_WriteString("RPM:");
        MCU_LCD_Write3Digits(
            MCU_Data.hall.rpm);

        MCU_LCD_SetCursor(1U, 0U);
        MCU_LCD_WriteString("FAULT:");

        MCU_LCD_Write2Digits(
            MCU_Data.fault.code);

        MCU_LCD_WriteString(" ");

        if (MCU_Data.system_healthy)
        {
            MCU_LCD_WriteString("OK");
        }
        else
        {
            MCU_LCD_WriteString("CHECK");
        }

        MCU_Data.lcd.page = 0U;
    }
}

/* ============================================================================
 * LCD clear
 * ========================================================================== */

void MCU_LCD_Clear(void)
{
    if (!mcu_lcd_initialized)
    {
        return;
    }

    MCU_LCD_SendCommand(MCU_LCD_CMD_CLEAR);
    HAL_Delay(2U);
}

/* ============================================================================
 * Show state
 * ========================================================================== */

void MCU_LCD_ShowState(uint8_t state)
{
    if (!mcu_lcd_initialized)
    {
        return;
    }

    MCU_LCD_Clear();

    MCU_LCD_SetCursor(0U, 0U);
    MCU_LCD_WriteLine("MCU STATE:");

    MCU_LCD_SetCursor(1U, 0U);
    MCU_LCD_WriteLine(
        MCU_LCD_GetStateName(state));
}

/* ============================================================================
 * Show motor status
 * ========================================================================== */

void MCU_LCD_ShowMotor(uint8_t direction,
                       uint8_t pwm_percent,
                       uint16_t rpm)
{
    if (!mcu_lcd_initialized)
    {
        return;
    }

    MCU_LCD_Clear();

    MCU_LCD_SetCursor(0U, 0U);
    MCU_LCD_WriteString("M:");
    MCU_LCD_WriteString(
        MCU_LCD_GetDirectionName(direction));

    MCU_LCD_WriteString(" PWM:");
    MCU_LCD_Write3Digits(pwm_percent);

    MCU_LCD_SetCursor(1U, 0U);
    MCU_LCD_WriteString("RPM:");
    MCU_LCD_Write3Digits(rpm);
}

/* ============================================================================
 * Show fault
 * ========================================================================== */

void MCU_LCD_ShowFault(uint8_t fault_code)
{
    if (!mcu_lcd_initialized)
    {
        return;
    }

    MCU_LCD_Clear();

    MCU_LCD_SetCursor(0U, 0U);
    MCU_LCD_WriteLine("!! FAULT !!");

    MCU_LCD_SetCursor(1U, 0U);
    MCU_LCD_WriteString("CODE:");

    MCU_LCD_Write2Digits(fault_code);
}

/* ============================================================================
 * Show overall system status
 * ========================================================================== */

void MCU_LCD_ShowSystemStatus(uint8_t system_healthy)
{
    if (!mcu_lcd_initialized)
    {
        return;
    }

    MCU_LCD_Clear();

    MCU_LCD_SetCursor(0U, 0U);
    MCU_LCD_WriteLine("SYSTEM STATUS");

    MCU_LCD_SetCursor(1U, 0U);

    if (system_healthy)
    {
        MCU_LCD_WriteLine("HEALTHY");
    }
    else
    {
        MCU_LCD_WriteLine("NOT READY");
    }
}

/* ============================================================================
 * Request LCD update
 * ========================================================================== */

void MCU_LCD_RequestUpdate(void)
{
    MCU_Data.lcd.update_required = MCU_TRUE;
}

/* ============================================================================
 * LCD ready status
 * ========================================================================== */

uint8_t MCU_LCD_IsReady(void)
{
    return mcu_lcd_initialized;
}
