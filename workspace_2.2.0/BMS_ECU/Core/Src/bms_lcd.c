/*
 * bms_lcd.c
 *
 * BMS 16x2 I2C LCD driver.
 *
 * Designed for common PCF8574-based 16x2 LCD backpacks.
 *
 * I2C:
 *   I2C1
 *
 * LCD address:
 *   Defined in bms_config.h
 *
 * IMPORTANT:
 *   - No floating-point printf formatting is used.
 *   - No snprintf() is used for measurement display.
 *   - LCD formatting is performed using fixed-point helper functions.
 *
 * NOTE:
 *   The PCF8574 pin mapping and LCD address must match the actual
 *   hardware backpack.
 */

#include "main.h"
#include "i2c.h"

#include "bms_lcd.h"
#include "bms_data.h"
#include "bms_config.h"
#include "bms_faults.h"

#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* External I2C handle                                                        */
/* -------------------------------------------------------------------------- */

extern I2C_HandleTypeDef hi2c1;

/* -------------------------------------------------------------------------- */
/* PCF8574 LCD bit definitions                                                */
/* -------------------------------------------------------------------------- */

#define LCD_RS              0x01U
#define LCD_RW              0x02U
#define LCD_EN              0x04U
#define LCD_BACKLIGHT       0x08U

/* -------------------------------------------------------------------------- */
/* LCD configuration                                                          */
/* -------------------------------------------------------------------------- */

#define LCD_COLUMNS         16U
#define LCD_ROWS             2U

/* -------------------------------------------------------------------------- */
/* Private variables                                                          */
/* -------------------------------------------------------------------------- */

static uint8_t lcd_backpack_state = LCD_BACKLIGHT;
static uint8_t lcd_page = 0U;
static uint32_t lcd_page_time = 0U;
static uint8_t lcd_initialized = 0U;

/* -------------------------------------------------------------------------- */
/* Private low-level functions                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief Write one byte to the PCF8574 backpack.
 */
static HAL_StatusTypeDef BMS_LCD_WriteExpander(uint8_t value)
{
    return HAL_I2C_Master_Transmit(
        &hi2c1,
        BMS_LCD_I2C_ADDRESS,
        &value,
        1U,
        100U);
}

/**
 * @brief Generate LCD enable pulse.
 */
static HAL_StatusTypeDef BMS_LCD_PulseEnable(uint8_t data)
{
    HAL_StatusTypeDef status;

    status =
        BMS_LCD_WriteExpander(
            (uint8_t)(data | LCD_EN));

    if (status != HAL_OK)
    {
        return status;
    }

    HAL_Delay(1U);

    status =
        BMS_LCD_WriteExpander(
            (uint8_t)(data & (uint8_t)(~LCD_EN)));

    return status;
}

/**
 * @brief Write one 4-bit nibble to LCD.
 */
static HAL_StatusTypeDef BMS_LCD_WriteNibble(
    uint8_t nibble,
    uint8_t rs)
{
    uint8_t data;

    /*
     * PCF8574 P4-P7 -> LCD D4-D7.
     */
    data =
        (uint8_t)((nibble & 0x0FU) << 4);

    /*
     * Preserve backlight state.
     */
    data |=
        (uint8_t)(lcd_backpack_state & LCD_BACKLIGHT);

    /*
     * RS selection.
     */
    if (rs != 0U)
    {
        data |= LCD_RS;
    }
    else
    {
        data &= (uint8_t)(~LCD_RS);
    }

    /*
     * RW is always LOW for this driver.
     */
    data &= (uint8_t)(~LCD_RW);

    return BMS_LCD_PulseEnable(data);
}

/**
 * @brief Send complete command byte.
 */
static HAL_StatusTypeDef BMS_LCD_SendCommand(uint8_t command)
{
    HAL_StatusTypeDef status;

    status =
        BMS_LCD_WriteNibble(
            (uint8_t)(command >> 4),
            0U);

    if (status != HAL_OK)
    {
        return status;
    }

    status =
        BMS_LCD_WriteNibble(
            (uint8_t)(command & 0x0FU),
            0U);

    if (status != HAL_OK)
    {
        return status;
    }

    /*
     * Clear and home commands need extra execution time.
     */
    if ((command == 0x01U) ||
        (command == 0x02U))
    {
        HAL_Delay(2U);
    }

    return HAL_OK;
}

