/*
 * bms_state.c
 *
 * BMS system state-machine implementation.
 *
 * States:
 *
 *              +----------------+
 *              |      INIT      |
 *              +-------+--------+
 *                      |
 *                measurements valid
 *                      |
 *                      v
 *              +-------+--------+
 *              |    WARNING     |
 *              |   (disabled)   |
 *              +---+---------+--+
 *                  |         |
 *            enable        shutdown
 *                  |         |
 *                  v         v
 *            +-----+----+  +--+--------+
 *            |  NORMAL  |  | SHUTDOWN  |
 *            +----+-----+  +-----------+
 *                 |
 *              fault
 *                 |
 *                 v
 *            +----+-----+
 *            |  FAULT   |
 *            +----+-----+
 *                 |
 *          fault cleared +
 *          recovery request
 *                 |
 *                 v
 *              WARNING
 *
 * Protection decisions are provided by bms_faults.c.
 */

#include "main.h"
#include "bms_state.h"
#include "bms_data.h"
#include "bms_faults.h"
#include "bms_config.h"

/* -------------------------------------------------------------------------- */
/* Private variables                                                          */
/* -------------------------------------------------------------------------- */

static uint8_t enable_request = 0U;
static uint8_t disable_request = 0U;
static uint8_t shutdown_request = 0U;
static uint8_t recovery_request = 0U;

static uint32_t state_entry_time = 0U;

/* -------------------------------------------------------------------------- */
/* Private functions                                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief Check whether all required measurements are valid.
 */
static uint8_t BMS_State_MeasurementsValid(void)
{
    if (BMS_Data.voltage_valid != BMS_MEASUREMENT_VALID)
    {
        return 0U;
    }

    if (BMS_Data.current_valid != BMS_MEASUREMENT_VALID)
    {
        return 0U;
    }

    if (BMS_Data.temperature_valid != BMS_MEASUREMENT_VALID)
    {
        return 0U;
    }

    return 1U;
}

/**
 * @brief Check whether the BMS is safe to enable.
 */
static uint8_t BMS_State_SafetyOK(void)
{
    if (BMS_Data.fault_status != 0U)
    {
        return 0U;
    }

    if (BMS_Data.fault_latched != 0U)
    {
        return 0U;
    }

    if (BMS_State_MeasurementsValid() == 0U)
    {
        return 0U;
    }

    return 1U;
}

/**
 * @brief Change state and record state-entry time.
 */
static void BMS_State_Set(BMS_SystemState_t new_state)
{
    if (BMS_Data.system_state != new_state)
    {
        BMS_Data.system_state = new_state;
        state_entry_time = HAL_GetTick();
    }
}

/**
 * @brief Clear all pending state requests.
 */
static void BMS_State_ClearRequests(void)
{
    enable_request = 0U;
    disable_request = 0U;
    shutdown_request = 0U;
    recovery_request = 0U;
}

/**
 * @brief Handle INIT state.
 */
static void BMS_State_HandleInit(void)
{
    /*
     * Fault has highest priority.
     */
    if ((BMS_Data.fault_status != 0U) ||
        (BMS_Data.fault_latched != 0U))
    {
        BMS_Data.system_enabled = 0U;

        BMS_State_ClearRequests();

        BMS_State_Set(BMS_SYSTEM_FAULT);

        return;
    }

    /*
     * Required measurements must be valid before
     * leaving initialization.
     */
    if (BMS_State_MeasurementsValid() == 0U)
    {
        BMS_Data.system_enabled = 0U;

        return;
    }

    /*
     * Shutdown request has priority over enable.
     */
    if (shutdown_request != 0U)
    {
        BMS_Data.system_enabled = 0U;

        BMS_State_ClearRequests();

        BMS_State_Set(BMS_SYSTEM_SHUTDOWN);

        return;
    }

    /*
     * Enable only through an explicit request.
     */
    if (enable_request != 0U)
    {
        enable_request = 0U;
        disable_request = 0U;
        recovery_request = 0U;

        if (BMS_State_SafetyOK() != 0U)
        {
            BMS_Data.system_enabled = 1U;

            BMS_State_Set(BMS_SYSTEM_NORMAL);

            return;
        }
    }

    /*
     * Measurements are valid but BMS is not enabled.
     */
    BMS_Data.system_enabled = 0U;

    BMS_State_Set(BMS_SYSTEM_WARNING);
}

/**
 * @brief Handle WARNING / disabled state.
 */
