#ifndef MCU_CONFIG_H
#define MCU_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * MCU ECU CONFIGURATION
 * STM32F103C8T6
 *
 * Hardware:
 * - TB6612FNG motor driver
 * - DC motor
 * - Digital Hall sensor
 * - CAN transceiver
 * - 16x2 I2C LCD
 * - Potentiometer
 * - 4 push buttons
 * - Green LED
 * - Red LED
 * - Buzzer
 *
 * ========================================================================== */

/* ============================================================================
 * Firmware information
 * ========================================================================== */

#define MCU_FIRMWARE_MAJOR                 1U
#define MCU_FIRMWARE_MINOR                 0U
#define MCU_FIRMWARE_PATCH                 0U

/* ============================================================================
 * General timing
 * ========================================================================== */

#define MCU_CONTROL_PERIOD_MS              20U
#define MCU_CAN_PERIOD_MS                  100U
#define MCU_LCD_PERIOD_MS                  500U
#define MCU_DEBUG_PERIOD_MS                1000U

/* CAN communication timeout */
#define MCU_VCU_TIMEOUT_MS                 500U

/* Hall/RPM timeout */
#define MCU_HALL_TIMEOUT_MS                500U

/* ============================================================================
 * MCU clock related configuration
 * ========================================================================== */

#define MCU_SYSTEM_CLOCK_HZ                72000000UL
#define MCU_APB1_CLOCK_HZ                  36000000UL
#define MCU_APB2_CLOCK_HZ                  72000000UL

/* ============================================================================
 * ADC / potentiometer
 * ========================================================================== */

#define MCU_ADC_MAX_VALUE                  4095.0f
#define MCU_ADC_REFERENCE_VOLTAGE          3.3f

#define MCU_POT_ADC_CHANNEL                1U

/* Potentiometer command range */
#define MCU_POT_MIN_PERCENT                0U
#define MCU_POT_MAX_PERCENT                100U

/* ============================================================================
 * Motor PWM
 * ========================================================================== */

/*
 * TIM1_CH1:
 * PA8 -> TB6612FNG PWMA
 *
 * Timer:
 * TIM1 clock = 72 MHz
 * Prescaler  = 0
 * Period     = 3599
 *
 * PWM frequency:
 * 72 MHz / 3600 = 20 kHz
 */

#define MCU_PWM_FREQUENCY_HZ               20000UL
#define MCU_PWM_PRESCALER                  0U
#define MCU_PWM_PERIOD                     3599U

#define MCU_PWM_MIN_PERCENT                0U
#define MCU_PWM_MAX_PERCENT                100U

/* ============================================================================
 * TB6612FNG motor-control logic
 * ========================================================================== */

#define MCU_MOTOR_FORWARD                  1U
#define MCU_MOTOR_REVERSE                  2U
#define MCU_MOTOR_NEUTRAL                  0U

#define MCU_MOTOR_DISABLED                 0U
#define MCU_MOTOR_ENABLED                  1U

/* TB6612FNG initial safe state */
#define MCU_MOTOR_STARTUP_PWM              0U
#define MCU_MOTOR_STARTUP_STBY             0U
#define MCU_MOTOR_STARTUP_AIN1             0U
#define MCU_MOTOR_STARTUP_AIN2             0U

/* ============================================================================
 * Hall sensor / RPM measurement
 * ========================================================================== */

/*
 * TIM2_CH1:
 * PA0 -> Hall sensor output
 *
 * TIM2:
 * Timer clock = 72 MHz
 * Prescaler   = 71
 * Counter     = 1 MHz
 *
 * Therefore:
 * 1 timer count = 1 us
 */

/*
 * IMPORTANT:
 * This is a provisional value.
 *
 * Set the actual number after determining how many Hall pulses occur
 * during one revolution of the shaft being measured.
 */
#define MCU_HALL_PULSES_PER_REV             1U

#define MCU_HALL_TIMER_FREQUENCY_HZ         1000000UL

#define MCU_RPM_MIN                         0U
#define MCU_RPM_MAX                         60000U

/* ============================================================================
 * Motor states
 * ========================================================================== */

#define MCU_STATE_INIT                      0U
#define MCU_STATE_DISABLED                  1U
#define MCU_STATE_READY                     2U
#define MCU_STATE_RUN                       3U
#define MCU_STATE_BRAKE                     4U
#define MCU_STATE_WARNING                   5U
#define MCU_STATE_FAULT                     6U
#define MCU_STATE_SHUTDOWN                  7U

/* ============================================================================
 * Motor direction
 * ========================================================================== */

#define MCU_DIRECTION_NEUTRAL               0U
#define MCU_DIRECTION_FORWARD               1U
#define MCU_DIRECTION_REVERSE               2U

