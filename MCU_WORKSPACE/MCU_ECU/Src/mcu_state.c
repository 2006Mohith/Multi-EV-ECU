#include "main.h"
#include "mcu_config.h"
#include "mcu_data.h"
#include "mcu_motor.h"
#include "mcu_input.h"
#include "mcu_state.h"

/* ============================================================================
 * MCU State Management
 * ============================================================================
 *
 * State flow:
 *
 *              +--------+
 *              |  INIT  |
 *              +---+----+
 *                  |
 *                  v
 *            +-----------+
 *            | DISABLED  |
 *            +-----+-----+
 *                  |
 *              healthy
 *                  |
 *                  v
 *            +-----------+
 *            |   READY   |
 *            +-----+-----+
 *                  |
 *              run request
 *                  |
 *                  v
 *            +-----------+
 *            |    RUN    |
 *            +-----+-----+
 *              |       |
 *            brake     stop
 *              |       |
 *              v       v
 *           +------+  READY
 *           | BRAKE|
 *           +------+
 *
 * Serious fault:
 *
 *   ANY STATE -> FAULT
 *
 * Shutdown:
 *
 *   ANY STATE -> SHUTDOWN
 *
 * IMPORTANT:
 *
 * MCU_Data.motor_command belongs to the command-reception layer.
 * This state module READS that structure but does not overwrite it
 * when performing motor output actions.
 *
 * ========================================================================== */

/* ============================================================================
 * Private request variables
 * ========================================================================== */

static uint8_t mcu_requested_state;

static uint8_t mcu_fault_requested;
static uint8_t mcu_shutdown_requested;
static uint8_t mcu_recovery_requested;

static uint8_t mcu_run_requested;
static uint8_t mcu_stop_requested;
static uint8_t mcu_brake_requested;

/* ============================================================================
 * Private fault evaluation
 * ========================================================================== */

static uint8_t MCU_State_HasActiveFault(void)
{
    if (MCU_Data.fault.active)
    {
        return MCU_TRUE;
    }

    if (MCU_Data.fault.latched)
    {
        return MCU_TRUE;
    }

    if (MCU_Data.fault.can_fault)
    {
        return MCU_TRUE;
    }

    if (MCU_Data.fault.vcu_comm_fault)
    {
        return MCU_TRUE;
    }

    if (MCU_Data.fault.command_fault)
    {
        return MCU_TRUE;
    }

    if (MCU_Data.fault.hall_fault)
    {
        return MCU_TRUE;
    }

    if (MCU_Data.fault.direction_fault)
    {
        return MCU_TRUE;
    }

    if (MCU_Data.fault.motor_driver_fault)
    {
        return MCU_TRUE;
    }

    if (MCU_Data.fault.potentiometer_fault)
    {
        return MCU_TRUE;
    }

    if (MCU_Data.fault.button_fault)
    {
        return MCU_TRUE;
    }

    return MCU_FALSE;
}

/* -------------------------------------------------------------------------- */

static uint8_t MCU_State_IsSeriousFault(void)
{
    if (!MCU_State_HasActiveFault())
    {
        return MCU_FALSE;
    }

    if (MCU_Data.fault.can_fault)
    {
        return MCU_TRUE;
    }

    if (MCU_Data.fault.vcu_comm_fault)
    {
        return MCU_TRUE;
    }

    if (MCU_Data.fault.command_fault)
    {
        return MCU_TRUE;
    }

    if (MCU_Data.fault.hall_fault)
    {
        return MCU_TRUE;
    }

    if (MCU_Data.fault.direction_fault)
    {
        return MCU_TRUE;
    }

    if (MCU_Data.fault.motor_driver_fault)
    {
        return MCU_TRUE;
    }

    if (MCU_Data.fault.button_fault)
    {
        return MCU_TRUE;
    }

    return MCU_FALSE;
}

/* ============================================================================
 * Private motor control helpers
 * ========================================================================== */

/*
 * IMPORTANT:
 *
 * These functions operate on the actual motor hardware/state only.
 * They do NOT overwrite MCU_Data.motor_command because that structure
 * contains the latest command received from the VCU.
 */

/* -------------------------------------------------------------------------- */

