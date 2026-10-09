#ifndef VCU_CAN_H
#define VCU_CAN_H

#include <stdint.h>

#include "vcu_data.h"

/* =========================================================
 * CAN INITIALIZATION
 * ========================================================= */

/*
 * Initializes the VCU CAN software layer and
 * activates CAN reception through FIFO0 interrupts.
 */
void VCU_CAN_Init(void);

/* =========================================================
 * CAN RECEIVE PROCESSING
 * ========================================================= */

/*
 * Processes one received CAN frame.
 *
 * This function is called by the CAN interrupt callback
 * after CubeMX/HAL receives a frame into FIFO0.
 */
void VCU_CAN_ProcessRx(
    uint32_t id,
    uint8_t *data,
    uint8_t dlc);

/* =========================================================
 * CAN TRANSMISSION
 * ========================================================= */

/*
 * Send the VCU status frame.
 */
uint8_t VCU_CAN_SendStatus(void);

/*
 * Send a command to the BMS.
 */
uint8_t VCU_CAN_SendBMSCommand(uint8_t command);

/*
 * Send a command to the motor controller.
 */
uint8_t VCU_CAN_SendMCUCommand(uint8_t command);

/* =========================================================
 * PERIODIC CAN TASK
 * ========================================================= */

/*
 * Execute periodic CAN communication tasks.
 *
 * This function is responsible for:
 *   - VCU status transmission
 *   - BMS command transmission
 *   - MCU command transmission
 *   - communication timeout supervision
 */
void VCU_CAN_Update(void);

/* =========================================================
 * COMMUNICATION STATUS
 * ========================================================= */

/*
 * Returns 1 when valid BMS communication is present.
 */
uint8_t VCU_CAN_IsBMSCommunicationValid(void);

/*
 * Returns 1 when valid MCU communication is present.
 */
uint8_t VCU_CAN_IsMCUCommunicationValid(void);

/*
 * Clear the BMS communication status.
 */
void VCU_CAN_ClearBMSCommunication(void);

/*
 * Clear the MCU communication status.
 */
void VCU_CAN_ClearMCUCommunication(void);

#endif /* VCU_CAN_H */
