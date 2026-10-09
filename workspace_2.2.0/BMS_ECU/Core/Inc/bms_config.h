#ifndef BMS_CONFIG_H
#define BMS_CONFIG_H

/*
 * ============================================================
 * BMS ECU CENTRAL CONFIGURATION
 * STM32F103C8T6
 * ============================================================
 *
 * This file contains all application-level configuration:
 *
 *   - Battery limits
 *   - Protection hysteresis
 *   - ADC filtering
 *   - Sensor scaling
 *   - Measurement validity
 *   - SOC / SOH configuration
 *   - Fault timing
 *   - BMS task timing
 *   - CAN identifiers and commands
 *   - BMS system states
 *   - LCD configuration
 *
 * IMPORTANT:
 * The voltage/current scaling values are provisional until
 * they are calibrated against the actual sensing circuits.
 */


/* ============================================================
 * BATTERY PACK LIMITS
 * ============================================================ */

/*
 * Pack voltage limits.
 */
#define BMS_MAX_VOLTAGE                 54.6f
#define BMS_MIN_VOLTAGE                 39.0f


/*
 * Pack temperature limits.
 */
#define BMS_MAX_TEMPERATURE             45.0f
#define BMS_MIN_TEMPERATURE              0.0f


/*
 * Maximum allowed magnitude of battery current.
 *
 * Both:
 *
 *     +BMS_MAX_CURRENT
 *
 * and:
 *
 *     -BMS_MAX_CURRENT
 *
 * are treated as over-current conditions.
 */
#define BMS_MAX_CURRENT                 50.0f


/* ============================================================
 * PROTECTION HYSTERESIS
 * ============================================================ */

/*
 * Over-voltage:
 *
 * Fault activates above 54.6 V.
 * Fault recovery region is 54.0 V or lower.
 */
#define BMS_OVERVOLTAGE_RELEASE         54.0f


/*
 * Under-voltage:
 *
 * Fault activates below 39.0 V.
 * Fault recovery region is 39.6 V or higher.
 */
#define BMS_UNDERVOLTAGE_RELEASE        39.6f


/*
 * Over-current:
 *
 * Fault activates above ±50 A.
 * Recovery region is within ±48 A.
 */
#define BMS_OVERCURRENT_RELEASE         48.0f


/*
 * Over-temperature:
 *
 * Fault activates above 45 °C.
 * Recovery region is 43 °C or lower.
 */
#define BMS_OVERTEMPERATURE_RELEASE     43.0f


/*
 * Under-temperature:
 *
 * Fault activates below 0 °C.
 * Recovery region is 2 °C or higher.
 */
#define BMS_UNDERTEMPERATURE_RELEASE     2.0f


/* ============================================================
 * ADC CONFIGURATION
 * ============================================================ */

/*
 * STM32F103 ADC resolution:
 * 12-bit -> 0 ... 4095
 */
#define ADC_MAX_VALUE                   4095.0f


/*
 * ADC reference voltage.
 */
#define ADC_REFERENCE_VOLTAGE           3.3f


/*
 * Number of samples averaged by the BMS ADC software.
 */
#define BMS_ADC_FILTER_SAMPLES         8U


/*
 * Valid raw ADC range.
 */
#define BMS_ADC_MIN_VALID               0U
#define BMS_ADC_MAX_VALID            4095U


/* ============================================================
 * BATTERY VOLTAGE SENSOR
 * ============================================================ */

/*
 * Battery voltage calculation:
 *
 *     Vbattery = Vadc × scale
 *
 * Current value is provisional.
 *
 * IMPORTANT:
 * Replace this after calculating the actual resistor-divider
 * ratio from the hardware.
 */
#define BATTERY_VOLTAGE_SCALE          16.545f


/* ============================================================
 * CURRENT SENSOR
 * ============================================================ */

/*
 * Current calculation:
 *
 *     Ibattery =
 *         (Vsensor - Voffset) × scale
 *
 * Current values are provisional.
 */
#define CURRENT_SENSOR_SCALE           15.15f


/*
 * Zero-current sensor offset.
 *
 * For a bidirectional sensor this should normally be calibrated
 * from the actual sensor output at zero current.
 */
#define CURRENT_SENSOR_OFFSET_VOLTAGE   0.0f


/* ============================================================
 * MEASUREMENT PLAUSIBILITY LIMITS
 * ============================================================ */

/*
 * These limits determine whether a measurement is physically
 * plausible. They are deliberately different from protection
 * thresholds.
 */
#define BMS_VALID_VOLTAGE_MIN           0.0f
#define BMS_VALID_VOLTAGE_MAX          60.0f


#define BMS_VALID_CURRENT_MIN        -100.0f
#define BMS_VALID_CURRENT_MAX         100.0f


