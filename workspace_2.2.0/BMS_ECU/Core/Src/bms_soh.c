/*
 * bms_soh.c
 *
 * BMS State of Health estimation module.
 *
 * SOH definition:
 *
 *     SOH (%) =
 *     (Available Capacity / Rated Capacity) x 100
 *
 * Current implementation:
 *   - Maintains a configurable initial SOH.
 *   - Keeps SOH within 0...100%.
 *   - Provides the framework for measured-capacity SOH.
 *
 * Future implementation:
 *   - Battery capacity test
 *   - Coulomb counting
 *   - Cycle aging
 *   - Internal resistance estimation
 *   - Temperature/aging compensation
 *
 * NOTE:
 * The rated capacity and available capacity used here are placeholders
 * until the actual battery specification and capacity measurement method
 * are finalized.
 */

#include "bms_soh.h"
#include "bms_data.h"
#include "bms_config.h"

#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* Private configuration                                                       */
/* -------------------------------------------------------------------------- */

/*
 * Placeholder rated battery capacity in Ah.
 *
 * Replace this with the actual battery-pack rated capacity once the
 * battery specification is finalized.
 */
#define BMS_RATED_CAPACITY_AH       20.0f

/* -------------------------------------------------------------------------- */
/* Private variables                                                          */
/* -------------------------------------------------------------------------- */

/*
 * Estimated available battery capacity in Ah.
 *
 * At present this remains equal to rated capacity because there is
 * no actual capacity-test or coulomb-counting mechanism yet.
 */
static float available_capacity_ah = 0.0f;

/*
 * SOH initialization status.
 */
static uint8_t soh_initialized = 0U;

/* -------------------------------------------------------------------------- */
/* Private functions                                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief Clamp SOH to the configured range.
 */
static float BMS_SOH_Clamp(float value)
{
    if (value < BMS_SOH_MIN)
    {
        return BMS_SOH_MIN;
    }

    if (value > BMS_SOH_MAX)
    {
        return BMS_SOH_MAX;
    }

    return value;
}

/**
 * @brief Calculate SOH from available and rated capacity.
 *
 * SOH (%) = (available / rated) x 100
 */
static float BMS_SOH_Calculate(
    float available_capacity,
    float rated_capacity)
{
    float soh;

    /*
     * Prevent division by zero or invalid rated capacity.
     */
    if (rated_capacity <= 0.0f)
    {
        return BMS_SOH_MIN;
    }

    /*
     * Calculate SOH.
     */
    soh =
        (available_capacity /
         rated_capacity) * 100.0f;

    /*
     * Keep result inside configured range.
     */
    return BMS_SOH_Clamp(soh);
}

/* -------------------------------------------------------------------------- */
/* Public functions                                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize SOH module.
 */
void BMS_SOH_Init(void)
{
    /*
     * Until an actual capacity measurement is available,
     * initialize available capacity to rated capacity.
     *
     * This produces an initial reference SOH of 100%.
     */
    available_capacity_ah =
        BMS_RATED_CAPACITY_AH;

    /*
     * Calculate initial SOH.
     */
    BMS_Data.state_of_health =
        BMS_SOH_Calculate(
            available_capacity_ah,
            BMS_RATED_CAPACITY_AH);

    /*
     * Mark module initialized.
     */
    soh_initialized = 1U;
}

/**
 * @brief Update SOH estimate.
 */
void BMS_SOH_Update(void)
{
    /*
     * Ensure initialization has occurred.
     */
    if (soh_initialized == 0U)
    {
        BMS_SOH_Init();
    }

    /*
     * --------------------------------------------------------------
     * Current implementation limitation
     * --------------------------------------------------------------
     *
     * There is currently no measured usable-capacity value.
     *
     * Therefore:
     *
     *     available capacity = rated capacity
     *
     * and:
     *
     *     SOH = 100%
     *
     * This is a commissioning/reference value, not an actual
     * battery-aging measurement.
     */
    BMS_Data.state_of_health =
        BMS_SOH_Calculate(
            available_capacity_ah,
            BMS_RATED_CAPACITY_AH);

    /*
     * Final defensive clamp.
     */
    BMS_Data.state_of_health =
        BMS_SOH_Clamp(
            BMS_Data.state_of_health);
}