static void MCU_State_CommandMotorStop(void)
{
    MCU_Motor_SafeStop();

    MCU_Data.motor.enabled = MCU_DISABLED;
    MCU_Data.motor.direction = MCU_DIRECTION_NEUTRAL;
    MCU_Data.motor.pwm_percent = 0U;
    MCU_Data.motor.pwm_compare = 0U;
    MCU_Data.motor.braking = MCU_FALSE;
    MCU_Data.motor.driver_enabled = MCU_FALSE;
}

/* -------------------------------------------------------------------------- */

static void MCU_State_CommandMotorRun(uint8_t direction,
                                      uint8_t pwm_percent)
{
    if ((direction != MCU_DIRECTION_FORWARD) &&
        (direction != MCU_DIRECTION_REVERSE))
    {
        MCU_State_CommandMotorStop();
        return;
    }

    if (pwm_percent > MCU_PWM_MAX_PERCENT)
    {
        pwm_percent = MCU_PWM_MAX_PERCENT;
    }

    MCU_Data.motor.direction = direction;
    MCU_Data.motor.enabled = MCU_ENABLED;
    MCU_Data.motor.pwm_percent = pwm_percent;
    MCU_Data.motor.braking = MCU_FALSE;

    MCU_Motor_Enable();
    MCU_Motor_SetDirection(direction);
    MCU_Motor_SetPWM(pwm_percent);
    MCU_Motor_ApplyOutputs();

    MCU_Data.motor.driver_enabled = MCU_TRUE;
}

/* -------------------------------------------------------------------------- */

static void MCU_State_CommandMotorBrake(void)
{
    /*
     * Current hardware implementation uses a zero-PWM safe stop.
     */
    MCU_Motor_SafeStop();

    MCU_Data.motor.enabled = MCU_DISABLED;
    MCU_Data.motor.direction = MCU_DIRECTION_NEUTRAL;
    MCU_Data.motor.pwm_percent =
        MCU_MAX_PWM_WHILE_BRAKING;
    MCU_Data.motor.pwm_compare = 0U;
    MCU_Data.motor.braking = MCU_TRUE;
    MCU_Data.motor.driver_enabled = MCU_FALSE;
}

/* ============================================================================
 * Initialization
 * ========================================================================== */

void MCU_State_Init(void)
{
    mcu_requested_state = MCU_STATE_INIT;

    mcu_fault_requested = MCU_FALSE;
    mcu_shutdown_requested = MCU_FALSE;
    mcu_recovery_requested = MCU_FALSE;

    mcu_run_requested = MCU_FALSE;
    mcu_stop_requested = MCU_FALSE;
    mcu_brake_requested = MCU_FALSE;

    MCU_Data.motor.state = MCU_STATE_INIT;
    MCU_Data.system_healthy = MCU_FALSE;
}

/* ============================================================================
 * Main state-machine update
 * ========================================================================== */

void MCU_State_Update(void)
{
    /*
     * Shutdown has highest priority.
     */
    if (mcu_shutdown_requested)
    {
        MCU_State_SetState(MCU_STATE_SHUTDOWN);
        return;
    }

    /*
     * Explicit fault request.
     *
     * Clear the request immediately after consuming it. A persistent
     * hardware/software fault is separately detected through
     * MCU_State_IsSeriousFault().
     */
    if (mcu_fault_requested)
    {
        mcu_fault_requested = MCU_FALSE;

        MCU_State_SetState(MCU_STATE_FAULT);
        return;
    }

    /*
     * Persistent serious fault.
     */
    if (MCU_State_IsSeriousFault())
    {
        MCU_State_SetState(MCU_STATE_FAULT);
        return;
    }

    switch (MCU_Data.motor.state)
    {
        case MCU_STATE_INIT:
            MCU_State_ProcessInit();
            break;

        case MCU_STATE_DISABLED:
            MCU_State_ProcessDisabled();
            break;

        case MCU_STATE_READY:
            MCU_State_ProcessReady();
            break;

        case MCU_STATE_RUN:
            MCU_State_ProcessRun();
            break;

        case MCU_STATE_BRAKE:
            MCU_State_ProcessBrake();
            break;

        case MCU_STATE_WARNING:
            MCU_State_ProcessWarning();
            break;

        case MCU_STATE_FAULT:
            MCU_State_ProcessFault();
            break;

        case MCU_STATE_SHUTDOWN:
            MCU_State_ProcessShutdown();
            break;

        default:
            MCU_State_SetState(MCU_STATE_FAULT);
            break;
    }
}

