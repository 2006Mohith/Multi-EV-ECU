#ifndef BMS_SOH_H
#define BMS_SOH_H

/*
 * ============================================================
 * BMS STATE OF HEALTH MODULE
 * ============================================================
 *
 * Responsibilities:
 *   - SOH initialization
 *   - SOH calculation/update
 *   - SOH range management
 *
 * Current implementation:
 *   Uses the configured initial SOH until actual battery
 *   capacity/aging measurements are available.
 */


/**
 * @brief Initialize the State of Health module.
 */
void BMS_SOH_Init(void);


/**
 * @brief Update the State of Health estimate.
 */
void BMS_SOH_Update(void);

#endif /* BMS_SOH_H */
