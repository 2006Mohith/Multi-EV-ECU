/*
 * mcu_system.c
 *
 *  Created on: 21-Sept-2026
 *      Author: MohithK
 */


#include "main.h"

#include "mcu_config.h"
#include "mcu_data.h"
#include "mcu_motor.h"
#include "mcu_hall.h"
#include "mcu_adc.h"
#include "mcu_can.h"
#include "mcu_input.h"
#include "mcu_state.h"
#include "mcu_faults.h"
#include "mcu_lcd.h"
#include "mcu_debug.h"
#include "mcu_system.h"

/* ============================================================================
 * MCU System Integration
 * ============================================================================
 *
 * Main execution structure:
 *
 *                 MCU_System_Init()
 *                         |
 *                         v
 *                    Main while(1)
 *                         |
 *                         v
 *                 MCU_System_Run()
 *                         |
 *          +--------------+--------------+
 *          |              |              |
 *       20 ms          100 ms         500 ms
 *          |              |              |
 *       Control          CAN            LCD
 *       tasks            tasks          task
 *
 * Debug output runs at a separate 1-second interval.
 *
 * ========================================================================== */

/* ============================================================================
 * Local timing constants
 * ========================================================================== */

#define MCU_SYSTEM_CONTROL_PERIOD_MS   20U
#define MCU_SYSTEM_CAN_PERIOD_MS       100U
#define MCU_SYSTEM_LCD_PERIOD_MS       500U
#define MCU_SYSTEM_DEBUG_PERIOD_MS    1000U

/* ============================================================================
 * Private variables
 * ========================================================================== */

static uint8_t mcu_system_initialized = MCU_FALSE;

static uint32_t mcu_system_last_control_time = 0U;
static uint32_t mcu_system_last_can_time = 0U;
static uint32_t mcu_system_last_lcd_time = 0U;
static uint32_t mcu_system_last_debug_time = 0U;
static uint32_t mcu_system_last_supervision_time = 0U;

static uint32_t mcu_system_uptime_ms = 0U;

/* ============================================================================
 * Initialization
 * ========================================================================== */

void MCU_System_Init(void)
{
    uint32_t now;

    /*
     * ------------------------------------------------------------------------
     * Global application data
     * ------------------------------------------------------------------------
     */
    MCU_Data_Init();

    /*
     * ------------------------------------------------------------------------
     * Low-level motor control
     *
     * Safe startup:
     * PWM = 0
     * STBY = 0
     * AIN1 = 0
     * AIN2 = 0
     * ------------------------------------------------------------------------
     */
    MCU_Motor_Init();

    /*
     * ------------------------------------------------------------------------
     * Hall sensor
     * ------------------------------------------------------------------------
     */
    MCU_Hall_Init();

    /*
     * ------------------------------------------------------------------------
     * Potentiometer ADC
     * ------------------------------------------------------------------------
     */
    MCU_ADC_Init();

    /*
     * ------------------------------------------------------------------------
     * CAN communication
     * ------------------------------------------------------------------------
     */
    MCU_CAN_Init();

    /*
     * ------------------------------------------------------------------------
     * Physical buttons
     * ------------------------------------------------------------------------
     */
    MCU_Input_Init();

    /*
     * ------------------------------------------------------------------------
     * Fault manager
     * ------------------------------------------------------------------------
     */
    MCU_Faults_Init();

    /*
     * ------------------------------------------------------------------------
     * LCD
     * ------------------------------------------------------------------------
     */
    MCU_LCD_Init();

    /*
     * ------------------------------------------------------------------------
     * UART debug
     * ------------------------------------------------------------------------
     */
    MCU_Debug_Init();

    /*
     * ------------------------------------------------------------------------
     * State machine
     *
     * State initialization is intentionally performed after the other
     * application modules have been initialized.
     * ------------------------------------------------------------------------
     */
    MCU_State_Init();

    now = HAL_GetTick();

    mcu_system_last_control_time = now;
    mcu_system_last_can_time = now;
    mcu_system_last_lcd_time = now;
    mcu_system_last_debug_time = now;
    mcu_system_last_supervision_time = now;

    mcu_system_uptime_ms = 0U;

    mcu_system_initialized = MCU_TRUE;

    MCU_Data.system_initialized = MCU_TRUE;
    MCU_Data.system_healthy = MCU_FALSE;

    /*
     * Request initial LCD and debug update.
     */
    MCU_LCD_RequestUpdate();
    MCU_Debug_RequestUpdate();
}

/* ============================================================================
 * Main system runner
 * ========================================================================== */