/**
 * @brief Send one character to LCD.
 */
static HAL_StatusTypeDef BMS_LCD_SendData(uint8_t data)
{
    HAL_StatusTypeDef status;

    status =
        BMS_LCD_WriteNibble(
            (uint8_t)(data >> 4),
            1U);

    if (status != HAL_OK)
    {
        return status;
    }

    status =
        BMS_LCD_WriteNibble(
            (uint8_t)(data & 0x0FU),
            1U);

    return status;
}

/**
 * @brief Print up to 16 characters.
 */
static void BMS_LCD_PrintFixed(const char *text)
{
    uint8_t count = 0U;

    if ((lcd_initialized == 0U) ||
        (text == NULL))
    {
        return;
    }

    while ((*text != '\0') &&
           (count < LCD_COLUMNS))
    {
        if (BMS_LCD_SendData(
                (uint8_t)*text) != HAL_OK)
        {
            /*
             * Stop immediately on I2C/LCD communication failure.
             */
            lcd_initialized = 0U;
            return;
        }

        text++;
        count++;
    }
}

/**
 * @brief Print a signed integer with leading spaces.
 */
static void BMS_LCD_PrintSigned(
    int32_t value,
    uint8_t width)
{
    char digits[12];
    uint8_t count = 0U;
    uint8_t i;
    uint8_t negative = 0U;
    uint32_t magnitude;

    if (lcd_initialized == 0U)
    {
        return;
    }

    if (value < 0)
    {
        negative = 1U;

        /*
         * Avoid overflow for INT32_MIN.
         */
        magnitude =
            (uint32_t)(-(value + 1)) + 1U;
    }
    else
    {
        magnitude =
            (uint32_t)value;
    }

    /*
     * Convert decimal number to reverse digits.
     */
    do
    {
        digits[count] =
            (char)('0' + (magnitude % 10U));

        magnitude /= 10U;
        count++;

    } while ((magnitude != 0U) &&
             (count < sizeof(digits)));

    /*
     * Leading spaces.
     */
    if (negative != 0U)
    {
        if (width > (uint8_t)(count + 1U))
        {
            for (i = 0U;
                 i < (uint8_t)(width - count - 1U);
                 i++)
            {
                (void)BMS_LCD_SendData(' ');
            }
        }

        (void)BMS_LCD_SendData('-');
    }
    else
    {
        if (width > count)
        {
            for (i = 0U;
                 i < (uint8_t)(width - count);
                 i++)
            {
                (void)BMS_LCD_SendData(' ');
            }
        }
    }

    /*
     * Print digits in normal order.
     */
    while (count > 0U)
    {
        count--;

        (void)BMS_LCD_SendData(
            (uint8_t)digits[count]);
    }
}

/**
 * @brief Print unsigned integer with leading spaces.
 */
static void BMS_LCD_PrintUnsigned(
    uint32_t value,
    uint8_t width)
{
    char digits[11];
    uint8_t count = 0U;
    uint8_t i;

    if (lcd_initialized == 0U)
    {
        return;
    }

    do
    {
        digits[count] =
            (char)('0' + (value % 10U));

        value /= 10U;
        count++;

    } while ((value != 0U) &&
             (count < sizeof(digits)));

    if (width > count)
    {
        for (i = 0U;
             i < (uint8_t)(width - count);
             i++)
        {
            (void)BMS_LCD_SendData(' ');
        }
    }

    while (count > 0U)
    {
        count--;

        (void)BMS_LCD_SendData(
            (uint8_t)digits[count]);
    }
}

/**
 * @brief Print fixed-point value with one decimal digit.
 *
 * value10 represents value * 10.
 *
 * Example:
 *   485 -> "48.5"
 *   -25 -> "-2.5"
 */
static void BMS_LCD_PrintFixed1(
    int32_t value10,
    uint8_t integer_width)
{
    int32_t integer_part;
    uint8_t fraction;

    if (lcd_initialized == 0U)
    {
        return;
    }

    integer_part =
        value10 / 10;

    /*
     * Handle negative values correctly.
     */
    if (value10 < 0)
    {
        fraction =
            (uint8_t)(-(value10 % 10));
    }
    else
    {
        fraction =
            (uint8_t)(value10 % 10);
    }

    /*
     * Integer portion.
     */
    BMS_LCD_PrintSigned(
        integer_part,
        integer_width);

    /*
     * Decimal point.
     */
    (void)BMS_LCD_SendData('.');

    /*
     * Fraction.
     */
    (void)BMS_LCD_SendData(
        (uint8_t)('0' + fraction));
}

