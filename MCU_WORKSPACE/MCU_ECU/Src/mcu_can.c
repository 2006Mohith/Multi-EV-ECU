#include "main.h"
#include "can.h"
#include "mcu_config.h"
#include "mcu_data.h"
#include "mcu_can.h"

/* ============================================================================
 * MCU CAN Communication Module
 *
 * Physical interface:
 * PA12 -> CAN transceiver CTX / TXD
 * PA11 <- CAN transceiver CRX / RXD
 *
 * CAN:
 * 500 kbit/s
 *
 * IDs:
 * 0x300 -> VCU command
 * 0x301 -> MCU status
 * 0x310 -> MCU heartbeat
 * 0x320 -> MCU diagnostics
 * ========================================================================== */

/* ============================================================================
 * Private variables
 * ========================================================================== */

static CAN_FilterTypeDef mcu_filter_config;

static CAN_RxHeaderTypeDef mcu_rx_header;
static uint8_t mcu_rx_data[MCU_CAN_STANDARD_DLC];

static CAN_TxHeaderTypeDef mcu_tx_header;
static uint8_t mcu_tx_data[MCU_CAN_STANDARD_DLC];

static uint32_t mcu_tx_mailbox;

/* ============================================================================
 * Private helper
 * ========================================================================== */

static void MCU_CAN_PrepareTxHeader(uint32_t std_id)
{
    mcu_tx_header.StdId = std_id;
    mcu_tx_header.ExtId = 0U;
    mcu_tx_header.IDE = CAN_ID_STD;
    mcu_tx_header.RTR = CAN_RTR_DATA;
    mcu_tx_header.DLC = MCU_CAN_STANDARD_DLC;
    mcu_tx_header.TransmitGlobalTime = DISABLE;
}

/* ============================================================================
 * Initialization
 * ========================================================================== */

