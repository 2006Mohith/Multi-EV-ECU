#include "vcu_can.h"

#include "can.h"
#include "vcu_config.h"

/* =========================================================
 * PRIVATE VARIABLES
 * ========================================================= */


static uint8_t vcu_rx_data[VCU_CAN_STANDARD_DLC];
static uint8_t vcu_tx_data[VCU_CAN_STANDARD_DLC];

static CAN_TxHeaderTypeDef vcu_tx_header;
static uint32_t vcu_tx_mailbox;

/* =========================================================
 * PRIVATE HELPERS
 * ========================================================= */

static void VCU_CAN_PrepareTxHeader(uint32_t standard_id)
{
    vcu_tx_header.StdId = standard_id;
    vcu_tx_header.ExtId = 0U;
    vcu_tx_header.IDE = CAN_ID_STD;
    vcu_tx_header.RTR = CAN_RTR_DATA;
    vcu_tx_header.DLC = VCU_CAN_STANDARD_DLC;
    vcu_tx_header.TransmitGlobalTime = DISABLE;
}

static uint8_t VCU_CAN_Transmit(uint32_t standard_id,
                                const uint8_t *data)
{
    if (data == NULL)
    {
        return 0U;
    }

    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0U)
    {
        return 0U;
    }

    VCU_CAN_PrepareTxHeader(standard_id);

    if (HAL_CAN_AddTxMessage(&hcan,
                             &vcu_tx_header,
                             data,
                             &vcu_tx_mailbox) != HAL_OK)
    {
        return 0U;
    }

    return 1U;
}

/* =========================================================
 * CAN INITIALIZATION
 * ========================================================= */