static void BMS_State_HandleWarning(void)
{
    /*
     * Fault always has priority.
     */
    if ((BMS_Data.fault_status != 0U) ||
        (BMS_Data.fault_latched != 0U))
    {
        BMS_Data.system_enabled = 0U;

        BMS_State_ClearRequests();

        BMS_State_Set(BMS_SYSTEM_FAULT);

        return;
    }

    /*
     * Loss of required measurements returns to INIT.
     */
    if (BMS_State_MeasurementsValid() == 0U)
    {
        BMS_Data.system_enabled = 0U;

        BMS_State_ClearRequests();

        BMS_State_Set(BMS_SYSTEM_INIT);

        return;
    }

    /*
     * Explicit shutdown request.
     */
    if (shutdown_request != 0U)
    {
        BMS_Data.system_enabled = 0U;

        BMS_State_ClearRequests();

        BMS_State_Set(BMS_SYSTEM_SHUTDOWN);

        return;
    }

    /*
     * Explicit enable request.
     */
    if (enable_request != 0U)
    {
        enable_request = 0U;
        disable_request = 0U;

        if (BMS_State_SafetyOK() != 0U)
        {
            BMS_Data.system_enabled = 1U;

            BMS_State_Set(BMS_SYSTEM_NORMAL);

            return;
        }
    }

    /*
     * Remain safely disabled.
     */
    BMS_Data.system_enabled = 0U;

    /*
     * A disable request is no longer relevant while
     * already in WARNING.
     */
    disable_request = 0U;
}

/**
 * @brief Handle NORMAL state.
 */
static void BMS_State_HandleNormal(void)
{
    /*
     * Any active or latched fault immediately disables BMS.
     */
    if ((BMS_Data.fault_status != 0U) ||
        (BMS_Data.fault_latched != 0U))
    {
        BMS_Data.system_enabled = 0U;

        BMS_State_ClearRequests();

        BMS_State_Set(BMS_SYSTEM_FAULT);

        return;
    }

    /*
     * Invalid measurements are not acceptable in NORMAL.
     */
    if (BMS_State_MeasurementsValid() == 0U)
    {
        BMS_Data.system_enabled = 0U;

        BMS_State_ClearRequests();

        BMS_State_Set(BMS_SYSTEM_INIT);

        return;
    }

    /*
     * Explicit shutdown.
     */
    if (shutdown_request != 0U)
    {
        BMS_Data.system_enabled = 0U;

        BMS_State_ClearRequests();

        BMS_State_Set(BMS_SYSTEM_SHUTDOWN);

        return;
    }

    /*
     * Explicit disable.
     */
    if (disable_request != 0U)
    {
        BMS_Data.system_enabled = 0U;

        BMS_State_ClearRequests();

        BMS_State_Set(BMS_SYSTEM_WARNING);

        return;
    }

    /*
     * Enable request is unnecessary once already NORMAL.
     */
    enable_request = 0U;

    /*
     * Continue normal operation.
     */
    BMS_Data.system_enabled = 1U;
}

/**
 * @brief Handle FAULT state.
 */
static void BMS_State_HandleFault(void)
{
    /*
     * BMS must always be disabled during FAULT.
     */
    BMS_Data.system_enabled = 0U;

    /*
     * Shutdown takes priority.
     */
    if (shutdown_request != 0U)
    {
        BMS_State_ClearRequests();

        BMS_State_Set(BMS_SYSTEM_SHUTDOWN);

        return;
    }

    /*
     * Do not recover while an active or latched fault remains.
     */
    if ((BMS_Data.fault_status != 0U) ||
        (BMS_Data.fault_latched != 0U))
    {
        return;
    }

    /*
     * Recovery requires an explicit request.
     */
    if (recovery_request != 0U)
    {
        recovery_request = 0U;
        enable_request = 0U;
        disable_request = 0U;
        shutdown_request = 0U;

        /*
         * Verify the measurements are still valid and
         * there are no remaining safety problems.
         */
        if (BMS_State_SafetyOK() != 0U)
        {
            BMS_Data.system_enabled = 0U;

            BMS_State_Set(BMS_SYSTEM_WARNING);
        }
    }
}

/**
 * @brief Handle SHUTDOWN state.
 */
static void BMS_State_HandleShutdown(void)
{
    /*
     * Shutdown always keeps BMS disabled.
     */
    BMS_Data.system_enabled = 0U;

    /*
     * Fault condition keeps ECU in SHUTDOWN.
     */
    if ((BMS_Data.fault_status != 0U) ||
        (BMS_Data.fault_latched != 0U))
    {
        return;
    }

    /*
     * Recovery requires an explicit request.
     */
    if (recovery_request != 0U)
    {
        recovery_request = 0U;
        enable_request = 0U;
        disable_request = 0U;
        shutdown_request = 0U;

        /*
         * Measurements must be valid before leaving shutdown.
         */
        if (BMS_State_MeasurementsValid() != 0U)
        {
            BMS_State_Set(BMS_SYSTEM_WARNING);
        }
    }
}