/* ============================================================================
 * INIT state
 * ========================================================================== */

void MCU_State_ProcessInit(void)
{
    MCU_State_CommandMotorStop();

    MCU_Data.system_healthy = MCU_FALSE;
    MCU_Data.motor.state = MCU_STATE_INIT;

    if (MCU_Data.system_initialized)
    {
        MCU_State_SetState(MCU_STATE_DISABLED);
    }
}

/* ============================================================================
 * DISABLED state
 * ========================================================================== */

void MCU_State_ProcessDisabled(void)
{
    MCU_State_CommandMotorStop();

    MCU_Data.system_healthy =
        MCU_State_IsSystemHealthy();

    /*
     * STOP keeps the system disabled.
     */
    if (mcu_stop_requested)
    {
        mcu_stop_requested = MCU_FALSE;
        mcu_run_requested = MCU_FALSE;
        return;
    }

    /*
     * A physical STOP button also cancels any run request.
     */
    if (MCU_Input_IsStopPressed())
    {
        mcu_stop_requested = MCU_FALSE;
        mcu_run_requested = MCU_FALSE;
        return;
    }

    if (MCU_State_IsSeriousFault())
    {
        MCU_State_SetState(MCU_STATE_FAULT);
        return;
    }

    if (MCU_State_IsSystemHealthy())
    {
        MCU_State_SetState(MCU_STATE_READY);
    }
}

/* ============================================================================
 * READY state
 * ========================================================================== */

void MCU_State_ProcessReady(void)
{
    uint8_t direction;

    /*
     * Always keep actual motor output stopped while READY.
     */
    MCU_State_CommandMotorStop();

    MCU_Data.system_healthy =
        MCU_State_IsSystemHealthy();

    /*
     * STOP request.
     */
    if (mcu_stop_requested)
    {
        mcu_stop_requested = MCU_FALSE;
        mcu_run_requested = MCU_FALSE;
        mcu_brake_requested = MCU_FALSE;

        MCU_State_SetState(MCU_STATE_DISABLED);
        return;
    }

    /*
     * Physical STOP button.
     */
    if (MCU_Input_IsStopPressed())
    {
        mcu_run_requested = MCU_FALSE;
        mcu_brake_requested = MCU_FALSE;

        MCU_State_SetState(MCU_STATE_DISABLED);
        return;
    }

    /*
     * Serious fault.
     */
    if (MCU_State_IsSeriousFault())
    {
        MCU_State_SetState(MCU_STATE_FAULT);
        return;
    }

    /*
     * Not healthy -> warning.
     */
    if (!MCU_State_IsSystemHealthy())
    {
        MCU_State_SetState(MCU_STATE_WARNING);
        return;
    }

    /*
     * Brake request has priority.
     */
    if (mcu_brake_requested)
    {
        mcu_brake_requested = MCU_FALSE;
        mcu_run_requested = MCU_FALSE;

        MCU_State_SetState(MCU_STATE_BRAKE);
        return;
    }

    /*
     * -------------------------------------------------------------
     * VCU-issued RUN command.
     *
     * NOTE:
     * motor_command is read only here.
     * -------------------------------------------------------------
     */
    if (MCU_Data.motor_command.valid &&
        MCU_Data.motor_command.enable &&
        (MCU_Data.motor_command.command == MCU_COMMAND_RUN))
    {
        mcu_run_requested = MCU_TRUE;
    }

    /*
     * A VCU stop/disable command cancels local run request.
     */
    if (MCU_Data.motor_command.valid &&
        ((MCU_Data.motor_command.command == MCU_COMMAND_STOP) ||
         (MCU_Data.motor_command.command == MCU_COMMAND_DISABLE)))
    {
        mcu_run_requested = MCU_FALSE;
    }

    /*
     * Local test mode:
     * START button creates a run request.
     */
    if (MCU_LOCAL_TEST_MODE_ENABLED &&
        MCU_Input_IsStartPressed())
    {
        mcu_run_requested = MCU_TRUE;
    }

    if (mcu_run_requested)
    {
        /*
         * First use the direction supplied by the VCU.
         */
        direction = MCU_Data.motor_command.direction;

        /*
         * In local-test mode, physical direction buttons can provide
         * the direction when the VCU command does not contain one.
         */
        if ((direction == MCU_DIRECTION_NEUTRAL) &&
            MCU_LOCAL_TEST_MODE_ENABLED)
        {
            direction =
                MCU_Input_GetRequestedDirection();
        }

        if ((direction == MCU_DIRECTION_FORWARD) ||
            (direction == MCU_DIRECTION_REVERSE))
        {
            MCU_State_SetState(MCU_STATE_RUN);
        }
    }
}

