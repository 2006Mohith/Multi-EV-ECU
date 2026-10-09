/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    vcu_lcd.c
  * @brief   VCU 16x2 I2C LCD dashboard
  *
  * Dashboard pages:
  *   1. BMS data
  *   2. VCU data
  *   3. MCU data
  *   4. Overall system status
  *
  * Auto page rotation:
  *   Every 2 seconds
  ******************************************************************************
  */
/* USER CODE END Header */

#include "vcu_lcd.h"

#include <stdint.h>

#include "i2c.h"
#include "vcu_config.h"
#include "vcu_data.h"

/* ========================================================================= */
/* LCD CONFIGURATION                                                        */
/* ========================================================================= */

#define VCU_LCD_COLUMNS             16U
#define VCU_LCD_ROWS                2U

#define VCU_LCD_PAGE_TIME_MS        2000U

/* PCF8574 bit mapping */
#define LCD_RS                      0x01U
#define LCD_RW                      0x02U
#define LCD_EN                      0x04U
#define LCD_BACKLIGHT               0x08U

/* ========================================================================= */
/* PRIVATE VARIABLES                                                        */
/* ========================================================================= */

static uint8_t s_lcd_ready = 0U;
static VCU_LCD_Page_t s_current_page = VCU_LCD_PAGE_BMS;
static uint8_t s_auto_page_enabled = 1U;
static uint8_t s_update_requested = 1U;

static uint32_t s_last_page_change_ms = 0U;

/* ========================================================================= */
/* PRIVATE FUNCTION PROTOTYPES                                              */
/* ========================================================================= */

static void VCU_LCD_Delay(uint32_t delay_ms);

static void VCU_LCD_WriteNibble(uint8_t nibble, uint8_t rs);
static void VCU_LCD_SendByte(uint8_t value, uint8_t rs);

static void VCU_LCD_SendCommand(uint8_t command);
static void VCU_LCD_SendData(uint8_t data);

static void VCU_LCD_SetCursor(uint8_t row, uint8_t column);
static void VCU_LCD_WriteString(const char *text);
static void VCU_LCD_WriteLine(uint8_t row, const char *text);

static void VCU_LCD_FormatUInt(uint32_t value,
                               char *buffer,
                               uint8_t buffer_size);

static void VCU_LCD_FormatFloat1(float value,
                                 char *buffer,
                                 uint8_t buffer_size);

static void VCU_LCD_FormatInt8(int32_t value,
                               char *buffer,
                               uint8_t buffer_size);

static const char *VCU_LCD_GetStateName(VCU_SystemState_t state);
static const char *VCU_LCD_GetDirectionName(VCU_Direction_t direction);

static void VCU_LCD_ShowCurrentPage(void);

/* ========================================================================= */
/* BASIC LCD LOW-LEVEL FUNCTIONS                                            */
/* ========================================================================= */

static void VCU_LCD_Delay(uint32_t delay_ms)
{
    HAL_Delay(delay_ms);
}

/* ------------------------------------------------------------------------- */

static void VCU_LCD_WriteNibble(uint8_t nibble, uint8_t rs)
{
    uint8_t data;
    uint8_t en_high;
    uint8_t en_low;

    data = (uint8_t)((nibble & 0x0FU) << 4);

    if (rs != 0U)
    {
        data |= LCD_RS;
    }

    data |= LCD_BACKLIGHT;

    en_high = (uint8_t)(data | LCD_EN);
    en_low = data;

    HAL_I2C_Master_Transmit(&hi2c1,
                            VCU_LCD_I2C_ADDRESS,
                            &en_high,
                            1U,
                            100U);

    VCU_LCD_Delay(1U);

    HAL_I2C_Master_Transmit(&hi2c1,
                            VCU_LCD_I2C_ADDRESS,
                            &en_low,
                            1U,
                            100U);

    VCU_LCD_Delay(1U);
}

/* ------------------------------------------------------------------------- */

static void VCU_LCD_SendByte(uint8_t value, uint8_t rs)
{
    VCU_LCD_WriteNibble((uint8_t)(value >> 4), rs);
    VCU_LCD_WriteNibble((uint8_t)(value & 0x0FU), rs);
}