/* -------------------------------------------------------------------------- */
/* Public basic functions                                                     */
/* -------------------------------------------------------------------------- */

/**
 * @brief Clear LCD.
 */
void BMS_LCD_Clear(void)
{
    if (lcd_initialized == 0U)
    {
        return;
    }

    if (BMS_LCD_SendCommand(0x01U) != HAL_OK)
    {
        lcd_initialized = 0U;
        return;
    }

    BMS_LCD_SetCursor(0U, 0U);
}

/**
 * @brief Set LCD cursor position.
 */
void BMS_LCD_SetCursor(
    uint8_t row,
    uint8_t column)
{
    uint8_t address;

    if (lcd_initialized == 0U)
    {
        return;
    }

    if (row >= LCD_ROWS)
    {
        row = LCD_ROWS - 1U;
    }

    if (column >= LCD_COLUMNS)
    {
        column = LCD_COLUMNS - 1U;
    }

    if (row == 0U)
    {
        address = column;
    }
    else
    {
        address =
            (uint8_t)(0x40U + column);
    }

    if (BMS_LCD_SendCommand(
            (uint8_t)(0x80U | address)) != HAL_OK)
    {
        lcd_initialized = 0U;
    }
}

/**
 * @brief Print text to LCD.
 */
void BMS_LCD_Print(const char *text)
{
    BMS_LCD_PrintFixed(text);
}

/* -------------------------------------------------------------------------- */
/* LCD initialization                                                         */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize 16x2 I2C LCD.
 */
void BMS_LCD_Init(void)
{
    HAL_StatusTypeDef status;

    lcd_initialized = 0U;

    /*
     * LCD power-up delay.
     */
    HAL_Delay(50U);

    /*
     * Backlight ON.
     */
    lcd_backpack_state =
        LCD_BACKLIGHT;

    status =
        BMS_LCD_WriteExpander(
            lcd_backpack_state);

    if (status != HAL_OK)
    {
        return;
    }

    HAL_Delay(10U);

    /*
     * HD44780 4-bit startup sequence.
     */
    status =
        BMS_LCD_WriteNibble(
            0x03U,
            0U);

    if (status != HAL_OK)
    {
        return;
    }

    HAL_Delay(5U);

    status =
        BMS_LCD_WriteNibble(
            0x03U,
            0U);

    if (status != HAL_OK)
    {
        return;
    }

    HAL_Delay(5U);

    status =
        BMS_LCD_WriteNibble(
            0x03U,
            0U);

    if (status != HAL_OK)
    {
        return;
    }

    HAL_Delay(1U);

    status =
        BMS_LCD_WriteNibble(
            0x02U,
            0U);

    if (status != HAL_OK)
    {
        return;
    }

    /*
     * 4-bit / 2-line / 5x8 font.
     */
    if (BMS_LCD_SendCommand(0x28U) != HAL_OK)
    {
        return;
    }

    /*
     * Display ON / cursor OFF / blink OFF.
     */
    if (BMS_LCD_SendCommand(0x0CU) != HAL_OK)
    {
        return;
    }

    /*
     * Entry mode.
     */
    if (BMS_LCD_SendCommand(0x06U) != HAL_OK)
    {
        return;
    }

    /*
     * Clear display.
     */
    if (BMS_LCD_SendCommand(0x01U) != HAL_OK)
    {
        return;
    }

    lcd_initialized = 1U;

    lcd_page = 0U;

    lcd_page_time = HAL_GetTick();

    /*
     * Startup screen.
     */
    BMS_LCD_SetCursor(0U, 0U);

    BMS_LCD_Print("BMS ECU");

    BMS_LCD_SetCursor(1U, 0U);

    BMS_LCD_Print("INITIALIZING");

    HAL_Delay(1000U);

    BMS_LCD_Clear();
}

/* -------------------------------------------------------------------------- */
/* Display helpers                                                            */
/* -------------------------------------------------------------------------- */

/**
 * @brief Clear one LCD line.
 */
