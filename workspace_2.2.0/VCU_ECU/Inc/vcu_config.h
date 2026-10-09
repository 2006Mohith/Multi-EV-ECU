#ifndef VCU_CONFIG_H
#define VCU_CONFIG_H

/*
 * VCU ECU Central Configuration
 *
 * MCU:
 * STM32F103C8T6
 *
 * System clock:
 * 72 MHz
 *
 * CAN:
 * 500 kbps
 *
 * This file contains system limits, CAN identifiers,
 * timing periods, ADC limits, and state definitions.
 */

/* =========================================================
 * VCU GENERAL CONFIGURATION
 * ========================================================= */

#define VCU_FIRMWARE_VERSION_MAJOR          1U
#define VCU_FIRMWARE_VERSION_MINOR          0U
#define VCU_FIRMWARE_VERSION_PATCH          0U

#define VCU_SYSTEM_STARTUP_DELAY_MS         100U

/* =========================================================
 * VCU ADC CONFIGURATION
 * ========================================================= */

#define VCU_ADC_MAX_VALUE                   4095U
#define VCU_ADC_REFERENCE_VOLTAGE           3.3f

#define VCU_ADC_CHANNEL_COUNT               4U

#define VCU_ADC_THROTTLE_CHANNEL            0U
#define VCU_ADC_BRAKE_CHANNEL               1U
#define VCU_ADC_AUX1_CHANNEL                2U
#define VCU_ADC_AUX2_CHANNEL                3U

#define VCU_ADC_MIN_VALID_VALUE             0U
#define VCU_ADC_MAX_VALID_VALUE             4095U

#define VCU_THROTTLE_MIN_PERCENT            0.0f
#define VCU_THROTTLE_MAX_PERCENT            100.0f

#define VCU_BRAKE_MIN_PERCENT               0.0f
#define VCU_BRAKE_MAX_PERCENT               100.0f

/*
 * These thresholds are initial software values.
 * They must be calibrated against the actual accelerator
 * and brake sensors during hardware bring-up.
 */
#define VCU_THROTTLE_DEADZONE_PERCENT       2.0f
#define VCU_BRAKE_DEADZONE_PERCENT          2.0f

#define VCU_THROTTLE_FAULT_LOW_PERCENT      0.5f
#define VCU_THROTTLE_FAULT_HIGH_PERCENT     99.5f

/* =========================================================
 * VCU SAFETY LIMITS
 * ========================================================= */

#define VCU_MAX_ALLOWED_THROTTLE_PERCENT    100.0f
#define VCU_MAX_ALLOWED_BRAKE_PERCENT       100.0f

/*
 * Maximum throttle permitted while braking.
 * Initial safety value for development.
 */
#define VCU_MAX_THROTTLE_WHILE_BRAKING      5.0f

/*
 * Accelerator plausibility timeout.
 */
#define VCU_INPUT_TIMEOUT_MS                100U

/* =========================================================
 * VCU VEHICLE STATES
 * ========================================================= */

#define VCU_SYSTEM_INIT                     0U
#define VCU_SYSTEM_OFF                      1U
#define VCU_SYSTEM_READY                    2U
#define VCU_SYSTEM_DRIVE                    3U
#define VCU_SYSTEM_BRAKE                    4U
#define VCU_SYSTEM_WARNING                  5U
#define VCU_SYSTEM_FAULT                    6U
#define VCU_SYSTEM_SHUTDOWN                 7U

/* =========================================================
 * VCU DIRECTION
 * ========================================================= */

#define VCU_DIRECTION_NEUTRAL               0U
#define VCU_DIRECTION_FORWARD               1U
#define VCU_DIRECTION_REVERSE               2U

/* =========================================================
 * VCU ENABLE STATES
 * ========================================================= */

#define VCU_DISABLED                       0U
#define VCU_ENABLED                        1U

/* =========================================================
 * VCU FAULT STATES
 * ========================================================= */

#define VCU_FAULT_NONE                      0U
#define VCU_FAULT_BMS_COMMUNICATION         1U
#define VCU_FAULT_BMS_CRITICAL              2U
#define VCU_FAULT_MCU_COMMUNICATION         3U
#define VCU_FAULT_MCU_CRITICAL              4U
#define VCU_FAULT_THROTTLE                  5U
#define VCU_FAULT_BRAKE                     6U
#define VCU_FAULT_DIRECTION                 7U
#define VCU_FAULT_EMERGENCY                 8U
#define VCU_FAULT_IGNITION                  9U
#define VCU_FAULT_SYSTEM                    10U