/* -------------------------------------------------------------------------- */
/* Public functions                                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize the BMS state machine.
 */
void BMS_State_Init(void)
{
    BMS_State_ClearRequests();

    BMS_Data.system_state = BMS_SYSTEM_INIT;

    BMS_Data.system_enabled = 0U;

    state_entry_time = HAL_GetTick();
}

/**
 * @brief Update the BMS state machine.
 */
void BMS_State_Update(void)
{
    /*
     * Global fault handling.
     *
     * FAULT overrides all normal states.
     * SHUTDOWN remains SHUTDOWN even if a fault occurs there.
     */
    if ((BMS_Data.fault_status != 0U) ||
        (BMS_Data.fault_latched != 0U))
    {
        if (BMS_Data.system_state != BMS_SYSTEM_SHUTDOWN)
        {
            BMS_Data.system_enabled = 0U;

            /*
             * Do not allow a stale enable request to
             * automatically re-enable the BMS after recovery.
             */
            enable_request = 0U;
            disable_request = 0U;

            BMS_State_Set(BMS_SYSTEM_FAULT);
        }
    }

    /*
     * Process current-state behavior.
     */
    switch (BMS_Data.system_state)
    {
        case BMS_SYSTEM_INIT:

            BMS_State_HandleInit();

            break;

        case BMS_SYSTEM_NORMAL:

            BMS_State_HandleNormal();

            break;

        case BMS_SYSTEM_WARNING:

            BMS_State_HandleWarning();

            break;

        case BMS_SYSTEM_FAULT:

            BMS_State_HandleFault();

            break;

        case BMS_SYSTEM_SHUTDOWN:

            BMS_State_HandleShutdown();

            break;

        default:

            /*
             * Unknown state -> safest state is INIT.
             */
            BMS_Data.system_enabled = 0U;

            BMS_State_ClearRequests();

            BMS_State_Set(BMS_SYSTEM_INIT);

            break;
    }

    /*
     * Retain the timestamp for debugging and future
     * state-duration diagnostics.
     */
    (void)state_entry_time;
}

/**
 * @brief Request BMS enable.
 */
void BMS_State_RequestEnable(void)
{
    /*
     * Never accept an enable request while any fault
     * is active or latched.
     */
    if ((BMS_Data.fault_status != 0U) ||
        (BMS_Data.fault_latched != 0U))
    {
        return;
    }

    enable_request = 1U;

    /*
     * Enable and disable requests are mutually exclusive.
     */
    disable_request = 0U;
}

/**
 * @brief Request BMS disable.
 */
void BMS_State_RequestDisable(void)
{
    disable_request = 1U;

    /*
     * Cancel a pending enable.
     */
    enable_request = 0U;
}

/**
 * @brief Request BMS shutdown.
 */
void BMS_State_RequestShutdown(void)
{
    shutdown_request = 1U;

    /*
     * Shutdown cancels normal enable/disable transitions.
     */
    enable_request = 0U;
    disable_request = 0U;
}

/**
 * @brief Request recovery from FAULT or SHUTDOWN.
 */
void BMS_State_RequestRecovery(void)
{
    recovery_request = 1U;
}

/**
 * @brief Get current BMS state.
 */
uint8_t BMS_State_Get(void)
{
    return (uint8_t)BMS_Data.system_state;
}

/**
 * @brief Check whether BMS is in NORMAL.
 */
uint8_t BMS_State_IsNormal(void)
{
    if (BMS_Data.system_state == BMS_SYSTEM_NORMAL)
    {
        return 1U;
    }

    return 0U;
}

/**
 * @brief Check whether BMS is in FAULT.
 */
uint8_t BMS_State_IsFault(void)
{
    if ((BMS_Data.system_state == BMS_SYSTEM_FAULT) ||
        (BMS_Data.fault_status != 0U) ||
        (BMS_Data.fault_latched != 0U))
    {
        return 1U;
    }

    return 0U;
}

/**
 * @brief Check whether BMS is in SHUTDOWN.
 */
uint8_t BMS_State_IsShutdown(void)
{
    if (BMS_Data.system_state == BMS_SYSTEM_SHUTDOWN)
    {
        return 1U;
    }

    return 0U;
}
