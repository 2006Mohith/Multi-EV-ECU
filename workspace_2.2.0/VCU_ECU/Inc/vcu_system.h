#ifndef VCU_SYSTEM_H
#define VCU_SYSTEM_H

#include <stdint.h>

#include "vcu_data.h"

/*
 * Initialize the complete VCU application state.
 */
void VCU_System_Init(void);

/*
 * Execute the complete VCU control cycle.
 */
void VCU_System_Update(void);

/*
 * Apply the current software outputs to the STM32 GPIOs.
 */
void VCU_System_UpdateOutputs(void);

/*
 * Returns 1 when the VCU has valid BMS and MCU communication
 * and is not in a fault condition.
 */
uint8_t VCU_System_IsHealthy(void);

#endif /* VCU_SYSTEM_H */