/* =========================================================
 * VCU FAULT SEVERITY
 * ========================================================= */

#define VCU_FAULT_SEVERITY_NONE             0U
#define VCU_FAULT_SEVERITY_WARNING          1U
#define VCU_FAULT_SEVERITY_CRITICAL         2U

/* =========================================================
 * BMS CAN PROTOCOL
 * ========================================================= */

/*
 * Existing BMS protocol:
 *
 * BMS status:
 * ID 0x100
 *
 * BMS command:
 * ID 0x200
 */

#define VCU_CAN_BMS_STATUS_ID               0x100U
#define VCU_CAN_BMS_COMMAND_ID              0x200U

/* BMS commands */
#define VCU_BMS_CMD_STATUS_REQUEST          0x01U
#define VCU_BMS_CMD_FAULT_RESET             0x02U
#define VCU_BMS_CMD_ENABLE                  0x03U
#define VCU_BMS_CMD_DISABLE                 0x04U

/* =========================================================
 * MCU CAN PROTOCOL
 * ========================================================= */

/*
 * VCU → MCU command frame
 */
#define VCU_CAN_MCU_COMMAND_ID              0x300U

/*
 * MCU → VCU status frame
 */
#define VCU_CAN_MCU_STATUS_ID               0x301U

/* MCU command bytes */
#define VCU_MCU_CMD_DISABLE                 0x00U
#define VCU_MCU_CMD_ENABLE                  0x01U
#define VCU_MCU_CMD_RESET                   0x02U
#define VCU_MCU_CMD_FORWARD                 0x03U
#define VCU_MCU_CMD_REVERSE                 0x04U
#define VCU_MCU_CMD_NEUTRAL                 0x05U

/* =========================================================
 * VCU STATUS CAN FRAME
 * ========================================================= */

/*
 * VCU → other ECUs status frame
 */
#define VCU_CAN_STATUS_ID                   0x400U

/* =========================================================
 * CAN DLC
 * ========================================================= */

#define VCU_CAN_STANDARD_DLC                8U

/* =========================================================
 * CAN COMMUNICATION TIMING
 * ========================================================= */

#define VCU_CAN_RX_TIMEOUT_MS               500U

#define VCU_BMS_STATUS_TIMEOUT_MS           500U
#define VCU_MCU_STATUS_TIMEOUT_MS           500U

#define VCU_CAN_STATUS_PERIOD_MS            100U
#define VCU_CAN_COMMAND_PERIOD_MS           100U

/* =========================================================
 * SOFTWARE TASK TIMING
 * ========================================================= */

#define VCU_ADC_UPDATE_PERIOD_MS            20U
#define VCU_INPUT_UPDATE_PERIOD_MS          20U
#define VCU_STATE_UPDATE_PERIOD_MS          20U
#define VCU_FAULT_UPDATE_PERIOD_MS          20U
#define VCU_CAN_UPDATE_PERIOD_MS            20U
#define VCU_LCD_UPDATE_PERIOD_MS            500U

/* =========================================================
 * FAULT DEBOUNCE / RECOVERY
 * ========================================================= */

#define VCU_FAULT_DEBOUNCE_MS               100U
#define VCU_FAULT_RECOVERY_MS               500U

#define VCU_REQUIRED_VALID_SAMPLES          3U
#define VCU_MAX_INVALID_SAMPLES             3U

/* =========================================================
 * BMS STATUS FRAME BYTE DEFINITIONS
 * ========================================================= */

/*
 * BMS status CAN ID = 0x100
 *
 * Byte 0-1 : Battery voltage × 10
 * Byte 2-3 : Signed battery current × 10
 * Byte 4   : SOC %
 * Byte 5   : Signed temperature
 * Byte 6   : SOH %
 * Byte 7   : Fault status
 */

#define VCU_BMS_STATUS_VOLTAGE_MSB          0U
#define VCU_BMS_STATUS_VOLTAGE_LSB          1U

#define VCU_BMS_STATUS_CURRENT_MSB          2U
#define VCU_BMS_STATUS_CURRENT_LSB          3U

#define VCU_BMS_STATUS_SOC_BYTE             4U
#define VCU_BMS_STATUS_TEMPERATURE_BYTE     5U
#define VCU_BMS_STATUS_SOH_BYTE             6U
#define VCU_BMS_STATUS_FAULT_BYTE           7U