#define BMS_VALID_TEMPERATURE_MIN     -40.0f
#define BMS_VALID_TEMPERATURE_MAX     125.0f


/* ============================================================
 * TEMPERATURE CONFIGURATION
 * ============================================================ */

/*
 * Default temperature used before the first valid measurement.
 */
#define BMS_DEFAULT_TEMPERATURE        25.0f


/* ============================================================
 * SOC CONFIGURATION
 * ============================================================ */

/*
 * Allowed SOC range.
 */
#define BMS_SOC_MIN                     0.0f
#define BMS_SOC_MAX                   100.0f


/*
 * SOC update period.
 */
#define BMS_SOC_UPDATE_PERIOD_MS       1000U


/* ============================================================
 * SOH CONFIGURATION
 * ============================================================ */

/*
 * Allowed SOH range.
 */
#define BMS_SOH_MIN                     0.0f
#define BMS_SOH_MAX                   100.0f


/*
 * Initial SOH.
 *
 * This is a provisional reference value until measured battery
 * capacity / aging information is available.
 */
#define BMS_SOH_INITIAL               100.0f


/* ============================================================
 * FAULT TIMING
 * ============================================================ */

/*
 * Protection condition must remain active for this duration
 * before it becomes a confirmed fault.
 */
#define BMS_FAULT_DEBOUNCE_MS          100U


/*
 * Safety conditions must remain continuously valid for this
 * duration before a latched fault can be reset.
 */
#define BMS_FAULT_RECOVERY_MS          500U


/* ============================================================
 * BMS TASK PERIODS
 * ============================================================ */

/*
 * Measurements are updated every 100 ms.
 */
#define BMS_ADC_UPDATE_PERIOD_MS       100U

#define BMS_TEMP_UPDATE_PERIOD_MS      100U

#define BMS_FAULT_UPDATE_PERIOD_MS     100U


/*
 * CAN status transmission period.
 */
#define BMS_CAN_UPDATE_PERIOD_MS      1000U


/*
 * LCD refresh period.
 */
#define BMS_LCD_UPDATE_PERIOD_MS      500U


/* ============================================================
 * CAN IDENTIFIERS
 * ============================================================ */

/*
 * BMS -> CAN bus:
 * Status message.
 */
#define BMS_CAN_STATUS_ID             0x100U


/*
 * CAN bus -> BMS:
 * Command message.
 */
#define BMS_CAN_COMMAND_ID            0x200U


/* ============================================================
 * CAN COMMANDS
 * ============================================================ */

/*
 * Request immediate BMS status.
 */
#define BMS_CAN_CMD_STATUS_REQUEST    0x01U


/*
 * Request fault reset.
 */
#define BMS_CAN_CMD_FAULT_RESET       0x02U


/*
 * Enable BMS operation.
 */
#define BMS_CAN_CMD_ENABLE            0x03U


/*
 * Disable BMS operation.
 */
#define BMS_CAN_CMD_DISABLE           0x04U


/* ============================================================
 * BMS SYSTEM STATES
 * ============================================================ */

#define BMS_STATE_INIT                 0U
#define BMS_STATE_NORMAL               1U
#define BMS_STATE_WARNING              2U
#define BMS_STATE_FAULT                3U
#define BMS_STATE_SHUTDOWN             4U


/* ============================================================
 * LED CONFIGURATION
 * ============================================================ */

/*
 * Normal-operation LED.
 */
#define BMS_STATUS_LED_GPIO_PORT      GPIOB
#define BMS_STATUS_LED_PIN            GPIO_PIN_0


/*
 * Fault LED.
 */
#define BMS_FAULT_LED_GPIO_PORT       GPIOB
#define BMS_FAULT_LED_PIN             GPIO_PIN_1


/* ============================================================
 * LCD CONFIGURATION
 * ============================================================ */

/*
 * Common PCF8574 LCD backpack addresses:
 *
 *     0x27
 *     0x3F
 *
 * STM32 HAL expects the 8-bit shifted address, therefore:
 *
 *     0x27 << 1
 */
#define BMS_LCD_I2C_ADDRESS           (0x27U << 1)


/* ============================================================
 * DIAGNOSTIC CONFIGURATION
 * ============================================================ */

/*
 * Number of invalid measurements tolerated before a channel
 * should be considered persistently faulty.
 *
 * Reserved for future enhanced diagnostics.
 */
#define BMS_MAX_INVALID_MEASUREMENTS          3U


/*
 * Number of consecutive valid measurements required for
 * recovery of a previously invalid channel.
 *
 * Reserved for future enhanced diagnostics.
 */
#define BMS_REQUIRED_VALID_MEASUREMENTS       3U


#endif /* BMS_CONFIG_H */