static void BMS_LCD_ClearLine(uint8_t row)
{
    uint8_t i;

    BMS_LCD_SetCursor(
        row,
        0U);

    if (lcd_initialized == 0U)
    {
        return;
    }

    for (i = 0U;
         i < LCD_COLUMNS;
         i++)
    {
        if (BMS_LCD_SendData(' ') != HAL_OK)
        {
            lcd_initialized = 0U;
            return;
        }
    }

    BMS_LCD_SetCursor(
        row,
        0U);
}

/**
 * @brief Display voltage/current/temperature/SOC.
 *
 * Line 1:
 *   V:48.5 I:12.3
 *
 * Line 2:
 *   T:32.0C S: 61%
 */
static void BMS_LCD_DisplayMeasurements(void)
{
    int32_t voltage10;
    int32_t current10;
    int32_t temperature10;
    uint32_t soc;

    voltage10 =
        (int32_t)(
            BMS_Data.battery_voltage *
            10.0f);

    current10 =
        (int32_t)(
            BMS_Data.battery_current *
            10.0f);

    temperature10 =
        (int32_t)(
            BMS_Data.battery_temperature *
            10.0f);

    /*
     * Clamp SOC before displaying.
     */
    if (BMS_Data.state_of_charge <= 0.0f)
    {
        soc = 0U;
    }
    else if (BMS_Data.state_of_charge >= 100.0f)
    {
        soc = 100U;
    }
    else
    {
        soc =
            (uint32_t)BMS_Data.state_of_charge;
    }

    /*
     * Line 1.
     */
    BMS_LCD_ClearLine(0U);

    if (lcd_initialized == 0U)
    {
        return;
    }

    BMS_LCD_Print("V:");

    BMS_LCD_PrintFixed1(
        voltage10,
        4U);

    BMS_LCD_Print(" I:");

    BMS_LCD_PrintFixed1(
        current10,
        3U);

    /*
     * Line 2.
     */
    BMS_LCD_ClearLine(1U);

    if (lcd_initialized == 0U)
    {
        return;
    }

    BMS_LCD_Print("T:");

    BMS_LCD_PrintFixed1(
        temperature10,
        3U);

    BMS_LCD_Print("C S:");

    BMS_LCD_PrintUnsigned(
        soc,
        3U);

    BMS_LCD_Print("%");
}

/**
 * @brief Display SOC/SOH and temperature.
 *
 * Line 1:
 *   SOC: 61 SOH:100
 *
 * Line 2:
 *   TEMP: 32.0 C
 */
static void BMS_LCD_DisplayBatteryStatus(void)
{
    uint32_t soc;
    uint32_t soh;
    int32_t temperature10;

    /*
     * Clamp SOC.
     */
    if (BMS_Data.state_of_charge <= 0.0f)
    {
        soc = 0U;
    }
    else if (BMS_Data.state_of_charge >= 100.0f)
    {
        soc = 100U;
    }
    else
    {
        soc =
            (uint32_t)BMS_Data.state_of_charge;
    }

    /*
     * Clamp SOH.
     */
    if (BMS_Data.state_of_health <= 0.0f)
    {
        soh = 0U;
    }
    else if (BMS_Data.state_of_health >= 100.0f)
    {
        soh = 100U;
    }
    else
    {
        soh =
            (uint32_t)BMS_Data.state_of_health;
    }

    temperature10 =
        (int32_t)(
            BMS_Data.battery_temperature *
            10.0f);

    /*
     * Line 1.
     */
    BMS_LCD_ClearLine(0U);

    if (lcd_initialized == 0U)
    {
        return;
    }

    BMS_LCD_Print("SOC:");

    BMS_LCD_PrintUnsigned(
        soc,
        3U);

    BMS_LCD_Print(" SOH:");

    BMS_LCD_PrintUnsigned(
        soh,
        3U);

    /*
     * Line 2.
     */
    BMS_LCD_ClearLine(1U);

    if (lcd_initialized == 0U)
    {
        return;
    }

    BMS_LCD_Print("TEMP:");

    BMS_LCD_PrintFixed1(
        temperature10,
        3U);

    BMS_LCD_Print(" C");
}

/**
 * @brief Display BMS system state.
 */