/* =========================================================
 * MCU STATUS FRAME BYTE DEFINITIONS
 * ========================================================= */

/*
 * MCU status CAN ID = 0x301
 *
 * Byte 0-1 : Motor speed
 * Byte 2-3 : Motor current
 * Byte 4   : Controller temperature
 * Byte 5   : Motor enable state
 * Byte 6   : Fault status
 * Byte 7   : Reserved
 *
 * Final MCU frame format will be defined together with
 * the MCU_ECU software.
 */

#define VCU_MCU_STATUS_SPEED_MSB            0U
#define VCU_MCU_STATUS_SPEED_LSB            1U

#define VCU_MCU_STATUS_CURRENT_MSB          2U
#define VCU_MCU_STATUS_CURRENT_LSB          3U

#define VCU_MCU_STATUS_TEMPERATURE_BYTE     4U
#define VCU_MCU_STATUS_ENABLE_BYTE          5U
#define VCU_MCU_STATUS_FAULT_BYTE           6U
#define VCU_MCU_STATUS_RESERVED_BYTE        7U

/* =========================================================
 * VCU STATUS FRAME BYTE DEFINITIONS
 * ========================================================= */

/*
 * VCU status CAN ID = 0x400
 *
 * Byte 0 : VCU state
 * Byte 1 : Direction
 * Byte 2 : Throttle %
 * Byte 3 : Brake %
 * Byte 4 : VCU fault status
 * Byte 5 : BMS communication status
 * Byte 6 : MCU communication status
 * Byte 7 : Vehicle enable status
 */

#define VCU_STATUS_STATE_BYTE               0U
#define VCU_STATUS_DIRECTION_BYTE           1U
#define VCU_STATUS_THROTTLE_BYTE            2U
#define VCU_STATUS_BRAKE_BYTE               3U
#define VCU_STATUS_FAULT_BYTE               4U
#define VCU_STATUS_BMS_COMM_BYTE            5U
#define VCU_STATUS_MCU_COMM_BYTE            6U
#define VCU_STATUS_ENABLE_BYTE              7U

/* =========================================================
 * DIGITAL INPUT LOGIC
 * ========================================================= */

/*
 * These values describe the logical state used by the
 * application. Physical switch polarity will be finalized
 * during hardware integration.
 */

#define VCU_INPUT_INACTIVE                  0U
#define VCU_INPUT_ACTIVE                    1U

/* =========================================================
 * CONTACTOR / MOTOR CONTROL
 * ========================================================= */

#define VCU_MOTOR_DISABLE                   0U
#define VCU_MOTOR_ENABLE                    1U

#define VCU_CONTACTOR_DISABLE               0U
#define VCU_CONTACTOR_ENABLE                1U

/* =========================================================
 * LCD CONFIGURATION
 * ========================================================= */

#define VCU_LCD_I2C_ADDRESS                 (0x27U << 1)

#define VCU_LCD_COLUMNS                     16U
#define VCU_LCD_ROWS                        2U

/* =========================================================
 * UART DEBUG CONFIGURATION
 * ========================================================= */

#define VCU_UART_BAUD_RATE                  115200U

/* =========================================================
 * GPIO USER LABELS
 * ========================================================= */

/*
 * Actual GPIO ports and pins are generated by CubeMX
 * in gpio.h. These labels are documented here for the
 * VCU application.
 *
 * PB0  = VCU_STATUS_LED
 * PB1  = VCU_FAULT_LED
 * PB8  = MOTOR_ENABLE
 * PB9  = CONTACTOR_ENABLE
 *
 * PB10 = IGNITION_INPUT
 * PB11 = EMERGENCY_INPUT
 * PB12 = FORWARD_INPUT
 * PB13 = REVERSE_INPUT
 */

/* =========================================================
 * NUMERICAL LIMITS
 * ========================================================= */

#define VCU_PERCENT_MIN                    0.0f
#define VCU_PERCENT_MAX                    100.0f

#define VCU_TEMPERATURE_MIN                -40.0f
#define VCU_TEMPERATURE_MAX                125.0f

/* =========================================================
 * FEATURE SWITCHES
 * ========================================================= */

#define VCU_ENABLE_LCD                     1U
#define VCU_ENABLE_DEBUG_UART              1U

/*
 * Reserved for future software features.
 */
#define VCU_ENABLE_REVERSE_DRIVE           1U
#define VCU_ENABLE_REGENERATIVE_BRAKING    0U

#endif /* VCU_CONFIG_H */
