/**
 * Status code helpers.
 */

#include "ic_status.h"

const char*
IC_Status_ToString (ic_status_t status)
{
    switch (status) {
    case IC_STATUS_OK:
        return "OK";

    /* Board / system. */
    case IC_STATUS_BOARD_INIT_FAILED:
        return "Board init failed";
    case IC_STATUS_BOARD_UNSUPPORTED_HW:
        return "Board unsupported hardware";
    case IC_STATUS_BOARD_CONFIG_ERROR:
        return "Board configuration error";
    case IC_STATUS_BOARD_POWER_FAULT:
        return "Board power fault";

    /* Bus / communication. */
    case IC_STATUS_BUS_ERROR:
        return "Bus error";
    case IC_STATUS_I2C_NACK:
        return "I2C NACK";
    case IC_STATUS_I2C_TIMEOUT:
        return "I2C timeout";
    case IC_STATUS_I2C_BUSY:
        return "I2C busy";

    /* LED. */
    case IC_STATUS_LED_INVALID_CHANNEL:
        return "LED invalid channel";
    case IC_STATUS_LED_GPIO_ERROR:
        return "LED GPIO error";

    /* BMP280. */
    case IC_STATUS_BMP280_INIT_FAILED:
        return "BMP280 init failed";
    case IC_STATUS_BMP280_NOT_DETECTED:
        return "BMP280 not detected";
    case IC_STATUS_BMP280_READ_FAILED:
        return "BMP280 read failed";
    case IC_STATUS_BMP280_CALIB_INVALID:
        return "BMP280 calibration invalid";

    /* VL53L0X. */
    case IC_STATUS_VL53L0X_INIT_FAILED:
        return "VL53L0X init failed";
    case IC_STATUS_VL53L0X_NOT_DETECTED:
        return "VL53L0X not detected";
    case IC_STATUS_VL53L0X_READ_FAILED:
        return "VL53L0X read failed";
    case IC_STATUS_VL53L0X_OUT_OF_RANGE:
        return "VL53L0X out of range";

    case IC_STATUS_UNKNOWN:
        return "Unknown status";

    default:
        return "Unrecognized status";
    }
}

const char*
IC_Status_CategoryString (ic_status_t status)
{
    if (IC_STATUS_IS_OK (status)) {
        return "OK";
    }
    if (IC_STATUS_IS_BOARD_ERR (status)) {
        return "Board";
    }
    if (IC_STATUS_IS_BUS_ERR (status)) {
        return "Bus";
    }
    if (IC_STATUS_IS_LED_ERR (status)) {
        return "LED";
    }
    if (IC_STATUS_IS_BMP280_ERR (status)) {
        return "BMP280";
    }
    if (IC_STATUS_IS_VL53L0X_ERR (status)) {
        return "VL53L0X";
    }

    return "Unknown";
}
