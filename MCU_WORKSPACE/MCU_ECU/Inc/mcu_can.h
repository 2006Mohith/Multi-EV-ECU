#ifndef MCU_CAN_H
#define MCU_CAN_H

#include <stdint.h>
#include "main.h"
#include "can.h"
#include "mcu_config.h"
#include "mcu_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * MCU CAN Communication Module
 *
 * Hardware:
 * PA11 -> CAN RX -> CAN transceiver CRX/RXD
 * PA12 -> CAN TX -> CAN transceiver CTX/TXD
 *
 * CAN bus:
 * 500 kbit/s
 *
 * CAN IDs:
 * 0x300 -> VCU command to MCU
 * 0x301 -> MCU status to VCU
 * 0x310 -> MCU heartbeat
 * 0x320 -> MCU diagnostics
 * ========================================================================== */

/* ============================================================================
 * Initialization
 * ========================================================================== */

/**
 * @brief Initialize MCU CAN communication.
 *
 * Configures the CAN filter, starts CAN, and enables RX FIFO0 interrupt.
 */
void MCU_CAN_Init(void);

/* ============================================================================
 * Periodic CAN processing
 * ========================================================================== */

/**
 * @brief Periodically supervise CAN communication and timeouts.
 */
void MCU_CAN_Update(void);

/* ============================================================================
 * CAN receive processing
 * ========================================================================== */

/**
 * @brief Process a received CAN frame.
 *
 * This function checks the received CAN identifier and dispatches
 * the frame to the appropriate processing routine.
 *
 * @param rx_header Received CAN header.
 * @param rx_data   Received CAN data bytes.
 */
void MCU_CAN_ProcessRx(const CAN_RxHeaderTypeDef *rx_header,
                       const uint8_t *rx_data);

/**
 * @brief HAL CAN FIFO0 receive callback interface.
 *
 * This function is called from:
 * HAL_CAN_RxFifo0MsgPendingCallback()
 */
void MCU_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan);

/* ============================================================================
 * VCU command processing
 * ========================================================================== */

/**
 * @brief Process a command received from the VCU.
 *
 * CAN ID: 0x300
 *
 * Frame format:
 * Byte 0 : Command
 * Byte 1 : Direction
 * Byte 2 : PWM / motor demand %
 * Byte 3 : Enable
 * Byte 4 : Requested state
 * Byte 5 : Command counter
 * Byte 6 : Reserved
 * Byte 7 : Reserved
 *
 * @param data Received command data.
 */
void MCU_CAN_ProcessVCUCommand(const uint8_t *data);

/* ============================================================================
 * CAN transmit functions
 * ========================================================================== */

/**
 * @brief Send MCU status to the VCU.
 *
 * CAN ID: 0x301
 */
void MCU_CAN_SendStatus(void);

/**
 * @brief Send MCU heartbeat to the VCU.
 *
 * CAN ID: 0x310
 */
void MCU_CAN_SendHeartbeat(void);

/**
 * @brief Send MCU diagnostic information.
 *
 * CAN ID: 0x320
 */
void MCU_CAN_SendDiagnostics(void);

/* ============================================================================
 * CAN status / communication supervision
 * ========================================================================== */

/**
 * @brief Check whether communication with the VCU is currently valid.
 *
 * @return 1 if communication is valid, otherwise 0.
 */
uint8_t MCU_CAN_IsVCUCommunicationValid(void);

/**
 * @brief Return the number of CAN communication timeouts.
 *
 * @return Timeout counter.
 */
uint32_t MCU_CAN_GetTimeoutCount(void);

/**
 * @brief Return the current CAN heartbeat counter.
 *
 * @return Transmit heartbeat counter.
 */
uint8_t MCU_CAN_GetHeartbeatCounter(void);

/* ============================================================================
 * CAN command status
 * ========================================================================== */

/**
 * @brief Return whether a valid VCU command has been received.
 *
 * @return 1 if valid command exists, otherwise 0.
 */
uint8_t MCU_CAN_IsCommandValid(void);

/**
 * @brief Get the last received VCU command counter.
 *
 * @return Command counter.
 */
uint8_t MCU_CAN_GetCommandCounter(void);

/* ============================================================================
 * CAN error handling
 * ========================================================================== */

/**
 * @brief Record a CAN error condition.
 */
void MCU_CAN_SetError(void);

/**
 * @brief Clear the CAN error condition.
 */
void MCU_CAN_ClearError(void);

/**
 * @brief Check whether a CAN error is active.
 *
 * @return 1 if CAN error is active, otherwise 0.
 */
uint8_t MCU_CAN_IsErrorActive(void);

#ifdef __cplusplus
}
#endif

#endif /* MCU_CAN_H */
