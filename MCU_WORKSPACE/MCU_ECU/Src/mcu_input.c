#include "main.h"
#include "mcu_config.h"
#include "mcu_data.h"
#include "mcu_input.h"

/* ============================================================================
 * MCU Input Module
 * ============================================================================
 *
 * Button connections:
 *
 * PB10 -> START
 * PB11 -> STOP
 * PB12 -> FORWARD
 * PB13 -> REVERSE
 *
 * GPIO configuration:
 * Internal pull-up
 *
 * Released -> HIGH
 * Pressed  -> LOW
 *
 * Software debounce:
 * MCU_BUTTON_DEBOUNCE_MS
 *
 * ========================================================================== */

/* ============================================================================
 * Private button state
 * ========================================================================== */

typedef struct
{
    uint8_t raw_state;
    uint8_t stable_state;
    uint32_t last_change_time;
} MCU_InputButton_t;

static MCU_InputButton_t start_button;
static MCU_InputButton_t stop_button;
static MCU_InputButton_t forward_button;
static MCU_InputButton_t reverse_button;

/* ============================================================================
 * Private functions
 * ========================================================================== */

static uint8_t MCU_Input_ReadPressed(GPIO_TypeDef *GPIOx,
                                     uint16_t GPIO_Pin)
{
    /*
     * Buttons are active-low.
     */
    if (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_RESET)
    {
        return MCU_TRUE;
    }

    return MCU_FALSE;
}

static uint8_t MCU_Input_DebounceButton(MCU_InputButton_t *button,
                                        uint8_t raw_pressed,
                                        uint32_t now)
{
    if (button == NULL)
    {
        return MCU_FALSE;
    }

    /*
     * Raw state changed.
     */
    if (raw_pressed != button->raw_state)
    {
        button->raw_state = raw_pressed;
        button->last_change_time = now;
    }

    /*
     * Accept the new state only after the configured debounce period.
     *
     * Note:
     * The project configuration uses MCU_BUTTON_DEBOUNCE_MS.
     */
    if ((now - button->last_change_time) >= MCU_BUTTON_DEBOUNCE_MS)
    {
        button->stable_state = button->raw_state;
    }

    return button->stable_state;
}

/* ============================================================================
 * Initialization
 * ========================================================================== */

void MCU_Input_Init(void)
{
    uint32_t now;

    now = HAL_GetTick();

    start_button.raw_state = MCU_FALSE;
    start_button.stable_state = MCU_FALSE;
    start_button.last_change_time = now;

    stop_button.raw_state = MCU_FALSE;
    stop_button.stable_state = MCU_FALSE;
    stop_button.last_change_time = now;

    forward_button.raw_state = MCU_FALSE;
    forward_button.stable_state = MCU_FALSE;
    forward_button.last_change_time = now;

    reverse_button.raw_state = MCU_FALSE;
    reverse_button.stable_state = MCU_FALSE;
    reverse_button.last_change_time = now;

    MCU_Data.input.start_pressed = MCU_FALSE;
    MCU_Data.input.stop_pressed = MCU_FALSE;
    MCU_Data.input.forward_pressed = MCU_FALSE;
    MCU_Data.input.reverse_pressed = MCU_FALSE;

    MCU_Data.input.direction_conflict = MCU_FALSE;
    MCU_Data.input.inputs_valid = MCU_TRUE;

    MCU_Data.task.input_ok = MCU_TRUE;
}

/* ============================================================================
 * Periodic input update
 * ========================================================================== */