static void BMS_LCD_DisplayState(void)
{
    const char *state_text;

    switch (BMS_Data.system_state)
    {
        case BMS_SYSTEM_INIT:
            state_text = "INIT";
            break;

        case BMS_SYSTEM_NORMAL:
            state_text = "NORMAL";
            break;

        case BMS_SYSTEM_WARNING:
            state_text = "DISABLED";
            break;

        case BMS_SYSTEM_FAULT:
            state_text = "FAULT";
            break;

        case BMS_SYSTEM_SHUTDOWN:
            state_text = "SHUTDOWN";
            break;

        default:
            state_text = "UNKNOWN";
            break;
    }

    BMS_LCD_ClearLine(0U);

    if (lcd_initialized == 0U)
    {
        return;
    }

    BMS_LCD_Print("SYSTEM STATUS");

    BMS_LCD_ClearLine(1U);

    if (lcd_initialized == 0U)
    {
        return;
    }

    BMS_LCD_Print(state_text);
}

/**
 * @brief Display highest-priority active fault.
 */
static void BMS_LCD_DisplayFault(void)
{
    BMS_LCD_ClearLine(0U);

    if (lcd_initialized == 0U)
    {
        return;
    }

    BMS_LCD_Print("!! BMS FAULT !!");

    BMS_LCD_ClearLine(1U);

    if (lcd_initialized == 0U)
    {
        return;
    }

    /*
     * Display the highest-priority active fault.
     */
    if ((BMS_Data.fault_status &
         BMS_FAULT_OVERVOLTAGE) != 0U)
    {
        BMS_LCD_Print("OVER VOLTAGE");
    }
    else if ((BMS_Data.fault_status &
              BMS_FAULT_UNDERVOLTAGE) != 0U)
    {
        BMS_LCD_Print("UNDER VOLTAGE");
    }
    else if ((BMS_Data.fault_status &
              BMS_FAULT_OVERCURRENT) != 0U)
    {
        BMS_LCD_Print("OVER CURRENT");
    }
    else if ((BMS_Data.fault_status &
              BMS_FAULT_OVERTEMPERATURE) != 0U)
    {
        BMS_LCD_Print("OVER TEMP");
    }
    else if ((BMS_Data.fault_status &
              BMS_FAULT_UNDERTEMPERATURE) != 0U)
    {
        BMS_LCD_Print("UNDER TEMP");
    }
    else if ((BMS_Data.fault_status &
              BMS_FAULT_VOLTAGE_SENSOR) != 0U)
    {
        BMS_LCD_Print("VOLT SENSOR");
    }
    else if ((BMS_Data.fault_status &
              BMS_FAULT_CURRENT_SENSOR) != 0U)
    {
        BMS_LCD_Print("CURR SENSOR");
    }
    else if ((BMS_Data.fault_status &
              BMS_FAULT_TEMPERATURE_SENSOR) != 0U)
    {
        BMS_LCD_Print("TEMP SENSOR");
    }
    else
    {
        BMS_LCD_Print("FAULT UNKNOWN");
    }
}

/* -------------------------------------------------------------------------- */
/* Main LCD update                                                             */
/* -------------------------------------------------------------------------- */

/**
 * @brief Update LCD display.
 *
 * Faults always have priority.
 *
 * Normal pages:
 *   0 -> Voltage/current + temperature/SOC
 *   1 -> SOC/SOH + temperature
 *   2 -> System state
 */
void BMS_LCD_Update(void)
{
    uint32_t current_tick;

    if (lcd_initialized == 0U)
    {
        return;
    }

    current_tick = HAL_GetTick();

    /*
     * Fault screen always has highest priority.
     */
    if ((BMS_Data.fault_status != 0U) ||
        (BMS_Data.fault_latched != 0U))
    {
        BMS_LCD_DisplayFault();

        return;
    }

    /*
     * Page timing.
     */
    if ((uint32_t)(
            current_tick -
            lcd_page_time) >=
        BMS_LCD_UPDATE_PERIOD_MS)
    {
        lcd_page_time = current_tick;

        lcd_page++;

        if (lcd_page > 2U)
        {
            lcd_page = 0U;
        }
    }

    /*
     * Display selected page.
     */
    switch (lcd_page)
    {
        case 0U:

            BMS_LCD_DisplayMeasurements();

            break;

        case 1U:

            BMS_LCD_DisplayBatteryStatus();

            break;

        case 2U:

            BMS_LCD_DisplayState();

            break;

        default:

            lcd_page = 0U;

            break;
    }
}
