#ifndef BMS_TEMPERATURE_H
#define BMS_TEMPERATURE_H

#include <stdint.h>

/*
 * ============================================================
 * BMS TEMPERATURE MODULE
 * ============================================================
 *
 * ADC2:
 *   PA2 -> Temperature sensor
 *
 * Responsibilities:
 *   - Temperature ADC acquisition
 *   - Filtering
 *   - Sensor-voltage conversion
 *   - Plausibility checking
 *   - Temperature validity status
 *   - Temperature sensor fault indication
 */


/* ============================================================
 * INITIALIZATION
 * ============================================================ */

/**
 * @brief Initialize the temperature monitoring module.
 */
void BMS_Temperature_Init(void);


/* ============================================================
 * UPDATE
 * ============================================================ */

/**
 * @brief Read and process the battery temperature.
 */
void BMS_Temperature_Update(void);

#endif /* BMS_TEMPERATURE_H */