/* ============================================================================
 * RUN state
 * ========================================================================== */

void MCU_State_ProcessRun(void)
{
    uint8_t direction;
    uint8_t pwm_percent;
    uint8_t button_direction;

    MCU_Data.system_healthy =
        MCU_State_IsSystemHealthy();

    /*
     * Safety check.
     */
    if (!MCU_State_IsSystemHealthy())
    {
        mcu_run_requested = MCU_FALSE;

        MCU_State_CommandMotorStop();
        MCU_State_SetState(MCU_STATE_WARNING);
        return;
    }

    /*
     * Physical STOP button.
     */
    if (MCU_Input_IsStopPressed())
    {
        mcu_stop_requested = MCU_TRUE;
        mcu_run_requested = MCU_FALSE;
    }

    /*
     * Process latest VCU command.
     *
     * motor_command is only READ here.
     */
    if (MCU_Data.vcu_comm.command_received)
    {
        if ((MCU_Data.motor_command.command == MCU_COMMAND_STOP) ||
            (MCU_Data.motor_command.command == MCU_COMMAND_DISABLE))
        {
            mcu_stop_requested = MCU_TRUE;
            mcu_run_requested = MCU_FALSE;
        }
        else if (MCU_Data.motor_command.command == MCU_COMMAND_BRAKE)
        {
            mcu_brake_requested = MCU_TRUE;
            mcu_run_requested = MCU_FALSE;
        }
        else if ((MCU_Data.motor_command.command == MCU_COMMAND_RUN) &&
                 !MCU_Data.motor_command.enable)
        {
            mcu_stop_requested = MCU_TRUE;
            mcu_run_requested = MCU_FALSE;
        }
    }

    /*
     * STOP request.
     */
    if (mcu_stop_requested)
    {
        mcu_stop_requested = MCU_FALSE;
        mcu_run_requested = MCU_FALSE;

        MCU_State_CommandMotorStop();
        MCU_State_SetState(MCU_STATE_READY);
        return;
    }

    /*
     * BRAKE request.
     */
    if (mcu_brake_requested)
    {
        mcu_brake_requested = MCU_FALSE;
        mcu_run_requested = MCU_FALSE;

        MCU_State_CommandMotorBrake();
        MCU_State_SetState(MCU_STATE_BRAKE);
        return;
    }

    /*
     * A command that is no longer valid cannot keep the motor running.
     */
    if (!MCU_Data.motor_command.valid &&
        !MCU_LOCAL_TEST_MODE_ENABLED)
    {
        mcu_run_requested = MCU_FALSE;

        MCU_State_CommandMotorStop();
        MCU_State_SetState(MCU_STATE_READY);
        return;
    }

    /* ------------------------------------------------------------------------
     * Determine direction
     * ---------------------------------------------------------------------- */

    direction = MCU_Data.motor_command.direction;

    /*
     * Local-test mode:
     * physical FORWARD/REVERSE buttons can override neutral VCU direction.
     */
    if (MCU_LOCAL_TEST_MODE_ENABLED)
    {
        button_direction =
            MCU_Input_GetRequestedDirection();

        if (button_direction != MCU_DIRECTION_NEUTRAL)
        {
            direction = button_direction;
        }
        else if (MCU_Data.motor_command.direction ==
                 MCU_DIRECTION_NEUTRAL)
        {
            /*
             * No direction available -> stop.
             */
            direction = MCU_DIRECTION_NEUTRAL;
        }
    }

    /*
     * Neutral/invalid direction means stop.
     */
    if ((direction != MCU_DIRECTION_FORWARD) &&
        (direction != MCU_DIRECTION_REVERSE))
    {
        mcu_run_requested = MCU_FALSE;

        MCU_State_CommandMotorStop();
        MCU_State_SetState(MCU_STATE_READY);
        return;
    }

    /* ------------------------------------------------------------------------
     * Determine PWM
     * ---------------------------------------------------------------------- */

    pwm_percent = MCU_Data.motor_command.pwm_percent;

    /*
     * Local-test mode:
     * potentiometer controls motor PWM.
     */
    if (MCU_LOCAL_TEST_MODE_ENABLED)
    {
        if (MCU_Data.adc.valid)
        {
            pwm_percent =
                MCU_Data.adc.pot_percent;
        }
        else
        {
            pwm_percent = 0U;
        }
    }

    if (pwm_percent > MCU_PWM_MAX_PERCENT)
    {
        pwm_percent = MCU_PWM_MAX_PERCENT;
    }

    /* ------------------------------------------------------------------------
     * Run motor
     * ---------------------------------------------------------------------- */

    MCU_Data.motor.state = MCU_STATE_RUN;
    MCU_Data.motor.direction = direction;
    MCU_Data.motor.enabled = MCU_ENABLED;
    MCU_Data.motor.pwm_percent = pwm_percent;
    MCU_Data.motor.braking = MCU_FALSE;

    MCU_State_CommandMotorRun(
        direction,
        pwm_percent);
}