#define MCU_REVERSE_ENABLED                 1U

/* ============================================================================
 * Enable / disable
 * ========================================================================== */

#define MCU_DISABLED                        0U
#define MCU_ENABLED                         1U

/* ============================================================================
 * Fault severity
 * ========================================================================== */

#define MCU_FAULT_SEVERITY_NONE             0U
#define MCU_FAULT_SEVERITY_INFO             1U
#define MCU_FAULT_SEVERITY_WARNING          2U
#define MCU_FAULT_SEVERITY_DERATE           3U
#define MCU_FAULT_SEVERITY_FAULT            4U
#define MCU_FAULT_SEVERITY_CRITICAL         5U

/* ============================================================================
 * Fault codes
 * ========================================================================== */

#define MCU_FAULT_NONE                      0U

#define MCU_FAULT_VCU_COMM                  1U
#define MCU_FAULT_CAN_RX                    2U
#define MCU_FAULT_CAN_COMMAND               3U

#define MCU_FAULT_HALL_SIGNAL               4U
#define MCU_FAULT_HALL_TIMEOUT              5U

#define MCU_FAULT_DIRECTION                 6U
#define MCU_FAULT_MOTOR_DRIVER              7U
#define MCU_FAULT_MOTOR_ENABLE              8U

#define MCU_FAULT_POTENTIOMETER             9U
#define MCU_FAULT_BUTTON                    10U

#define MCU_FAULT_SYSTEM                   11U
#define MCU_FAULT_WATCHDOG                 12U

/* ============================================================================
 * Fault handling
 * ========================================================================== */

#define MCU_FAULT_CONFIRMATION_COUNT        3U
#define MCU_FAULT_RECOVERY_COUNT            5U

#define MCU_ENABLE_FAULT_LATCHING           1U

/* ============================================================================
 * CAN communication
 * ========================================================================== */

/*
 * CAN bit rate:
 * 500 kbit/s
 */

#define MCU_CAN_BAUDRATE                    500000UL

/* --------------------------------------------------------------------------
 * CAN IDs
 * -------------------------------------------------------------------------- */

/* VCU -> MCU */
#define MCU_CAN_ID_VCU_COMMAND              0x300U

/* MCU -> VCU */
#define MCU_CAN_ID_MCU_STATUS               0x301U
#define MCU_CAN_ID_MCU_HEARTBEAT            0x310U

/* Optional future diagnostic frame */
#define MCU_CAN_ID_MCU_DIAGNOSTIC           0x320U

#define MCU_CAN_STANDARD_DLC                8U

/* ============================================================================
 * VCU -> MCU command frame
 * ========================================================================== */

/*
 * CAN ID = 0x300
 *
 * Byte 0 : Command
 * Byte 1 : Direction
 * Byte 2 : PWM / motor demand %
 * Byte 3 : Enable
 * Byte 4 : Requested state
 * Byte 5 : Command counter
 * Byte 6 : Reserved
 * Byte 7 : Reserved
 */

/* Command values */
#define MCU_COMMAND_NONE                    0x00U
#define MCU_COMMAND_STOP                    0x01U
#define MCU_COMMAND_RUN                     0x02U
#define MCU_COMMAND_BRAKE                   0x03U
#define MCU_COMMAND_RESET_FAULT             0x04U
#define MCU_COMMAND_ENABLE                 0x05U
#define MCU_COMMAND_DISABLE                0x06U

#define MCU_COMMAND_BYTE                    0U
#define MCU_COMMAND_DIRECTION_BYTE          1U
#define MCU_COMMAND_PWM_BYTE                2U
#define MCU_COMMAND_ENABLE_BYTE             3U
#define MCU_COMMAND_STATE_BYTE              4U
#define MCU_COMMAND_COUNTER_BYTE            5U

/* ============================================================================
 * MCU -> VCU status frame
 * ========================================================================== */

/*
 * CAN ID = 0x301
 *
 * Byte 0-1 : Motor RPM
 * Byte 2-3 : Motor current (reserved for future sensor)
 * Byte 4   : Motor temperature (reserved for future sensor)
 * Byte 5   : Enable status
 * Byte 6   : Fault code
 * Byte 7   : Direction
 */

#define MCU_STATUS_BYTE_RPM_HIGH            0U
#define MCU_STATUS_BYTE_RPM_LOW             1U
#define MCU_STATUS_BYTE_CURRENT_HIGH        2U
#define MCU_STATUS_BYTE_CURRENT_LOW         3U
#define MCU_STATUS_BYTE_TEMPERATURE         4U
#define MCU_STATUS_BYTE_ENABLE              5U
#define MCU_STATUS_BYTE_FAULT               6U
#define MCU_STATUS_BYTE_DIRECTION           7U

