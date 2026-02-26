#ifndef IC_STATUS_H
#define IC_STATUS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Application status codes.
 *
 * 0: OK
 * 11-19: Board / system
 * 21-29: Bus (I2C/SPI/UART)
 * 31-39: LED
 * 41-49: BMP280
 * 51-59: VL53L0X
 * 99: Unknown
 */
typedef enum {
    /** No error. */
    IC_STATUS_OK = 0,

    /** Board / system errors. */
    IC_STATUS_BOARD_INIT_FAILED      = 11,
    IC_STATUS_BOARD_UNSUPPORTED_HW   = 12,
    IC_STATUS_BOARD_CONFIG_ERROR     = 13,
    IC_STATUS_BOARD_POWER_FAULT      = 14,

    /** Bus errors. */
    IC_STATUS_BUS_ERROR              = 21,
    IC_STATUS_I2C_NACK               = 22,
    IC_STATUS_I2C_TIMEOUT            = 23,
    IC_STATUS_I2C_BUSY               = 24,

    /** LED errors. */
    IC_STATUS_LED_INVALID_CHANNEL    = 31,
    IC_STATUS_LED_GPIO_ERROR         = 32,

    /** BMP280 errors. */
    IC_STATUS_BMP280_INIT_FAILED     = 41,
    IC_STATUS_BMP280_NOT_DETECTED    = 42,
    IC_STATUS_BMP280_READ_FAILED     = 43,
    IC_STATUS_BMP280_CALIB_INVALID   = 44,

    /** VL53L0X errors. */
    IC_STATUS_VL53L0X_INIT_FAILED    = 51,
    IC_STATUS_VL53L0X_NOT_DETECTED   = 52,
    IC_STATUS_VL53L0X_READ_FAILED    = 53,
    IC_STATUS_VL53L0X_OUT_OF_RANGE   = 54,

    /** Fallback error. */
    IC_STATUS_UNKNOWN                = 99
} ic_status_t;

/**
 * Converts a status code into a string representation.
 *
 * @param[in] status is the status code.
 *
 * @return String representation for the status code.
 */
const char*
IC_Status_ToString (ic_status_t status);

/**
 * Returns a category string for a status code.
 *
 * @param[in] status is the status code.
 *
 * @return Category name string (Board, Bus, LED, BMP280, VL53L0X, Unknown).
 */
const char*
IC_Status_CategoryString (ic_status_t status);

/** Status helper macros. */
#define IC_STATUS_IS_OK(s)          ((s) == IC_STATUS_OK)
#define IC_STATUS_IS_ERROR(s)       ((s) != IC_STATUS_OK)

#define IC_STATUS_IS_BOARD_ERR(s)   ((s) >= 11 && (s) <= 19)
#define IC_STATUS_IS_BUS_ERR(s)     ((s) >= 21 && (s) <= 29)
#define IC_STATUS_IS_LED_ERR(s)     ((s) >= 31 && (s) <= 39)
#define IC_STATUS_IS_BMP280_ERR(s)  ((s) >= 41 && (s) <= 49)
#define IC_STATUS_IS_VL53L0X_ERR(s) ((s) >= 51 && (s) <= 59)

#ifdef __cplusplus
}
#endif

#endif /* IC_STATUS_H */
