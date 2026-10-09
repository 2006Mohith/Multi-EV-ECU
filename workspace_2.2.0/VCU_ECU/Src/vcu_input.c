#include "vcu_input.h"

#include "gpio.h"

/* =========================================================
 * VCU DIGITAL INPUT PIN DEFINITIONS
 * ========================================================= */

#define VCU_IGNITION_PORT          GPIOB
#define VCU_IGNITION_PIN           GPIO_PIN_10

#define VCU_EMERGENCY_PORT         GPIOB
#define VCU_EMERGENCY_PIN          GPIO_PIN_11

#define VCU_FORWARD_PORT           GPIOB
#define VCU_FORWARD_PIN            GPIO_PIN_12

#define VCU_REVERSE_PORT           GPIOB
#define VCU_REVERSE_PIN            GPIO_PIN_13

/* =========================================================
 * PRIVATE HELPERS
 * ========================================================= */

static VCU_Direction_t VCU_Input_GetDirection(void)
{
    uint8_t forward_active;
    uint8_t reverse_active;

    forward_active =
        (VCU_Data.input.digital.forward == VCU_INPUT_ACTIVE) ? 1U : 0U;

    reverse_active =
        (VCU_Data.input.digital.reverse == VCU_INPUT_ACTIVE) ? 1U : 0U;

    if ((forward_active != 0U) && (reverse_active != 0U))
    {
        return VCU_DIR_NEUTRAL;
    }

    if (forward_active != 0U)
    {
        return VCU_DIR_FORWARD;
    }

    if (reverse_active != 0U)
    {
        return VCU_DIR_REVERSE;
    }

    return VCU_DIR_NEUTRAL;
}

/* =========================================================
 * INITIALIZATION
 * ========================================================= */

void VCU_Input_Init(void)
{
    VCU_Data.input.digital.ignition = VCU_INPUT_INACTIVE;
    VCU_Data.input.digital.emergency = VCU_INPUT_INACTIVE;
    VCU_Data.input.digital.forward = VCU_INPUT_INACTIVE;
    VCU_Data.input.digital.reverse = VCU_INPUT_INACTIVE;

    VCU_Data.input.direction = VCU_DIR_NEUTRAL;

    VCU_Data.input.accelerator_request = 0U;
    VCU_Data.input.brake_request = 0U;
}

/* =========================================================
 * DIGITAL INPUT UPDATE
 * ========================================================= */

void VCU_Input_UpdateDigital(void)
{
    /*
     * Current software assumption:
     *
     * GPIO_PIN_SET   = Active
     * GPIO_PIN_RESET = Inactive
     *
     * The final input polarity will be verified during
     * hardware integration.
     */

    VCU_Data.input.digital.ignition =
        (HAL_GPIO_ReadPin(VCU_IGNITION_PORT,
                          VCU_IGNITION_PIN) == GPIO_PIN_SET)
        ? VCU_INPUT_ACTIVE
        : VCU_INPUT_INACTIVE;

    VCU_Data.input.digital.emergency =
        (HAL_GPIO_ReadPin(VCU_EMERGENCY_PORT,
                          VCU_EMERGENCY_PIN) == GPIO_PIN_SET)
        ? VCU_INPUT_ACTIVE
        : VCU_INPUT_INACTIVE;

    VCU_Data.input.digital.forward =
        (HAL_GPIO_ReadPin(VCU_FORWARD_PORT,
                          VCU_FORWARD_PIN) == GPIO_PIN_SET)
        ? VCU_INPUT_ACTIVE
        : VCU_INPUT_INACTIVE;

    VCU_Data.input.digital.reverse =
        (HAL_GPIO_ReadPin(VCU_REVERSE_PORT,
                          VCU_REVERSE_PIN) == GPIO_PIN_SET)
        ? VCU_INPUT_ACTIVE
        : VCU_INPUT_INACTIVE;
}

/* =========================================================
 * DRIVER INPUT PROCESSING
 * ========================================================= */

void VCU_Input_Process(void)
{
    /*
     * Determine vehicle direction.
     */
    VCU_Data.input.direction = VCU_Input_GetDirection();

    /*
     * Accelerator request is permitted only when:
     *
     * 1. Ignition is active
     * 2. Emergency input is inactive
     * 3. Throttle input is valid
     * 4. Brake input is valid
     * 5. Brake command is not significantly active
     */

    if ((VCU_Data.input.digital.ignition == VCU_INPUT_ACTIVE) &&
        (VCU_Data.input.digital.emergency == VCU_INPUT_INACTIVE) &&
        (VCU_Data.input.analog.throttle_valid == VCU_INPUT_VALID) &&
        (VCU_Data.input.analog.brake_valid == VCU_INPUT_VALID) &&
        (VCU_Data.input.analog.brake_percent <=
         VCU_MAX_THROTTLE_WHILE_BRAKING))
    {
        VCU_Data.input.accelerator_request = 1U;
    }
    else
    {
        VCU_Data.input.accelerator_request = 0U;
    }

    /*
     * Brake request.
     */

    if ((VCU_Data.input.digital.ignition == VCU_INPUT_ACTIVE) &&
        (VCU_Data.input.analog.brake_valid == VCU_INPUT_VALID) &&
        (VCU_Data.input.analog.brake_percent >
         VCU_BRAKE_DEADZONE_PERCENT))
    {
        VCU_Data.input.brake_request = 1U;
    }
    else
    {
        VCU_Data.input.brake_request = 0U;
    }
}

/* =========================================================
 * INPUT PLAUSIBILITY
 * ========================================================= */

uint8_t VCU_Input_IsPlausible(void)
{
    /*
     * Forward + Reverse simultaneously active
     * is an invalid command.
     */

    if (VCU_Input_IsDirectionConflict() != 0U)
    {
        return 0U;
    }

    /*
     * Invalid analog inputs are unsafe.
     */

    if (VCU_Data.input.analog.throttle_valid != VCU_INPUT_VALID)
    {
        return 0U;
    }

    if (VCU_Data.input.analog.brake_valid != VCU_INPUT_VALID)
    {
        return 0U;
    }

    /*
     * Large simultaneous throttle and brake commands
     * are considered implausible.
     */

    if ((VCU_Data.input.analog.brake_percent >
         VCU_BRAKE_DEADZONE_PERCENT) &&
        (VCU_Data.input.analog.throttle_percent >
         VCU_MAX_THROTTLE_WHILE_BRAKING))
    {
        return 0U;
    }

    return 1U;
}

/* =========================================================
 * DIRECTION CONFLICT
 * ========================================================= */

uint8_t VCU_Input_IsDirectionConflict(void)
{
    if ((VCU_Data.input.digital.forward == VCU_INPUT_ACTIVE) &&
        (VCU_Data.input.digital.reverse == VCU_INPUT_ACTIVE))
    {
        return 1U;
    }

    return 0U;
}

/* =========================================================
 * EMERGENCY INPUT
 * ========================================================= */

uint8_t VCU_Input_IsEmergencyActive(void)
{
    if (VCU_Data.input.digital.emergency == VCU_INPUT_ACTIVE)
    {
        return 1U;
    }

    return 0U;
}

/* =========================================================
 * IGNITION INPUT
 * ========================================================= */

uint8_t VCU_Input_IsIgnitionActive(void)
{
    if (VCU_Data.input.digital.ignition == VCU_INPUT_ACTIVE)
    {
        return 1U;
    }

    return 0U;
}