void VCU_CAN_Init(void)
{
    CAN_FilterTypeDef filter_config;
    uint8_t i;

    for (i = 0U; i < VCU_CAN_STANDARD_DLC; i++)
    {
        vcu_rx_data[i] = 0U;
        vcu_tx_data[i] = 0U;
    }

    /*
     * Accept standard CAN frames into FIFO0.
     *
     * Software filtering in VCU_CAN_ProcessRx() selects:
     *   0x100 = BMS status
     *   0x301 = MCU status
     *
     * Additional standard frames are ignored.
     */
    filter_config.FilterBank = 0U;
    filter_config.FilterMode = CAN_FILTERMODE_IDMASK;
    filter_config.FilterScale = CAN_FILTERSCALE_32BIT;

    filter_config.FilterIdHigh = 0U;
    filter_config.FilterIdLow = 0U;

    filter_config.FilterMaskIdHigh = 0U;
    filter_config.FilterMaskIdLow = 0U;

    filter_config.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter_config.FilterActivation = ENABLE;
    filter_config.SlaveStartFilterBank = 14U;

    if (HAL_CAN_ConfigFilter(&hcan, &filter_config) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_CAN_Start(&hcan) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_CAN_ActivateNotification(
            &hcan,
            CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    {
        Error_Handler();
    }

    VCU_Data.bms.communication_valid = 0U;
    VCU_Data.mcu.communication_valid = 0U;

    VCU_Data.communication.bms_status_received = 0U;
    VCU_Data.communication.mcu_status_received = 0U;

    VCU_Data.communication.last_bms_rx_time_ms = 0U;
    VCU_Data.communication.last_mcu_rx_time_ms = 0U;
}

/* =========================================================
 * CAN RX PROCESSING
 * ========================================================= */

void VCU_CAN_ProcessRx(uint32_t id,
                       uint8_t *data,
                       uint8_t dlc)
{
    uint32_t current_tick;

    if (data == NULL)
    {
        return;
    }

    if (dlc > VCU_CAN_STANDARD_DLC)
    {
        dlc = VCU_CAN_STANDARD_DLC;
    }

    current_tick = HAL_GetTick();

    /*
     * -----------------------------------------------------
     * BMS STATUS — CAN ID 0x100
     * -----------------------------------------------------
     */

    if ((id == VCU_CAN_BMS_STATUS_ID) &&
        (dlc == VCU_CAN_STANDARD_DLC))
    {
        uint16_t voltage_raw;
        int16_t current_raw;

        voltage_raw =
            ((uint16_t)data[VCU_BMS_STATUS_VOLTAGE_MSB] << 8U) |
            ((uint16_t)data[VCU_BMS_STATUS_VOLTAGE_LSB]);

        current_raw =
            (int16_t)(
                ((uint16_t)data[VCU_BMS_STATUS_CURRENT_MSB] << 8U) |
                ((uint16_t)data[VCU_BMS_STATUS_CURRENT_LSB])
            );

        VCU_Data.bms.battery_voltage =
            (float)voltage_raw / 10.0f;

        VCU_Data.bms.battery_current =
            (float)current_raw / 10.0f;

        VCU_Data.bms.soc =
            data[VCU_BMS_STATUS_SOC_BYTE];

        VCU_Data.bms.temperature =
            (int8_t)data[VCU_BMS_STATUS_TEMPERATURE_BYTE];

        VCU_Data.bms.soh =
            data[VCU_BMS_STATUS_SOH_BYTE];

        VCU_Data.bms.fault_status =
            data[VCU_BMS_STATUS_FAULT_BYTE];

        VCU_Data.bms.communication_valid = 1U;
        VCU_Data.communication.bms_status_received = 1U;
        VCU_Data.communication.last_bms_rx_time_ms = current_tick;
        VCU_Data.bms.last_rx_time_ms = current_tick;

        return;
    }

    /*
     * -----------------------------------------------------
     * MCU STATUS — CAN ID 0x301
     * -----------------------------------------------------
     */

    if ((id == VCU_CAN_MCU_STATUS_ID) &&
        (dlc == VCU_CAN_STANDARD_DLC))
    {
        uint16_t speed_raw;
        int16_t current_raw;

        speed_raw =
            ((uint16_t)data[VCU_MCU_STATUS_SPEED_MSB] << 8U) |
            ((uint16_t)data[VCU_MCU_STATUS_SPEED_LSB]);

        current_raw =
            (int16_t)(
                ((uint16_t)data[VCU_MCU_STATUS_CURRENT_MSB] << 8U) |
                ((uint16_t)data[VCU_MCU_STATUS_CURRENT_LSB])
            );

        VCU_Data.mcu.motor_speed = speed_raw;
        VCU_Data.mcu.motor_current = current_raw;

        VCU_Data.mcu.controller_temperature =
            (int8_t)data[VCU_MCU_STATUS_TEMPERATURE_BYTE];

        VCU_Data.mcu.enable_state =
            data[VCU_MCU_STATUS_ENABLE_BYTE];

        VCU_Data.mcu.fault_status =
            data[VCU_MCU_STATUS_FAULT_BYTE];

        VCU_Data.mcu.communication_valid = 1U;
        VCU_Data.communication.mcu_status_received = 1U;
        VCU_Data.communication.last_mcu_rx_time_ms = current_tick;
        VCU_Data.mcu.last_rx_time_ms = current_tick;

        return;
    }
}

/* =========================================================
 * SEND VCU STATUS
 * ========================================================= */

uint8_t VCU_CAN_SendStatus(void)
{
    uint8_t i;

    for (i = 0U; i < VCU_CAN_STANDARD_DLC; i++)
    {
        vcu_tx_data[i] = 0U;
    }

    vcu_tx_data[VCU_STATUS_STATE_BYTE] =
        (uint8_t)VCU_Data.system_state;

    vcu_tx_data[VCU_STATUS_DIRECTION_BYTE] =
        (uint8_t)VCU_Data.input.direction;

    vcu_tx_data[VCU_STATUS_THROTTLE_BYTE] =
        (uint8_t)VCU_Data.input.analog.throttle_percent;

    vcu_tx_data[VCU_STATUS_BRAKE_BYTE] =
        (uint8_t)VCU_Data.input.analog.brake_percent;

    vcu_tx_data[VCU_STATUS_FAULT_BYTE] =
        VCU_Data.faults.code;

    vcu_tx_data[VCU_STATUS_BMS_COMM_BYTE] =
        VCU_Data.bms.communication_valid;

    vcu_tx_data[VCU_STATUS_MCU_COMM_BYTE] =
        VCU_Data.mcu.communication_valid;

    vcu_tx_data[VCU_STATUS_ENABLE_BYTE] =
        VCU_Data.system_enabled;

    return VCU_CAN_Transmit(
        VCU_CAN_STATUS_ID,
        vcu_tx_data);
}

/* =========================================================
 * SEND BMS COMMAND
 * ========================================================= */

uint8_t VCU_CAN_SendBMSCommand(uint8_t command)
{
    uint8_t i;

    for (i = 0U; i < VCU_CAN_STANDARD_DLC; i++)
    {
        vcu_tx_data[i] = 0U;
    }

    vcu_tx_data[0U] = command;

    return VCU_CAN_Transmit(
        VCU_CAN_BMS_COMMAND_ID,
        vcu_tx_data);
}

/* =========================================================
 * SEND MCU COMMAND
 * ========================================================= */

uint8_t VCU_CAN_SendMCUCommand(uint8_t command)
{
    uint8_t i;

    for (i = 0U; i < VCU_CAN_STANDARD_DLC; i++)
    {
        vcu_tx_data[i] = 0U;
    }

    vcu_tx_data[0U] = command;

    return VCU_CAN_Transmit(
        VCU_CAN_MCU_COMMAND_ID,
        vcu_tx_data);
}

/* =========================================================
 * PERIODIC CAN UPDATE
 * ========================================================= */

void VCU_CAN_Update(void)
{
    uint32_t current_tick;

    current_tick = HAL_GetTick();

    /*
     * BMS communication timeout.
     */
    if ((VCU_Data.bms.communication_valid != 0U) &&
        ((current_tick -
          VCU_Data.communication.last_bms_rx_time_ms) >
         VCU_BMS_STATUS_TIMEOUT_MS))
    {
        VCU_Data.bms.communication_valid = 0U;
        VCU_Data.communication.bms_status_received = 0U;
    }

    /*
     * MCU communication timeout.
     */
    if ((VCU_Data.mcu.communication_valid != 0U) &&
        ((current_tick -
          VCU_Data.communication.last_mcu_rx_time_ms) >
         VCU_MCU_STATUS_TIMEOUT_MS))
    {
        VCU_Data.mcu.communication_valid = 0U;
        VCU_Data.communication.mcu_status_received = 0U;
    }
}

/* =========================================================
 * BMS COMMUNICATION STATUS
 * ========================================================= */

uint8_t VCU_CAN_IsBMSCommunicationValid(void)
{
    return VCU_Data.bms.communication_valid;
}

/* =========================================================
 * MCU COMMUNICATION STATUS
 * ========================================================= */

uint8_t VCU_CAN_IsMCUCommunicationValid(void)
{
    return VCU_Data.mcu.communication_valid;
}

/* =========================================================
 * CLEAR BMS COMMUNICATION
 * ========================================================= */

void VCU_CAN_ClearBMSCommunication(void)
{
    VCU_Data.bms.communication_valid = 0U;
    VCU_Data.communication.bms_status_received = 0U;
    VCU_Data.communication.last_bms_rx_time_ms = 0U;
    VCU_Data.bms.last_rx_time_ms = 0U;
}

/* =========================================================
 * CLEAR MCU COMMUNICATION
 * ========================================================= */

void VCU_CAN_ClearMCUCommunication(void)
{
    VCU_Data.mcu.communication_valid = 0U;
    VCU_Data.communication.mcu_status_received = 0U;
    VCU_Data.communication.last_mcu_rx_time_ms = 0U;
    VCU_Data.mcu.last_rx_time_ms = 0U;
}

/* =========================================================
 * HAL CAN RX FIFO0 CALLBACK
 * ========================================================= */

void HAL_CAN_RxFifo0MsgPendingCallback(
    CAN_HandleTypeDef *hcan_callback)
{
    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[VCU_CAN_STANDARD_DLC];

    if (hcan_callback != &hcan)
    {
        return;
    }

    if (HAL_CAN_GetRxMessage(
            hcan_callback,
            CAN_RX_FIFO0,
            &rx_header,
            rx_data) != HAL_OK)
    {
        return;
    }

    /*
     * The VCU application uses standard 11-bit CAN IDs.
     */
    if (rx_header.IDE != CAN_ID_STD)
    {
        return;
    }

    VCU_CAN_ProcessRx(
        rx_header.StdId,
        rx_data,
        (uint8_t)rx_header.DLC);
}