/* ------------------------------------------------------------------------- */

static void VCU_LCD_SendCommand(uint8_t command)
{
    VCU_LCD_SendByte(command, 0U);

    if ((command == 0x01U) || (command == 0x02U))
    {
        VCU_LCD_Delay(2U);
    }
}

/* ------------------------------------------------------------------------- */

static void VCU_LCD_SendData(uint8_t data)
{
    VCU_LCD_SendByte(data, 1U);
}

/* ------------------------------------------------------------------------- */

static void VCU_LCD_SetCursor(uint8_t row, uint8_t column)
{
    uint8_t address;

    if (row == 0U)
    {
        address = (uint8_t)(0x00U + column);
    }
    else
    {
        address = (uint8_t)(0x40U + column);
    }

    VCU_LCD_SendCommand((uint8_t)(0x80U | address));
}

/* ------------------------------------------------------------------------- */

static void VCU_LCD_WriteString(const char *text)
{
    if (text == NULL)
    {
        return;
    }

    while (*text != '\0')
    {
        VCU_LCD_SendData((uint8_t)*text);
        text++;
    }
}

/* ------------------------------------------------------------------------- */

static void VCU_LCD_WriteLine(uint8_t row, const char *text)
{
    uint8_t column = 0U;

    VCU_LCD_SetCursor(row, 0U);

    if (text != NULL)
    {
        while ((*text != '\0') && (column < VCU_LCD_COLUMNS))
        {
            VCU_LCD_SendData((uint8_t)*text);
            text++;
            column++;
        }
    }

    while (column < VCU_LCD_COLUMNS)
    {
        VCU_LCD_SendData((uint8_t)' ');
        column++;
    }
}

/* ========================================================================= */
/* FORMATTING FUNCTIONS                                                     */
/* ========================================================================= */

