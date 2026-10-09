#ifndef BMS_LCD_H
#define BMS_LCD_H

#include <stdint.h>

/*
 * ============================================================
 * BMS 16x2 I2C LCD MODULE
 * ============================================================
 *
 * LCD interface:
 *   16x2 character LCD
 *   I2C backpack, typically PCF8574-based
 *
 * Responsibilities:
 *   - LCD initialization
 *   - LCD command/data transfer
 *   - Display BMS measurements
 *   - Display BMS operating state
 *   - Display fault information
 */


/* ============================================================
 * INITIALIZATION
 * ============================================================ */

/**
 * @brief Initialize the BMS LCD.
 */
void BMS_LCD_Init(void);


/* ============================================================
 * DISPLAY UPDATE
 * ============================================================ */

/**
 * @brief Update the LCD with current BMS information.
 */
void BMS_LCD_Update(void);


/* ============================================================
 * BASIC LCD FUNCTIONS
 * ============================================================ */

/**
 * @brief Clear the LCD.
 */
void BMS_LCD_Clear(void);


/**
 * @brief Position cursor.
 *
 * @param row LCD row: 0 or 1
 * @param column LCD column: 0 to 15
 */
void BMS_LCD_SetCursor(uint8_t row, uint8_t column);


/**
 * @brief Print a string to the LCD.
 *
 * @param text Null-terminated string.
 */
void BMS_LCD_Print(const char *text);

#endif /* BMS_LCD_H */