void MCU_System_Run(void)
{
    uint32_t now;

    if (!mcu_system_initialized)
    {
        return;
    }

    now = HAL_GetTick();
    mcu_system_uptime_ms = now;

    /* ------------------------------------------------------------------------
     * 20 ms control cycle
     * ---------------------------------------------------------------------- */

    if ((now - mcu_system_last_control_time) >=
        MCU_SYSTEM_CONTROL_PERIOD_MS)
    {
        mcu_system_last_control_time = now;

        MCU_System_TaskADC();
        MCU_System_TaskHall();
        MCU_System_TaskInput();

        MCU_System_TaskFaults();
        MCU_System_TaskState();
        MCU_System_TaskMotor();

        MCU_System_Supervise();
    }

    /* ------------------------------------------------------------------------
     * 100 ms CAN cycle
     * ---------------------------------------------------------------------- */

    if ((now - mcu_system_last_can_time) >=
        MCU_SYSTEM_CAN_PERIOD_MS)
    {
        mcu_system_last_can_time = now;

        MCU_System_TaskCAN();
    }

    /* ------------------------------------------------------------------------
     * 500 ms LCD cycle
     * ---------------------------------------------------------------------- */

    if ((now - mcu_system_last_lcd_time) >=
        MCU_SYSTEM_LCD_PERIOD_MS)
    {
        mcu_system_last_lcd_time = now;

        MCU_System_TaskLCD();
    }

    /* ------------------------------------------------------------------------
     * 1 second debug cycle
     * ---------------------------------------------------------------------- */

    if ((now - mcu_system_last_debug_time) >=
        MCU_SYSTEM_DEBUG_PERIOD_MS)
    {
        mcu_system_last_debug_time = now;

        MCU_System_TaskDebug();
    }
}

/* ============================================================================
 * ADC task
 * ========================================================================== */

void MCU_System_TaskADC(void)
{
    MCU_ADC_Update();
}

/* ============================================================================
 * Hall task
 * ========================================================================== */

void MCU_System_TaskHall(void)
{
    MCU_Hall_Update();
}

/* ============================================================================
 * Input task
 * ========================================================================== */

void MCU_System_TaskInput(void)
{
    MCU_Input_Update();
}

/* ============================================================================
 * CAN task
 * ========================================================================== */

void MCU_System_TaskCAN(void)
{
    /*
     * Supervise incoming VCU communication.
     */
    MCU_CAN_Update();

    /*
     * Periodic MCU status frame.
     */
    MCU_CAN_SendStatus();

    /*
     * Periodic MCU heartbeat.
     */
    MCU_CAN_SendHeartbeat();

    /*
     * Diagnostic information is included periodically.
     */
    MCU_CAN_SendDiagnostics();
}

/* ============================================================================
 * Fault task
 * ========================================================================== */

void MCU_System_TaskFaults(void)
{
    MCU_Faults_Update();
}

/* ============================================================================
 * State task
 * ========================================================================== */

void MCU_System_TaskState(void)
{
    MCU_State_Update();
}

/* ============================================================================
 * Motor task
 * ========================================================================== */

void MCU_System_TaskMotor(void)
{
    MCU_Motor_Update();
}

/* ============================================================================
 * LCD task
 * ========================================================================== */

void MCU_System_TaskLCD(void)
{
    MCU_LCD_Update();
}

/* ============================================================================
 * Debug task
 * ========================================================================== */

void MCU_System_TaskDebug(void)
{
    MCU_Debug_Update();
}

/* ============================================================================
 * System supervision
 * ========================================================================== */

void MCU_System_Supervise(void)
{
    uint32_t now;

    now = HAL_GetTick();

    /*
     * Record supervision time.
     */
    mcu_system_last_supervision_time = now;

    MCU_Data.task.last_supervision_time = now;

    /*
     * Required task status.
     *
     * LCD is not considered a safety-critical condition. Debug UART is
     * also not safety-critical.
     */
    if (!MCU_Data.task.adc_ok)
    {
        MCU_Data.task.supervision_fault = MCU_TRUE;
    }
    else if (!MCU_Data.task.can_ok)
    {
        MCU_Data.task.supervision_fault = MCU_TRUE;
    }
    else if (!MCU_Data.task.motor_ok)
    {
        MCU_Data.task.supervision_fault = MCU_TRUE;
    }
    else if (!MCU_Data.task.input_ok)
    {
        MCU_Data.task.supervision_fault = MCU_TRUE;
    }
    else
    {
        MCU_Data.task.supervision_fault = MCU_FALSE;
    }

    /*
     * Overall system health.
     *
     * The state manager remains the authoritative source for the actual
     * operating state.
     */
    MCU_Data.system_healthy =
        MCU_State_IsSystemHealthy();
}

/* ============================================================================
 * System initialized status
 * ========================================================================== */

uint8_t MCU_System_IsInitialized(void)
{
    return mcu_system_initialized;
}

/* ============================================================================
 * System health status
 * ========================================================================== */

uint8_t MCU_System_IsHealthy(void)
{
    if (!mcu_system_initialized)
    {
        return MCU_FALSE;
    }

    return MCU_Data.system_healthy;
}

/* ============================================================================
 * System uptime
 * ========================================================================== */

uint32_t MCU_System_GetUptime(void)
{
    return mcu_system_uptime_ms;
}

/* ============================================================================
 * Shutdown
 * ========================================================================== */

void MCU_System_Shutdown(void)
{
    /*
     * Always put the motor hardware in its safest state first.
     */
    MCU_Motor_SafeStop();

    /*
     * Mark the application as shutting down.
     */
    MCU_Data.motor.state = MCU_STATE_SHUTDOWN;
    MCU_Data.motor.enabled = MCU_DISABLED;
    MCU_Data.motor.pwm_percent = 0U;

    MCU_Data.system_healthy = MCU_FALSE;

    mcu_system_initialized = MCU_FALSE;
}