/* ============================================================================
 * BRAKE state
 * ========================================================================== */

void MCU_State_ProcessBrake(void)
{
    MCU_State_CommandMotorBrake();

    MCU_Data.system_healthy =
        MCU_State_IsSystemHealthy();

    MCU_Data.motor.state = MCU_STATE_BRAKE;
    MCU_Data.motor.braking = MCU_TRUE;

    /*
     * Physical STOP cancels braking and returns to READY.
     */
    if (MCU_Input_IsStopPressed())
    {
        mcu_stop_requested = MCU_FALSE;
        mcu_run_requested = MCU_FALSE;

        MCU_Data.motor.braking = MCU_FALSE;
        MCU_State_SetState(MCU_STATE_READY);
        return;
    }

    /*
     * Explicit STOP request.
     */
    if (mcu_stop_requested)
    {
        mcu_stop_requested = MCU_FALSE;
        mcu_run_requested = MCU_FALSE;

        MCU_Data.motor.braking = MCU_FALSE;
        MCU_State_SetState(MCU_STATE_READY);
        return;
    }

    /*
     * Current brake action is a zero-PWM stop.
     * Return to READY once the system remains healthy.
     */
    if (MCU_State_IsSystemHealthy())
    {
        MCU_Data.motor.braking = MCU_FALSE;
        MCU_State_SetState(MCU_STATE_READY);
    }
}

/* ============================================================================
 * WARNING state
 * ========================================================================== */

void MCU_State_ProcessWarning(void)
{
    MCU_State_CommandMotorStop();

    MCU_Data.motor.state = MCU_STATE_WARNING;
    MCU_Data.system_healthy = MCU_FALSE;

    if (mcu_shutdown_requested)
    {
        MCU_State_SetState(MCU_STATE_SHUTDOWN);
        return;
    }

    if (MCU_State_IsSeriousFault())
    {
        MCU_State_SetState(MCU_STATE_FAULT);
        return;
    }

    /*
     * Physical STOP keeps the motor stopped.
     */
    if (MCU_Input_IsStopPressed())
    {
        mcu_run_requested = MCU_FALSE;
        return;
    }

    /*
     * Return to READY when all required conditions recover.
     */
    if (MCU_State_IsSystemHealthy())
    {
        MCU_Data.system_healthy = MCU_TRUE;
        MCU_State_SetState(MCU_STATE_READY);
    }
}

/* ============================================================================
 * FAULT state
 * ========================================================================== */

void MCU_State_ProcessFault(void)
{
    MCU_State_CommandMotorStop();

    MCU_Data.motor.state = MCU_STATE_FAULT;
    MCU_Data.system_healthy = MCU_FALSE;

    /*
     * Consume any stale software fault request.
     */
    mcu_fault_requested = MCU_FALSE;

    /*
     * Recovery requires an explicit request.
     */
    if (mcu_recovery_requested)
    {
        mcu_recovery_requested = MCU_FALSE;

        if (MCU_State_CanRecover())
        {
            /*
             * Ask the fault manager to perform the actual reset on its
             * next fault-management cycle.
             */
            MCU_Data.fault.reset_requested = MCU_TRUE;

            /*
             * Clear the general fault indication immediately so the
             * state machine can leave FAULT.
             *
             * Individual fault flags are cleared by MCU_Faults_Reset().
             */
            MCU_Data.fault.active = MCU_FALSE;
            MCU_Data.fault.latched = MCU_FALSE;
            MCU_Data.fault.code = MCU_FAULT_NONE;
            MCU_Data.fault.severity =
                MCU_FAULT_SEVERITY_NONE;

            mcu_fault_requested = MCU_FALSE;

            MCU_State_SetState(MCU_STATE_DISABLED);
        }
    }
}