static void VCU_LCD_FormatUInt(uint32_t value,
                               char *buffer,
                               uint8_t buffer_size)
{
    char temp[11];
    uint8_t i = 0U;
    uint8_t j = 0U;

    if ((buffer == NULL) || (buffer_size == 0U))
    {
        return;
    }

    if (value == 0U)
    {
        buffer[0] = '0';

        if (buffer_size > 1U)
        {
            buffer[1] = '\0';
        }

        return;
    }

    while ((value > 0U) && (i < 10U))
    {
        temp[i++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    while ((i > 0U) && (j < (uint8_t)(buffer_size - 1U)))
    {
        buffer[j++] = temp[--i];
    }

    buffer[j] = '\0';
}

/* ------------------------------------------------------------------------- */

static void VCU_LCD_FormatInt8(int32_t value,
                               char *buffer,
                               uint8_t buffer_size)
{
    uint32_t magnitude;
    char temp[12];
    uint8_t index = 0U;
    uint8_t out = 0U;

    if ((buffer == NULL) || (buffer_size == 0U))
    {
        return;
    }

    if (value < 0)
    {
        if (buffer_size <= 2U)
        {
            buffer[0] = '\0';
            return;
        }

        buffer[out++] = '-';

        magnitude = (uint32_t)(-(value + 1));
        magnitude += 1U;
    }
    else
    {
        magnitude = (uint32_t)value;
    }

    if (magnitude == 0U)
    {
        if (out < (uint8_t)(buffer_size - 1U))
        {
            buffer[out++] = '0';
        }

        buffer[out] = '\0';
        return;
    }

    while ((magnitude > 0U) &&
           (index < (uint8_t)(sizeof(temp) - 1U)))
    {
        temp[index++] = (char)('0' + (magnitude % 10U));
        magnitude /= 10U;
    }

    while ((index > 0U) &&
           (out < (uint8_t)(buffer_size - 1U)))
    {
        buffer[out++] = temp[--index];
    }

    buffer[out] = '\0';
}

/* ------------------------------------------------------------------------- */

static void VCU_LCD_FormatFloat1(float value,
                                 char *buffer,
                                 uint8_t buffer_size)
{
    uint32_t integer_part;
    uint32_t decimal_part;
    uint8_t out = 0U;

    if ((buffer == NULL) || (buffer_size == 0U))
    {
        return;
    }

    if (value < 0.0f)
    {
        if (buffer_size <= 1U)
        {
            buffer[0] = '\0';
            return;
        }

        buffer[out++] = '-';
        value = -value;
    }

    integer_part = (uint32_t)value;

    decimal_part =
        (uint32_t)((value - (float)integer_part) * 10.0f + 0.5f);

    if (decimal_part >= 10U)
    {
        decimal_part = 0U;
        integer_part++;
    }

    {
        char integer_buffer[11];
        uint8_t i = 0U;

        VCU_LCD_FormatUInt(integer_part,
                           integer_buffer,
                           sizeof(integer_buffer));

        while ((integer_buffer[i] != '\0') &&
               (out < (uint8_t)(buffer_size - 1U)))
        {
            buffer[out++] = integer_buffer[i++];
        }
    }

    if (out < (uint8_t)(buffer_size - 1U))
    {
        buffer[out++] = '.';
    }

    if (out < (uint8_t)(buffer_size - 1U))
    {
        buffer[out++] = (char)('0' + decimal_part);
    }

    buffer[out] = '\0';
}

/* ========================================================================= */
/* STATE / DIRECTION TEXT                                                   */
/* ========================================================================= */

static const char *VCU_LCD_GetStateName(VCU_SystemState_t state)
{
    switch (state)
    {
        case VCU_STATE_INIT:
            return "INIT";

        case VCU_STATE_OFF:
            return "OFF";

        case VCU_STATE_READY:
            return "READY";

        case VCU_STATE_DRIVE:
            return "DRIVE";

        case VCU_STATE_BRAKE:
            return "BRAKE";

        case VCU_STATE_WARNING:
            return "WARN";

        case VCU_STATE_FAULT:
            return "FAULT";

        case VCU_STATE_SHUTDOWN:
            return "SHUT";

        default:
            return "UNK";
    }
}

/* ------------------------------------------------------------------------- */

static const char *VCU_LCD_GetDirectionName(VCU_Direction_t direction)
{
    switch (direction)
    {
        case VCU_DIR_FORWARD:
            return "FWD";

        case VCU_DIR_REVERSE:
            return "REV";

        case VCU_DIR_NEUTRAL:
        default:
            return "N";
    }
}

/* ========================================================================= */
/* PAGE DISPLAY FUNCTIONS                                                    */
/* ========================================================================= */

void VCU_LCD_ShowBMS(void)
{
    char voltage[12];
    char current[12];
    char soc[6];
    char temperature[6];
    char line1[17];
    char line2[17];

    if (s_lcd_ready == 0U)
    {
        return;
    }

    if (VCU_Data.bms.communication_valid == 0U)
    {
        VCU_LCD_WriteLine(0U, "BMS: NO DATA");
        VCU_LCD_WriteLine(1U, "CHECK CAN");
        return;
    }

    VCU_LCD_FormatFloat1(VCU_Data.bms.battery_voltage,
                         voltage,
                         sizeof(voltage));

    VCU_LCD_FormatFloat1(VCU_Data.bms.battery_current,
                         current,
                         sizeof(current));

    VCU_LCD_FormatUInt(VCU_Data.bms.soc,
                       soc,
                       sizeof(soc));

    VCU_LCD_FormatInt8((int32_t)VCU_Data.bms.temperature,
                       temperature,
                       sizeof(temperature));

    /* ------------------------------------------------------------- */
    /* LINE 1: BAT:48.2V SOC:82                                      */
    /* ------------------------------------------------------------- */

    line1[0] = '\0';

    {
        uint8_t index = 0U;
        uint8_t i = 0U;

        const char prefix1[] = "BAT:";
        const char prefix2[] = "V SOC:";

        while ((prefix1[i] != '\0') && (index < 16U))
        {
            line1[index++] = prefix1[i++];
        }

        i = 0U;

        while ((voltage[i] != '\0') && (index < 16U))
        {
            line1[index++] = voltage[i++];
        }

        if (index < 16U)
        {
            line1[index++] = 'V';
        }

        i = 0U;

        while ((prefix2[i] != '\0') && (index < 16U))
        {
            line1[index++] = prefix2[i++];
        }

        i = 0U;

        while ((soc[i] != '\0') && (index < 16U))
        {
            line1[index++] = soc[i++];
        }

        line1[index] = '\0';
    }

    /* ------------------------------------------------------------- */
    /* LINE 2: I:12.4A T:32C                                        */
    /* ------------------------------------------------------------- */

    line2[0] = '\0';

    {
        uint8_t index = 0U;
        uint8_t i = 0U;

        const char prefix1[] = "I:";
        const char prefix2[] = "A T:";

        while ((prefix1[i] != '\0') && (index < 16U))
        {
            line2[index++] = prefix1[i++];
        }

        i = 0U;

        while ((current[i] != '\0') && (index < 16U))
        {
            line2[index++] = current[i++];
        }

        i = 0U;

        while ((prefix2[i] != '\0') && (index < 16U))
        {
            line2[index++] = prefix2[i++];
        }

        i = 0U;

        while ((temperature[i] != '\0') && (index < 16U))
        {
            line2[index++] = temperature[i++];
        }

        if (index < 16U)
        {
            line2[index++] = 'C';
        }

        line2[index] = '\0';
    }

    VCU_LCD_WriteLine(0U, line1);
    VCU_LCD_WriteLine(1U, line2);
}

/* ------------------------------------------------------------------------- */

void VCU_LCD_ShowVCU(void)
{
    char throttle[6];
    char brake[6];
    char line1[17];
    char line2[17];

    if (s_lcd_ready == 0U)
    {
        return;
    }

    VCU_LCD_FormatUInt(
        (uint32_t)VCU_Data.input.analog.throttle_percent,
        throttle,
        sizeof(throttle));

    VCU_LCD_FormatUInt(
        (uint32_t)VCU_Data.input.analog.brake_percent,
        brake,
        sizeof(brake));

    /* ------------------------------------------------------------- */
    /* LINE 1: VCU:DRIVE FWD                                         */
    /* ------------------------------------------------------------- */

    line1[0] = '\0';

    {
        uint8_t index = 0U;
        uint8_t i = 0U;

        const char *state_name =
            VCU_LCD_GetStateName(VCU_Data.system_state);

        const char *direction_name =
            VCU_LCD_GetDirectionName(VCU_Data.input.direction);

        const char prefix[] = "VCU:";

        while ((prefix[i] != '\0') && (index < 16U))
        {
            line1[index++] = prefix[i++];
        }

        i = 0U;

        while ((state_name[i] != '\0') && (index < 16U))
        {
            line1[index++] = state_name[i++];
        }

        if (index < 16U)
        {
            line1[index++] = ' ';
        }

        i = 0U;

        while ((direction_name[i] != '\0') && (index < 16U))
        {
            line1[index++] = direction_name[i++];
        }

        line1[index] = '\0';
    }

    /* ------------------------------------------------------------- */
    /* LINE 2: THR:65 BRK:00                                         */
    /* ------------------------------------------------------------- */

    line2[0] = '\0';

    {
        uint8_t index = 0U;
        uint8_t i = 0U;

        const char prefix1[] = "THR:";
        const char prefix2[] = " BRK:";

        while ((prefix1[i] != '\0') && (index < 16U))
        {
            line2[index++] = prefix1[i++];
        }

        i = 0U;

        while ((throttle[i] != '\0') && (index < 16U))
        {
            line2[index++] = throttle[i++];
        }

        i = 0U;

        while ((prefix2[i] != '\0') && (index < 16U))
        {
            line2[index++] = prefix2[i++];
        }

        i = 0U;

        while ((brake[i] != '\0') && (index < 16U))
        {
            line2[index++] = brake[i++];
        }

        line2[index] = '\0';
    }

    VCU_LCD_WriteLine(0U, line1);
    VCU_LCD_WriteLine(1U, line2);
}

/* ------------------------------------------------------------------------- */

void VCU_LCD_ShowMCU(void)
{
    char rpm[8];
    char pwm[6];
    char enable[3];
    char line1[17];
    char line2[17];

    if (s_lcd_ready == 0U)
    {
        return;
    }

    if (VCU_Data.mcu.communication_valid == 0U)
    {
        VCU_LCD_WriteLine(0U, "MCU: NO DATA");
        VCU_LCD_WriteLine(1U, "CHECK CAN");
        return;
    }

    VCU_LCD_FormatUInt(VCU_Data.mcu.motor_speed,
                       rpm,
                       sizeof(rpm));

    /*
     * MCU status does not currently contain a separate PWM feedback
     * value. Therefore the VCU throttle request is displayed here
     * as the requested motor PWM level.
     */
    VCU_LCD_FormatUInt(
        (uint32_t)VCU_Data.driver_request.throttle_request_percent,
        pwm,
        sizeof(pwm));

    VCU_LCD_FormatUInt(VCU_Data.mcu.enable_state,
                       enable,
                       sizeof(enable));

    /* ------------------------------------------------------------- */
    /* LINE 1: MCU:RUN RPM:1240                                      */
    /* ------------------------------------------------------------- */

    line1[0] = '\0';

    {
        uint8_t index = 0U;
        uint8_t i = 0U;

        const char prefix1[] = "MCU:";
        const char prefix2[] = " RPM:";

        const char *mcu_state =
            (VCU_Data.mcu.enable_state != 0U)
            ? "RUN"
            : "STOP";

        while ((prefix1[i] != '\0') && (index < 16U))
        {
            line1[index++] = prefix1[i++];
        }

        i = 0U;

        while ((mcu_state[i] != '\0') && (index < 16U))
        {
            line1[index++] = mcu_state[i++];
        }

        i = 0U;

        while ((prefix2[i] != '\0') && (index < 16U))
        {
            line1[index++] = prefix2[i++];
        }

        i = 0U;

        while ((rpm[i] != '\0') && (index < 16U))
        {
            line1[index++] = rpm[i++];
        }

        line1[index] = '\0';
    }

    /* ------------------------------------------------------------- */
    /* LINE 2: PWM:65 EN:1 FWD                                      */
    /* ------------------------------------------------------------- */

    line2[0] = '\0';

    {
        uint8_t index = 0U;
        uint8_t i = 0U;

        const char prefix1[] = "PWM:";
        const char prefix2[] = " EN:";
        const char prefix3[] = " ";

        const char *direction_name =
            VCU_LCD_GetDirectionName(VCU_Data.input.direction);

        while ((prefix1[i] != '\0') && (index < 16U))
        {
            line2[index++] = prefix1[i++];
        }

        i = 0U;

        while ((pwm[i] != '\0') && (index < 16U))
        {
            line2[index++] = pwm[i++];
        }

        i = 0U;

        while ((prefix2[i] != '\0') && (index < 16U))
        {
            line2[index++] = prefix2[i++];
        }

        i = 0U;

        while ((enable[i] != '\0') && (index < 16U))
        {
            line2[index++] = enable[i++];
        }

        i = 0U;

        while ((prefix3[i] != '\0') && (index < 16U))
        {
            line2[index++] = prefix3[i++];
        }

        i = 0U;

        while ((direction_name[i] != '\0') && (index < 16U))
        {
            line2[index++] = direction_name[i++];
        }

        line2[index] = '\0';
    }

    VCU_LCD_WriteLine(0U, line1);
    VCU_LCD_WriteLine(1U, line2);
}

/* ------------------------------------------------------------------------- */

void VCU_LCD_ShowStatus(void)
{
    char fault_code[5];
    char soh[5];
    char line1[17];
    char line2[17];

    if (s_lcd_ready == 0U)
    {
        return;
    }

    VCU_LCD_FormatUInt(VCU_Data.faults.code,
                       fault_code,
                       sizeof(fault_code));

    VCU_LCD_FormatUInt(VCU_Data.bms.soh,
                       soh,
                       sizeof(soh));

    /* ------------------------------------------------------------- */
    /* LINE 1: BMS:OK MCU:OK                                         */
    /* ------------------------------------------------------------- */

    line1[0] = '\0';

    {
        uint8_t index = 0U;
        uint8_t i = 0U;

        const char *bms_state =
            (VCU_Data.bms.communication_valid != 0U)
            ? "OK"
            : "ERR";

        const char *mcu_state =
            (VCU_Data.mcu.communication_valid != 0U)
            ? "OK"
            : "ERR";

        const char prefix1[] = "BMS:";
        const char prefix2[] = " MCU:";

        while ((prefix1[i] != '\0') && (index < 16U))
        {
            line1[index++] = prefix1[i++];
        }

        i = 0U;

        while ((bms_state[i] != '\0') && (index < 16U))
        {
            line1[index++] = bms_state[i++];
        }

        i = 0U;

        while ((prefix2[i] != '\0') && (index < 16U))
        {
            line1[index++] = prefix2[i++];
        }

        i = 0U;

        while ((mcu_state[i] != '\0') && (index < 16U))
        {
            line1[index++] = mcu_state[i++];
        }

        line1[index] = '\0';
    }

    /* ------------------------------------------------------------- */
    /* LINE 2: FAULT:00 SOH:100                                     */
    /* ------------------------------------------------------------- */

    line2[0] = '\0';

    {
        uint8_t index = 0U;
        uint8_t i = 0U;

        const char prefix1[] = "FAULT:";
        const char prefix2[] = " SOH:";

        while ((prefix1[i] != '\0') && (index < 16U))
        {
            line2[index++] = prefix1[i++];
        }

        i = 0U;

        while ((fault_code[i] != '\0') && (index < 16U))
        {
            line2[index++] = fault_code[i++];
        }

        i = 0U;

        while ((prefix2[i] != '\0') && (index < 16U))
        {
            line2[index++] = prefix2[i++];
        }

        i = 0U;

        while ((soh[i] != '\0') && (index < 16U))
        {
            line2[index++] = soh[i++];
        }

        line2[index] = '\0';
    }

    VCU_LCD_WriteLine(0U, line1);
    VCU_LCD_WriteLine(1U, line2);
}

/* ========================================================================= */
/* PAGE CONTROL                                                             */
/* ========================================================================= */

static void VCU_LCD_ShowCurrentPage(void)
{
    switch (s_current_page)
    {
        case VCU_LCD_PAGE_BMS:
            VCU_LCD_ShowBMS();
            break;

        case VCU_LCD_PAGE_VCU:
            VCU_LCD_ShowVCU();
            break;

        case VCU_LCD_PAGE_MCU:
            VCU_LCD_ShowMCU();
            break;

        case VCU_LCD_PAGE_STATUS:
            VCU_LCD_ShowStatus();
            break;

        default:
            s_current_page = VCU_LCD_PAGE_BMS;
            VCU_LCD_ShowBMS();
            break;
    }

    s_update_requested = 0U;
}

/* ------------------------------------------------------------------------- */

void VCU_LCD_SetPage(VCU_LCD_Page_t page)
{
    if (page >= VCU_LCD_PAGE_COUNT)
    {
        return;
    }

    s_current_page = page;
    s_last_page_change_ms = HAL_GetTick();
    s_update_requested = 1U;
}

/* ------------------------------------------------------------------------- */

VCU_LCD_Page_t VCU_LCD_GetPage(void)
{
    return s_current_page;
}

/* ------------------------------------------------------------------------- */

void VCU_LCD_NextPage(void)
{
    if (s_current_page >= (VCU_LCD_PAGE_COUNT - 1U))
    {
        s_current_page = VCU_LCD_PAGE_BMS;
    }
    else
    {
        s_current_page++;
    }

    s_last_page_change_ms = HAL_GetTick();
    s_update_requested = 1U;
}

/* ------------------------------------------------------------------------- */

void VCU_LCD_RequestUpdate(void)
{
    s_update_requested = 1U;
}

/* ------------------------------------------------------------------------- */

void VCU_LCD_EnableAutoPage(void)
{
    s_auto_page_enabled = 1U;
    s_last_page_change_ms = HAL_GetTick();
}

/* ------------------------------------------------------------------------- */

void VCU_LCD_DisableAutoPage(void)
{
    s_auto_page_enabled = 0U;
}

/* ------------------------------------------------------------------------- */

uint8_t VCU_LCD_IsAutoPageEnabled(void)
{
    return s_auto_page_enabled;
}

/* ========================================================================= */
/* PUBLIC LCD FUNCTIONS                                                      */
/* ========================================================================= */

void VCU_LCD_Init(void)
{
    HAL_StatusTypeDef result;

    s_lcd_ready = 0U;
    s_current_page = VCU_LCD_PAGE_BMS;
    s_auto_page_enabled = 1U;
    s_update_requested = 1U;
    s_last_page_change_ms = HAL_GetTick();

    /*
     * Check whether the LCD backpack responds.
     */
    result = HAL_I2C_IsDeviceReady(&hi2c1,
                                   VCU_LCD_I2C_ADDRESS,
                                   3U,
                                   100U);

    if (result != HAL_OK)
    {
        s_lcd_ready = 0U;
        return;
    }

    /*
     * HD44780 power-up delay.
     */
    VCU_LCD_Delay(50U);

    /*
     * 4-bit initialization.
     */
    VCU_LCD_WriteNibble(0x03U, 0U);
    VCU_LCD_Delay(5U);

    VCU_LCD_WriteNibble(0x03U, 0U);
    VCU_LCD_Delay(1U);

    VCU_LCD_WriteNibble(0x03U, 0U);
    VCU_LCD_Delay(1U);

    VCU_LCD_WriteNibble(0x02U, 0U);

    /*
     * LCD configuration.
     */
    VCU_LCD_SendCommand(0x28U);   /* 4-bit, 2-line, 5x8 font */
    VCU_LCD_SendCommand(0x08U);   /* Display OFF */
    VCU_LCD_SendCommand(0x01U);   /* Clear */
    VCU_LCD_SendCommand(0x06U);   /* Entry mode */
    VCU_LCD_SendCommand(0x0CU);   /* Display ON, cursor OFF */

    /*
     * Startup message.
     *
     * VCU_LCD_WriteString() is intentionally used here so the
     * helper is part of the active LCD implementation.
     */
    VCU_LCD_SetCursor(0U, 0U);
    VCU_LCD_WriteString("EV SYSTEM");

    VCU_LCD_WriteLine(1U, "LCD READY");

    VCU_LCD_Delay(1000U);

    VCU_LCD_SendCommand(0x01U);

    s_lcd_ready = 1U;
    s_last_page_change_ms = HAL_GetTick();
    s_update_requested = 1U;
}

/* ------------------------------------------------------------------------- */

void VCU_LCD_Update(void)
{
    uint32_t current_time;

    if (s_lcd_ready == 0U)
    {
        return;
    }

    current_time = HAL_GetTick();

    /*
     * Automatic page rotation every 2 seconds.
     */
    if (s_auto_page_enabled != 0U)
    {
        if ((current_time - s_last_page_change_ms) >=
            VCU_LCD_PAGE_TIME_MS)
        {
            VCU_LCD_NextPage();
        }
    }

    /*
     * Refresh the page when required.
     */
    if (s_update_requested != 0U)
    {
        VCU_LCD_ShowCurrentPage();
    }
}

/* ------------------------------------------------------------------------- */

uint8_t VCU_LCD_IsReady(void)
{
    return s_lcd_ready;
}

/* ------------------------------------------------------------------------- */

void VCU_LCD_Clear(void)
{
    if (s_lcd_ready == 0U)
    {
        return;
    }

    VCU_LCD_SendCommand(0x01U);
}

/* ------------------------------------------------------------------------- */

void VCU_LCD_ShowMessage(const char *line1, const char *line2)
{
    if (s_lcd_ready == 0U)
    {
        return;
    }

    VCU_LCD_WriteLine(0U, line1);
    VCU_LCD_WriteLine(1U, line2);
}

/* ========================================================================= */
/* END OF FILE                                                               */
/* ========================================================================= */