/* ============================================================================
 * MCU heartbeat frame
 * ========================================================================== */

/*
 * CAN ID = 0x310
 *
 * Byte 0 : Alive counter
 * Byte 1 : MCU state
 * Byte 2 : Fault code
 * Byte 3 : Enable status
 * Byte 4-7 : Reserved
 */

#define MCU_HEARTBEAT_BYTE_COUNTER           0U
#define MCU_HEARTBEAT_BYTE_STATE             1U
#define MCU_HEARTBEAT_BYTE_FAULT             2U
#define MCU_HEARTBEAT_BYTE_ENABLE            3U

#define MCU_HEARTBEAT_COUNTER_MAX            255U

/* ============================================================================
 * CAN command timeout / supervision
 * ========================================================================== */

#define MCU_CAN_COMMAND_TIMEOUT_MS           500U

/* ============================================================================
 * LCD configuration
 * ========================================================================== */

#define MCU_LCD_ENABLED                      1U

/*
 * Common PCF8574 backpack address.
 * Verify the actual address later.
 */
#define MCU_LCD_I2C_ADDRESS                  (0x27U << 1)

#define MCU_LCD_UPDATE_PERIOD_MS             500U

#define MCU_LCD_COLUMNS                      16U
#define MCU_LCD_ROWS                         2U

/* ============================================================================
 * UART debug configuration
 * ========================================================================== */

#define MCU_DEBUG_UART_ENABLED               1U
#define MCU_DEBUG_UART_BAUDRATE              115200UL

/* ============================================================================
 * Digital input configuration
 * ========================================================================== */

/*
 * Buttons are configured as active LOW using internal pull-ups.
 */

#define MCU_BUTTON_ACTIVE_LEVEL              0U

#define MCU_START_BUTTON_ACTIVE              0U
#define MCU_STOP_BUTTON_ACTIVE               0U
#define MCU_FORWARD_BUTTON_ACTIVE            0U
#define MCU_REVERSE_BUTTON_ACTIVE            0U

/* ============================================================================
 * Button debounce
 * ========================================================================== */

#define MCU_BUTTON_DEBOUNCE_MS               30U

/* ============================================================================
 * Buzzer configuration
 * ========================================================================== */

#define MCU_BUZZER_OFF                       0U
#define MCU_BUZZER_ON                        1U

#define MCU_BUZZER_WARNING_PERIOD_MS         500U
#define MCU_BUZZER_FAULT_PERIOD_MS           200U

/* ============================================================================
 * LED configuration
 * ========================================================================== */

#define MCU_STATUS_LED_ON                    1U
#define MCU_STATUS_LED_OFF                   0U

#define MCU_FAULT_LED_ON                     1U
#define MCU_FAULT_LED_OFF                    0U

/* ============================================================================
 * Safety limits
 * ========================================================================== */

/*
 * If brake functionality is added later, this limit can be used to
 * restrict propulsion while braking.
 */
#define MCU_MAX_PWM_WHILE_BRAKING            0U

/* Direction change protection */
#define MCU_DIRECTION_CHANGE_DELAY_MS        100U

/* ============================================================================
 * Local test mode
 * ========================================================================== */

/*
 * Potentiometer/buttons are intended for bench testing.
 *
 * Normal vehicle control comes from the VCU through CAN.
 */
#define MCU_LOCAL_TEST_MODE_ENABLED          1U

/* ============================================================================
 * Software task supervision
 * ========================================================================== */

#define MCU_TASK_ADC                         0x01U
#define MCU_TASK_CAN                         0x02U
#define MCU_TASK_HALL                        0x04U
#define MCU_TASK_MOTOR                       0x08U
#define MCU_TASK_INPUT                       0x10U
#define MCU_TASK_LCD                         0x20U

#define MCU_TASK_REQUIRED_MASK               (MCU_TASK_ADC   | \
                                              MCU_TASK_CAN   | \
                                              MCU_TASK_HALL  | \
                                              MCU_TASK_MOTOR | \
                                              MCU_TASK_INPUT | \
                                              MCU_TASK_LCD)

#define MCU_TASK_SUPERVISION_TIMEOUT_MS      100U

/* ============================================================================
 * Boolean / utility definitions
 * ========================================================================== */

#define MCU_FALSE                            0U
#define MCU_TRUE                             1U

#define MCU_PERCENT_MIN                      0U
#define MCU_PERCENT_MAX                      100U

#ifdef __cplusplus
}
#endif

#endif /* MCU_CONFIG_H */
