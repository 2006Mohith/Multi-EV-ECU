#ifndef BMS_CAN_H
#define BMS_CAN_H

#include <stdint.h>

/*
 * ============================================================
 * BMS CAN INTERFACE
 * ============================================================
 *
 * CAN STATUS MESSAGE
 * ------------------
 *
 * CAN ID: 0x100
 * DLC: 8 bytes
 *
 * Byte 0-1 : Battery voltage × 10
 * Byte 2-3 : Battery current × 10
 * Byte 4   : SOC (%)
 * Byte 5   : Temperature (°C)
 * Byte 6   : SOH (%)
 * Byte 7   : Fault status bitmask
 *
 *
 * CAN COMMAND MESSAGE
 * -------------------
 *
 * CAN ID: 0x200
 * DLC: 1 byte minimum
 *
 * Byte 0 = Command
 *
 * 0x01 -> Request BMS status
 * 0x02 -> Request fault reset
 * 0x03 -> Enable BMS
 * 0x04 -> Disable BMS
 *
 * Additional command data can be added later.
 */


/* ============================================================
 * CAN INITIALIZATION
 * ============================================================ */

/**
 * @brief Initialize and start BMS CAN.
 */
void BMS_CAN_Start(void);


/* ============================================================
 * CAN TRANSMISSION
 * ============================================================ */

/**
 * @brief Send BMS status message.
 */
void BMS_CAN_SendTestMessage(void);


/* ============================================================
 * CAN COMMAND PROCESSING
 * ============================================================ */

/**
 * @brief Process pending CAN commands.
 */
void BMS_CAN_ProcessCommands(void);


/**
 * @brief Check whether a CAN status message was requested.
 *
 * @return 1 if a request is pending, otherwise 0.
 */
uint8_t BMS_CAN_StatusRequestPending(void);


/**
 * @brief Clear the current CAN status request.
 */
void BMS_CAN_ClearStatusRequest(void);


/**
 * @brief Check whether a CAN fault-reset request is pending.
 *
 * @return 1 if a request is pending, otherwise 0.
 */
uint8_t BMS_CAN_FaultResetPending(void);


/**
 * @brief Clear the current CAN fault-reset request.
 */
void BMS_CAN_ClearFaultResetRequest(void);


/* ============================================================
 * CAN COMMUNICATION STATUS
 * ============================================================ */

/**
 * @brief Check whether CAN communication is running.
 *
 * @return 1 if CAN is started, otherwise 0.
 */
uint8_t BMS_CAN_IsRunning(void);

#endif /* BMS_CAN_H */