void MCU_Input_Update(void)
{
    uint32_t now;

    uint8_t start_raw;
    uint8_t stop_raw;
    uint8_t forward_raw;
    uint8_t reverse_raw;

    now = HAL_GetTick();

    /* ------------------------------------------------------------------------
     * Read raw GPIO states
     * ---------------------------------------------------------------------- */

    start_raw =
        MCU_Input_ReadPressed(GPIOB, GPIO_PIN_10);

    stop_raw =
        MCU_Input_ReadPressed(GPIOB, GPIO_PIN_11);

    forward_raw =
        MCU_Input_ReadPressed(GPIOB, GPIO_PIN_12);

    reverse_raw =
        MCU_Input_ReadPressed(GPIOB, GPIO_PIN_13);

    /* ------------------------------------------------------------------------
     * Debounce each button
     * ---------------------------------------------------------------------- */

    MCU_Data.input.start_pressed =
        MCU_Input_DebounceButton(
            &start_button,
            start_raw,
            now);

    MCU_Data.input.stop_pressed =
        MCU_Input_DebounceButton(
            &stop_button,
            stop_raw,
            now);

    MCU_Data.input.forward_pressed =
        MCU_Input_DebounceButton(
            &forward_button,
            forward_raw,
            now);

    MCU_Data.input.reverse_pressed =
        MCU_Input_DebounceButton(
            &reverse_button,
            reverse_raw,
            now);

    /* ------------------------------------------------------------------------
     * Direction conflict detection
     *
     * Both FORWARD and REVERSE pressed simultaneously is invalid.
     * ---------------------------------------------------------------------- */

    if (MCU_Data.input.forward_pressed &&
        MCU_Data.input.reverse_pressed)
    {
        MCU_Data.input.direction_conflict = MCU_TRUE;
        MCU_Data.input.inputs_valid = MCU_FALSE;

        MCU_Data.fault.direction_fault = MCU_TRUE;
        MCU_Data.fault.button_fault = MCU_TRUE;

        MCU_Data.task.input_ok = MCU_FALSE;
    }
    else
    {
        MCU_Data.input.direction_conflict = MCU_FALSE;
        MCU_Data.input.inputs_valid = MCU_TRUE;

        MCU_Data.fault.direction_fault = MCU_FALSE;
        MCU_Data.fault.button_fault = MCU_FALSE;

        MCU_Data.task.input_ok = MCU_TRUE;
    }
}

/* ============================================================================
 * Individual button status
 * ========================================================================== */

uint8_t MCU_Input_IsStartPressed(void)
{
    return MCU_Data.input.start_pressed;
}

uint8_t MCU_Input_IsStopPressed(void)
{
    return MCU_Data.input.stop_pressed;
}

uint8_t MCU_Input_IsForwardPressed(void)
{
    return MCU_Data.input.forward_pressed;
}

uint8_t MCU_Input_IsReversePressed(void)
{
    return MCU_Data.input.reverse_pressed;
}

/* ============================================================================
 * Direction conflict
 * ========================================================================== */

uint8_t MCU_Input_IsDirectionConflict(void)
{
    return MCU_Data.input.direction_conflict;
}

/* ============================================================================
 * Input validity
 * ========================================================================== */

uint8_t MCU_Input_AreInputsValid(void)
{
    return MCU_Data.input.inputs_valid;
}

/* ============================================================================
 * Requested direction
 * ========================================================================== */

uint8_t MCU_Input_GetRequestedDirection(void)
{
    /*
     * Both directions pressed -> invalid/neutral.
     */
    if (MCU_Data.input.direction_conflict)
    {
        return MCU_DIRECTION_NEUTRAL;
    }

    /*
     * Forward button pressed.
     */
    if (MCU_Data.input.forward_pressed)
    {
        return MCU_DIRECTION_FORWARD;
    }

    /*
     * Reverse button pressed.
     */
    if (MCU_Data.input.reverse_pressed)
    {
        return MCU_DIRECTION_REVERSE;
    }

    /*
     * Neither direction button pressed.
     */
    return MCU_DIRECTION_NEUTRAL;
}

/* ============================================================================
 * Clear button states
 * ========================================================================== */

void MCU_Input_Clear(void)
{
    uint32_t now;

    now = HAL_GetTick();

    start_button.raw_state = MCU_FALSE;
    start_button.stable_state = MCU_FALSE;
    start_button.last_change_time = now;

    stop_button.raw_state = MCU_FALSE;
    stop_button.stable_state = MCU_FALSE;
    stop_button.last_change_time = now;

    forward_button.raw_state = MCU_FALSE;
    forward_button.stable_state = MCU_FALSE;
    forward_button.last_change_time = now;

    reverse_button.raw_state = MCU_FALSE;
    reverse_button.stable_state = MCU_FALSE;
    reverse_button.last_change_time = now;

    MCU_Data.input.start_pressed = MCU_FALSE;
    MCU_Data.input.stop_pressed = MCU_FALSE;
    MCU_Data.input.forward_pressed = MCU_FALSE;
    MCU_Data.input.reverse_pressed = MCU_FALSE;

    MCU_Data.input.direction_conflict = MCU_FALSE;
    MCU_Data.input.inputs_valid = MCU_TRUE;

    MCU_Data.task.input_ok = MCU_TRUE;
}
