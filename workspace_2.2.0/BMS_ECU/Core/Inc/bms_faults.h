#ifndef BMS_FAULTS_H
#define BMS_FAULTS_H

#include <stdint.h>

/*
 * ============================================================
 * BMS FAULT BIT DEFINITIONS
 * ============================================================
 *
 * One bit represents one fault condition.
 *
 * fault_status = 0
 *     -> No active faults
 *
 * Multiple faults can be active simultaneously.
 */

/* Battery voltage faults */
#define BMS_FAULT_OVERVOLTAGE          (1U << 0)
#define BMS_FAULT_UNDERVOLTAGE         (1U << 1)

/* Battery current fault */
#define BMS_FAULT_OVERCURRENT          (1U << 2)

/* Battery temperature faults */
#define BMS_FAULT_OVERTEMPERATURE      (1U << 3)
#define BMS_FAULT_UNDERTEMPERATURE     (1U << 4)

/* Sensor faults */
#define BMS_FAULT_VOLTAGE_SENSOR       (1U << 5)
#define BMS_FAULT_CURRENT_SENSOR       (1U << 6)
#define BMS_FAULT_TEMPERATURE_SENSOR   (1U << 7)


/*
 * ============================================================
 * BMS FAULT FUNCTIONS
 * ============================================================
 */

/**
 * @brief Initialize the fault management module.
 */
void BMS_Faults_Init(void);


/**
 * @brief Evaluate all BMS protection conditions.
 */
void BMS_Faults_Update(void);


/**
 * @brief Request clearing of a latched fault.
 *
 * The fault manager will only clear the latch when all
 * required safety conditions are satisfied.
 */
void BMS_Faults_RequestReset(void);


/**
 * @brief Check whether any fault is currently active.
 *
 * @return 1 if fault exists, otherwise 0.
 */
uint8_t BMS_Faults_IsActive(void);


/**
 * @brief Check whether a fault is latched.
 *
 * @return 1 if latched, otherwise 0.
 */
uint8_t BMS_Faults_IsLatched(void);

#endif /* BMS_FAULTS_H */