void MCU_CAN_Init(void)
{
    /*
     * Accept all standard CAN frames into FIFO0.
     * The application performs ID-based software filtering.
     */
    mcu_filter_config.FilterBank = 0U;
    mcu_filter_config.FilterMode = CAN_FILTERMODE_IDMASK;
    mcu_filter_config.FilterScale = CAN_FILTERSCALE_32BIT;

    mcu_filter_config.FilterIdHigh = 0U;
    mcu_filter_config.FilterIdLow = 0U;

    mcu_filter_config.FilterMaskIdHigh = 0U;
    mcu_filter_config.FilterMaskIdLow = 0U;

    mcu_filter_config.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    mcu_filter_config.FilterActivation = ENABLE;
    mcu_filter_config.SlaveStartFilterBank = 14U;

    if (HAL_CAN_ConfigFilter(&hcan, &mcu_filter_config) != HAL_OK)
    {
        MCU_CAN_SetError();
        Error_Handler();
    }

    if (HAL_CAN_Start(&hcan) != HAL_OK)
    {
        MCU_CAN_SetError();
        Error_Handler();
    }

    if (HAL_CAN_ActivateNotification(
            &hcan,
            CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    {
        MCU_CAN_SetError();
        Error_Handler();
    }

    /* Initialize VCU communication state */
    MCU_Data.vcu_comm.communication_valid = MCU_FALSE;
    MCU_Data.vcu_comm.command_received = MCU_FALSE;
    MCU_Data.vcu_comm.last_command_time = HAL_GetTick();
    MCU_Data.vcu_comm.last_status_time = 0U;
    MCU_Data.vcu_comm.timeout_count = 0U;
    MCU_Data.vcu_comm.last_command_counter = 0U;
    MCU_Data.vcu_comm.heartbeat_valid = MCU_FALSE;

    /* Initialize heartbeat state */
    MCU_Data.can_heartbeat.tx_counter = 0U;
    MCU_Data.can_heartbeat.rx_counter = 0U;
    MCU_Data.can_heartbeat.vcu_heartbeat_received = MCU_FALSE;
    MCU_Data.can_heartbeat.vcu_heartbeat_counter = 0U;
    MCU_Data.can_heartbeat.last_heartbeat_time = 0U;
    MCU_Data.can_heartbeat.valid = MCU_FALSE;

    MCU_Data.fault.can_fault = MCU_FALSE;
    MCU_Data.fault.vcu_comm_fault = MCU_FALSE;

    MCU_Data.task.can_ok = MCU_TRUE;
}

/* ============================================================================
 * CAN periodic supervision
 * ========================================================================== */

void MCU_CAN_Update(void)
{
    uint32_t now;
    uint32_t command_elapsed;

    now = HAL_GetTick();

    command_elapsed =
        now - MCU_Data.vcu_comm.last_command_time;

    /*
     * VCU command timeout is currently the primary communication
     * supervision mechanism.
     */
    if (command_elapsed > MCU_CAN_COMMAND_TIMEOUT_MS)
    {
        MCU_Data.vcu_comm.communication_valid = MCU_FALSE;
        MCU_Data.vcu_comm.command_received = MCU_FALSE;

        MCU_Data.fault.vcu_comm_fault = MCU_TRUE;

        if (MCU_Data.vcu_comm.timeout_count < 0xFFFFFFFFUL)
        {
            MCU_Data.vcu_comm.timeout_count++;
        }
    }
    else
    {
        MCU_Data.vcu_comm.communication_valid = MCU_TRUE;
        MCU_Data.fault.vcu_comm_fault = MCU_FALSE;
    }

    /*
     * VCU heartbeat reception is not yet implemented in the current
     * VCU firmware protocol, so leave this flag invalid until that
     * protocol is added.
     */
    MCU_Data.can_heartbeat.valid = MCU_FALSE;
    MCU_Data.vcu_comm.heartbeat_valid = MCU_FALSE;
}

/* ============================================================================
 * CAN receive processing
 * ========================================================================== */

void MCU_CAN_ProcessRx(const CAN_RxHeaderTypeDef *rx_header,
                       const uint8_t *rx_data)
{
    if ((rx_header == NULL) || (rx_data == NULL))
    {
        return;
    }

    /*
     * The current MCU protocol uses standard 8-byte data frames.
     */
    if ((rx_header->IDE != CAN_ID_STD) ||
        (rx_header->RTR != CAN_RTR_DATA) ||
        (rx_header->DLC != MCU_CAN_STANDARD_DLC))
    {
        MCU_Data.fault.can_fault = MCU_TRUE;
        return;
    }

    switch (rx_header->StdId)
    {
        case MCU_CAN_ID_VCU_COMMAND:

            MCU_CAN_ProcessVCUCommand(rx_data);
            break;

        case MCU_CAN_ID_MCU_HEARTBEAT:

            /*
             * Ignore our own heartbeat if it appears on the bus.
             */
            break;

        default:

            /*
             * Other CAN frames belong to other ECUs or future
             * protocol extensions. Unknown frames are ignored.
             */
            break;
    }
}

/* ============================================================================
 * HAL CAN FIFO0 receive callback
 * ========================================================================== */

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan_handle)
{
    if ((hcan_handle == NULL) ||
        (hcan_handle->Instance != CAN1))
    {
        return;
    }

    if (HAL_CAN_GetRxMessage(
            hcan_handle,
            CAN_RX_FIFO0,
            &mcu_rx_header,
            mcu_rx_data) != HAL_OK)
    {
        MCU_CAN_SetError();
        return;
    }

    MCU_CAN_ProcessRx(&mcu_rx_header, mcu_rx_data);
}

/* ============================================================================
 * VCU command processing
 * ========================================================================== */

void MCU_CAN_ProcessVCUCommand(const uint8_t *data)
{
    uint8_t command;
    uint8_t direction;
    uint8_t pwm_percent;
    uint8_t enable;
    uint8_t requested_state;
    uint8_t command_counter;

    if (data == NULL)
    {
        return;
    }

    command = data[MCU_COMMAND_BYTE];
    direction = data[MCU_COMMAND_DIRECTION_BYTE];
    pwm_percent = data[MCU_COMMAND_PWM_BYTE];
    enable = data[MCU_COMMAND_ENABLE_BYTE];
    requested_state = data[MCU_COMMAND_STATE_BYTE];
    command_counter = data[MCU_COMMAND_COUNTER_BYTE];

    /* ------------------------------------------------------------------------
     * Direction validation
     * ---------------------------------------------------------------------- */

    if ((direction != MCU_DIRECTION_NEUTRAL) &&
        (direction != MCU_DIRECTION_FORWARD) &&
        (direction != MCU_DIRECTION_REVERSE))
    {
        MCU_Data.fault.command_fault = MCU_TRUE;
        MCU_Data.motor_command.valid = MCU_FALSE;
        return;
    }

    /* ------------------------------------------------------------------------
     * PWM validation
     * ---------------------------------------------------------------------- */

    if (pwm_percent > MCU_PWM_MAX_PERCENT)
    {
        MCU_Data.fault.command_fault = MCU_TRUE;
        MCU_Data.motor_command.valid = MCU_FALSE;
        return;
    }

    /* ------------------------------------------------------------------------
     * Enable validation
     * ---------------------------------------------------------------------- */

    if ((enable != MCU_DISABLED) &&
        (enable != MCU_ENABLED))
    {
        MCU_Data.fault.command_fault = MCU_TRUE;
        MCU_Data.motor_command.valid = MCU_FALSE;
        return;
    }

    /* ------------------------------------------------------------------------
     * Store received command
     * ---------------------------------------------------------------------- */

    MCU_Data.motor_command.command = command;
    MCU_Data.motor_command.direction = direction;
    MCU_Data.motor_command.pwm_percent = pwm_percent;
    MCU_Data.motor_command.enable = enable;
    MCU_Data.motor_command.requested_state = requested_state;
    MCU_Data.motor_command.command_counter = command_counter;

    /* ------------------------------------------------------------------------
     * Process command type
     * ---------------------------------------------------------------------- */

    switch (command)
    {
        case MCU_COMMAND_RUN:
        case MCU_COMMAND_ENABLE:

            MCU_Data.motor_command.valid = MCU_TRUE;
            break;

        case MCU_COMMAND_STOP:
        case MCU_COMMAND_BRAKE:
        case MCU_COMMAND_DISABLE:

            MCU_Data.motor_command.enable = MCU_DISABLED;
            MCU_Data.motor_command.pwm_percent = 0U;
            MCU_Data.motor_command.direction = MCU_DIRECTION_NEUTRAL;
            MCU_Data.motor_command.valid = MCU_TRUE;

            break;

        case MCU_COMMAND_RESET_FAULT:

            MCU_Data.fault.reset_requested = MCU_TRUE;

            MCU_Data.motor_command.enable = MCU_DISABLED;
            MCU_Data.motor_command.pwm_percent = 0U;
            MCU_Data.motor_command.direction = MCU_DIRECTION_NEUTRAL;
            MCU_Data.motor_command.valid = MCU_TRUE;

            break;

        case MCU_COMMAND_NONE:

            MCU_Data.motor_command.valid = MCU_TRUE;
            break;

        default:

            MCU_Data.fault.command_fault = MCU_TRUE;
            MCU_Data.motor_command.valid = MCU_FALSE;
            return;
    }

    /* ------------------------------------------------------------------------
     * Communication supervision data
     * ---------------------------------------------------------------------- */

    MCU_Data.vcu_comm.communication_valid = MCU_TRUE;
    MCU_Data.vcu_comm.command_received = MCU_TRUE;
    MCU_Data.vcu_comm.last_command_time = HAL_GetTick();
    MCU_Data.vcu_comm.last_status_time = HAL_GetTick();
    MCU_Data.vcu_comm.last_command_counter = command_counter;

    MCU_Data.fault.command_fault = MCU_FALSE;
    MCU_Data.fault.vcu_comm_fault = MCU_FALSE;
}

/* ============================================================================
 * Send MCU status
 * ========================================================================== */

void MCU_CAN_SendStatus(void)
{
    uint16_t rpm;
    uint16_t current_x10;
    uint8_t temperature;

    if (MCU_Data.hall.rpm > 65535U)
    {
        rpm = 65535U;
    }
    else
    {
        rpm = (uint16_t)MCU_Data.hall.rpm;
    }

    /*
     * Current and temperature sensors are not present in the current
     * hardware configuration, so these values remain zero.
     */
    current_x10 = 0U;
    temperature = 0U;

    MCU_CAN_PrepareTxHeader(MCU_CAN_ID_MCU_STATUS);

    mcu_tx_data[MCU_STATUS_BYTE_RPM_HIGH] =
        (uint8_t)((rpm >> 8) & 0xFFU);

    mcu_tx_data[MCU_STATUS_BYTE_RPM_LOW] =
        (uint8_t)(rpm & 0xFFU);

    mcu_tx_data[MCU_STATUS_BYTE_CURRENT_HIGH] =
        (uint8_t)((current_x10 >> 8) & 0xFFU);

    mcu_tx_data[MCU_STATUS_BYTE_CURRENT_LOW] =
        (uint8_t)(current_x10 & 0xFFU);

    mcu_tx_data[MCU_STATUS_BYTE_TEMPERATURE] =
        temperature;

    mcu_tx_data[MCU_STATUS_BYTE_ENABLE] =
        MCU_Data.motor.enabled;

    mcu_tx_data[MCU_STATUS_BYTE_FAULT] =
        MCU_Data.fault.code;

    mcu_tx_data[MCU_STATUS_BYTE_DIRECTION] =
        MCU_Data.motor.direction;

    if (HAL_CAN_AddTxMessage(
            &hcan,
            &mcu_tx_header,
            mcu_tx_data,
            &mcu_tx_mailbox) != HAL_OK)
    {
        MCU_CAN_SetError();
    }
}

/* ============================================================================
 * Send MCU heartbeat
 * ========================================================================== */

void MCU_CAN_SendHeartbeat(void)
{
    MCU_CAN_PrepareTxHeader(MCU_CAN_ID_MCU_HEARTBEAT);

    MCU_Data.can_heartbeat.tx_counter++;

    mcu_tx_data[MCU_HEARTBEAT_BYTE_COUNTER] =
        MCU_Data.can_heartbeat.tx_counter;

    mcu_tx_data[MCU_HEARTBEAT_BYTE_STATE] =
        MCU_Data.motor.state;

    mcu_tx_data[MCU_HEARTBEAT_BYTE_FAULT] =
        MCU_Data.fault.code;

    mcu_tx_data[MCU_HEARTBEAT_BYTE_ENABLE] =
        MCU_Data.motor.enabled;

    mcu_tx_data[4] = 0U;
    mcu_tx_data[5] = 0U;
    mcu_tx_data[6] = 0U;
    mcu_tx_data[7] = 0U;

    if (HAL_CAN_AddTxMessage(
            &hcan,
            &mcu_tx_header,
            mcu_tx_data,
            &mcu_tx_mailbox) != HAL_OK)
    {
        MCU_CAN_SetError();
    }
}

/* ============================================================================
 * Send diagnostic frame
 * ========================================================================== */

void MCU_CAN_SendDiagnostics(void)
{
    MCU_CAN_PrepareTxHeader(MCU_CAN_ID_MCU_DIAGNOSTIC);

    mcu_tx_data[0] = MCU_Data.fault.code;
    mcu_tx_data[1] = MCU_Data.fault.severity;
    mcu_tx_data[2] = MCU_Data.fault.latched;
    mcu_tx_data[3] = MCU_Data.hall.signal_valid;
    mcu_tx_data[4] = MCU_Data.hall.timeout;
    mcu_tx_data[5] = MCU_Data.vcu_comm.communication_valid;
    mcu_tx_data[6] = MCU_Data.can_heartbeat.valid;
    mcu_tx_data[7] = MCU_Data.motor.enabled;

    if (HAL_CAN_AddTxMessage(
            &hcan,
            &mcu_tx_header,
            mcu_tx_data,
            &mcu_tx_mailbox) != HAL_OK)
    {
        MCU_CAN_SetError();
    }
}

/* ============================================================================
 * Communication status
 * ========================================================================== */

uint8_t MCU_CAN_IsVCUCommunicationValid(void)
{
    return MCU_Data.vcu_comm.communication_valid;
}

/* ============================================================================
 * Timeout counter
 * ========================================================================== */

uint32_t MCU_CAN_GetTimeoutCount(void)
{
    return MCU_Data.vcu_comm.timeout_count;
}

/* ============================================================================
 * Heartbeat counter
 * ========================================================================== */

uint8_t MCU_CAN_GetHeartbeatCounter(void)
{
    return MCU_Data.can_heartbeat.tx_counter;
}

/* ============================================================================
 * Command validity
 * ========================================================================== */

uint8_t MCU_CAN_IsCommandValid(void)
{
    return MCU_Data.motor_command.valid;
}

/* ============================================================================
 * Command counter
 * ========================================================================== */

uint8_t MCU_CAN_GetCommandCounter(void)
{
    return MCU_Data.vcu_comm.last_command_counter;
}

/* ============================================================================
 * CAN error
 * ========================================================================== */

void MCU_CAN_SetError(void)
{
    MCU_Data.fault.can_fault = MCU_TRUE;
    MCU_Data.task.can_ok = MCU_FALSE;
}

/* ============================================================================
 * CAN error clear
 * ========================================================================== */

void MCU_CAN_ClearError(void)
{
    MCU_Data.fault.can_fault = MCU_FALSE;
    MCU_Data.task.can_ok = MCU_TRUE;
}

/* ============================================================================
 * CAN error status
 * ========================================================================== */

uint8_t MCU_CAN_IsErrorActive(void)
{
    return MCU_Data.fault.can_fault;
}