/* ============================================================================
 * SHUTDOWN state
 * ========================================================================== */

void MCU_State_ProcessShutdown(void)
{
    MCU_State_CommandMotorStop();

    MCU_Data.motor.state = MCU_STATE_SHUTDOWN;
    MCU_Data.motor.enabled = MCU_DISABLED;
    MCU_Data.motor.direction = MCU_DIRECTION_NEUTRAL;
    MCU_Data.motor.pwm_percent = 0U;
    MCU_Data.motor.pwm_compare = 0U;
    MCU_Data.motor.braking = MCU_FALSE;

    MCU_Data.system_healthy = MCU_FALSE;
}

/* ============================================================================
 * State transition
 * ========================================================================== */

void MCU_State_SetState(uint8_t new_state)
{
    if (new_state > MCU_STATE_SHUTDOWN)
    {
        new_state = MCU_STATE_FAULT;
    }

    MCU_Data.motor.state = new_state;
    mcu_requested_state = new_state;

    if (new_state != MCU_STATE_RUN)
    {
        MCU_Data.motor.direction_change_pending =
            MCU_FALSE;
    }

    if (new_state == MCU_STATE_BRAKE)
    {
        MCU_Data.motor.braking = MCU_TRUE;
    }
    else
    {
        MCU_Data.motor.braking = MCU_FALSE;
    }

    /*
     * Every non-running state forces the physical motor outputs safe.
     */
    if ((new_state == MCU_STATE_DISABLED) ||
        (new_state == MCU_STATE_READY) ||
        (new_state == MCU_STATE_WARNING) ||
        (new_state == MCU_STATE_FAULT) ||
        (new_state == MCU_STATE_SHUTDOWN))
    {
        MCU_State_CommandMotorStop();
    }
}

/* ============================================================================
 * Get current state
 * ========================================================================== */

uint8_t MCU_State_GetState(void)
{
    return MCU_Data.motor.state;
}

/* ============================================================================
 * State query functions
 * ========================================================================== */

uint8_t MCU_State_IsInit(void)
{
    return (MCU_Data.motor.state == MCU_STATE_INIT) ?
           MCU_TRUE : MCU_FALSE;
}

uint8_t MCU_State_IsDisabled(void)
{
    return (MCU_Data.motor.state == MCU_STATE_DISABLED) ?
           MCU_TRUE : MCU_FALSE;
}

uint8_t MCU_State_IsReady(void)
{
    return (MCU_Data.motor.state == MCU_STATE_READY) ?
           MCU_TRUE : MCU_FALSE;
}

uint8_t MCU_State_IsRunning(void)
{
    return (MCU_Data.motor.state == MCU_STATE_RUN) ?
           MCU_TRUE : MCU_FALSE;
}

uint8_t MCU_State_IsBraking(void)
{
    return (MCU_Data.motor.state == MCU_STATE_BRAKE) ?
           MCU_TRUE : MCU_FALSE;
}

uint8_t MCU_State_IsWarning(void)
{
    return (MCU_Data.motor.state == MCU_STATE_WARNING) ?
           MCU_TRUE : MCU_FALSE;
}

uint8_t MCU_State_IsFault(void)
{
    return (MCU_Data.motor.state == MCU_STATE_FAULT) ?
           MCU_TRUE : MCU_FALSE;
}

uint8_t MCU_State_IsShutdown(void)
{
    return (MCU_Data.motor.state == MCU_STATE_SHUTDOWN) ?
           MCU_TRUE : MCU_FALSE;
}

/* ============================================================================
 * System health
 * ========================================================================== */

