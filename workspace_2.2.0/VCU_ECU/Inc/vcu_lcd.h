#ifndef VCU_LCD_H
#define VCU_LCD_H

#include <stdint.h>
#include "vcu_data.h"

/* =========================================================
 * LCD PAGES
 * ========================================================= */

typedef enum
{
    VCU_LCD_PAGE_BMS = 0U,
    VCU_LCD_PAGE_VCU,
    VCU_LCD_PAGE_MCU,
    VCU_LCD_PAGE_STATUS,
    VCU_LCD_PAGE_COUNT
} VCU_LCD_Page_t;

/* =========================================================
 * LCD INITIALIZATION
 * ========================================================= */

/*
 * Initialize the VCU LCD interface.
 *
 * LCD connection:
 *
 * PB6 -> I2C1 SCL
 * PB7 -> I2C1 SDA
 */
void VCU_LCD_Init(void);

/* =========================================================
 * LCD UPDATE
 * ========================================================= */

/*
 * Update the LCD dashboard.
 *
 * The LCD automatically displays the currently selected page.
 */
void VCU_LCD_Update(void);

/* =========================================================
 * LCD STATUS
 * ========================================================= */

/*
 * Returns 1 when the LCD was successfully initialized.
 */
uint8_t VCU_LCD_IsReady(void);

/* =========================================================
 * BASIC LCD FUNCTIONS
 * ========================================================= */

/*
 * Clear the LCD display.
 */
void VCU_LCD_Clear(void);

/*
 * Display a two-line message.
 */
void VCU_LCD_ShowMessage(const char *line1,
                         const char *line2);

/* =========================================================
 * ECU DASHBOARD PAGES
 * ========================================================= */

/*
 * Display BMS information.
 *
 * Example:
 *
 * BAT:48.2V SOC:82
 * I:12.4A T:32C
 */
void VCU_LCD_ShowBMS(void);

/*
 * Display VCU information.
 *
 * Example:
 *
 * VCU:DRIVE
 * THR:65 BRK:00
 */
void VCU_LCD_ShowVCU(void);

/*
 * Display MCU / motor-controller information.
 *
 * Example:
 *
 * MCU:RUN
 * RPM:1240 PWM:65
 */
void VCU_LCD_ShowMCU(void);

/*
 * Display overall communication and fault status.
 *
 * Example:
 *
 * BMS:OK MCU:OK
 * FAULT:00
 */
void VCU_LCD_ShowStatus(void);

/* =========================================================
 * PAGE CONTROL
 * ========================================================= */

/*
 * Select the LCD page.
 */
void VCU_LCD_SetPage(VCU_LCD_Page_t page);

/*
 * Return the currently selected page.
 */
VCU_LCD_Page_t VCU_LCD_GetPage(void);

/*
 * Move to the next LCD page.
 */
void VCU_LCD_NextPage(void);

/*
 * Request an immediate LCD refresh.
 */
void VCU_LCD_RequestUpdate(void);

/* =========================================================
 * AUTOMATIC PAGE ROTATION
 * ========================================================= */

/*
 * Enable automatic page rotation.
 */
void VCU_LCD_EnableAutoPage(void);

/*
 * Disable automatic page rotation.
 */
void VCU_LCD_DisableAutoPage(void);

/*
 * Returns 1 when automatic page rotation is enabled.
 */
uint8_t VCU_LCD_IsAutoPageEnabled(void);

#endif /* VCU_LCD_H */
