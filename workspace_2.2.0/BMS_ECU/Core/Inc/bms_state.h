#ifndef BMS_STATE_H
#define BMS_STATE_H

#include <stdint.h>

/*
 * ============================================================
 * BMS STATE MACHINE MODULE
 * ============================================================
 *
 * System states:
 *
 *   INIT
 *      |
 *      v
 *   NORMAL <-------> WARNING
 *      |
 *      v
 *    FAULT
 *      |
 *      v
 *  SHUTDOWN
 *
 * The state machine is responsible for deciding the overall
 * operating state of the BMS from:
 *
 *   - Measurement validity
 *   - Fault status
 *   - Fault latch
 *   - Enable command
 *   - Disable command
 *
 * Protection decisions themselves remain in bms_faults.c.
 */


/* ============================================================
 * INITIALIZATION
 * ============================================================ */

/**
 * @brief Initialize the BMS state machine.
 */
void BMS_State_Init(void);


/* ============================================================
 * STATE UPDATE
 * ============================================================ */

/**
 * @brief Evaluate and update the BMS system state.
 */
void BMS_State_Update(void);


/* ============================================================
 * STATE CONTROL
 * ============================================================ */

/**
 * @brief Request BMS enable.
 */
void BMS_State_RequestEnable(void);


/**
 * @brief Request BMS disable.
 */
void BMS_State_RequestDisable(void);


/**
 * @brief Request BMS shutdown.
 */
void BMS_State_RequestShutdown(void);


/**
 * @brief Request recovery from shutdown.
 *
 * Recovery is only allowed if the required safety conditions
 * are satisfied.
 */
void BMS_State_RequestRecovery(void);


/* ============================================================
 * STATE QUERY
 * ============================================================ */

/**
 * @brief Get current BMS system state.
 *
 * @return Current BMS_SystemState_t value.
 */
uint8_t BMS_State_Get(void);


/**
 * @brief Check whether BMS is operating normally.
 *
 * @return 1 if NORMAL, otherwise 0.
 */
uint8_t BMS_State_IsNormal(void);


/**
 * @brief Check whether BMS is in a fault state.
 *
 * @return 1 if FAULT or fault-latched, otherwise 0.
 */
uint8_t BMS_State_IsFault(void);


/**
 * @brief Check whether BMS is shut down.
 *
 * @return 1 if SHUTDOWN, otherwise 0.
 */
uint8_t BMS_State_IsShutdown(void);

#endif /* BMS_STATE_H */