uint8_t MCU_State_IsSystemHealthy(void)
{
    /*
     * ADC must be operational.
     */
    if (!MCU_Data.task.adc_ok)
    {
        return MCU_FALSE;
    }

    /*
     * CAN subsystem must be operational.
     */
    if (!MCU_Data.task.can_ok)
    {
        return MCU_FALSE;
    }

    /*
     * Motor subsystem must be operational.
     */
    if (!MCU_Data.task.motor_ok)
    {
        return MCU_FALSE;
    }

    /*
     * Physical inputs must be valid.
     */
    if (!MCU_Data.task.input_ok)
    {
        return MCU_FALSE;
    }

    /*
     * Normal network operation requires VCU communication.
     * Local-test mode intentionally bypasses this requirement.
     */
    if (!MCU_LOCAL_TEST_MODE_ENABLED)
    {
        if (!MCU_Data.vcu_comm.communication_valid)
        {
            return MCU_FALSE;
        }
    }

    /*
     * Any active safety fault prevents READY/RUN.
     */
    if (MCU_State_HasActiveFault())
    {
        return MCU_FALSE;
    }

    return MCU_TRUE;
}

/* ============================================================================
 * Run permission
 * ========================================================================== */

uint8_t MCU_State_CanRun(void)
{
    if (!MCU_State_IsSystemHealthy())
    {
        return MCU_FALSE;
    }

    if (MCU_Data.motor.state == MCU_STATE_FAULT)
    {
        return MCU_FALSE;
    }

    if (MCU_Data.motor.state == MCU_STATE_SHUTDOWN)
    {
        return MCU_FALSE;
    }

    if (MCU_Data.input.direction_conflict)
    {
        return MCU_FALSE;
    }

    return MCU_TRUE;
}

/* ============================================================================
 * Recovery permission
 * ========================================================================== */

uint8_t MCU_State_CanRecover(void)
{
    /*
     * Shutdown cannot use the normal fault recovery path.
     */
    if (mcu_shutdown_requested)
    {
        return MCU_FALSE;
    }

    /*
     * Motor hardware must not be running.
     */
    if (MCU_Data.motor.enabled)
    {
        return MCU_FALSE;
    }

    /*
     * Physical direction conflict must be gone.
     */
    if (MCU_Data.input.direction_conflict)
    {
        return MCU_FALSE;
    }

    /*
     * Motor driver must be healthy.
     */
    if (MCU_Data.fault.motor_driver_fault)
    {
        return MCU_FALSE;
    }

    /*
     * CAN hardware must be healthy.
     */
    if (MCU_Data.fault.can_fault)
    {
        return MCU_FALSE;
    }

    if (!MCU_Data.task.can_ok)
    {
        return MCU_FALSE;
    }

    /*
     * Buttons must be valid.
     */
    if (MCU_Data.fault.button_fault)
    {
        return MCU_FALSE;
    }

    if (!MCU_Data.task.input_ok)
    {
        return MCU_FALSE;
    }

    /*
     * ADC/potentiometer must be valid.
     */
    if (!MCU_Data.adc.valid)
    {
        return MCU_FALSE;
    }

    if (!MCU_Data.task.adc_ok)
    {
        return MCU_FALSE;
    }

    /*
     * Motor subsystem must be healthy.
     */
    if (!MCU_Data.task.motor_ok)
    {
        return MCU_FALSE;
    }

    /*
     * Normal networked operation requires VCU communication to have
     * recovered before a fault can be cleared.
     */
    if (!MCU_LOCAL_TEST_MODE_ENABLED)
    {
        if (!MCU_Data.vcu_comm.communication_valid)
        {
            return MCU_FALSE;
        }
    }

    return MCU_TRUE;
}

/* ============================================================================
 * Request functions
 * ========================================================================== */

void MCU_State_RequestFault(void)
{
    mcu_fault_requested = MCU_TRUE;
}

void MCU_State_RequestShutdown(void)
{
    mcu_shutdown_requested = MCU_TRUE;
}

void MCU_State_RequestRecovery(void)
{
    mcu_recovery_requested = MCU_TRUE;
}

void MCU_State_RequestRun(void)
{
    mcu_run_requested = MCU_TRUE;
    mcu_stop_requested = MCU_FALSE;
}

void MCU_State_RequestStop(void)
{
    mcu_stop_requested = MCU_TRUE;
    mcu_run_requested = MCU_FALSE;
    mcu_brake_requested = MCU_FALSE;
}

void MCU_State_RequestBrake(void)
{
    mcu_brake_requested = MCU_TRUE;
    mcu_run_requested = MCU_FALSE;
}
