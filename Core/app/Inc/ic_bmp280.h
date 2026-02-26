#ifndef IC_BMP280_H
#define IC_BMP280_H

#include <stdint.h>

#include "stm32f4xx_hal.h"
#include "ic_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/** BMP280 device handle and calibration cache. */
typedef struct {
    /** HAL I2C handle used for sensor communication. */
    I2C_HandleTypeDef* hi2c;
    /** 8-bit I2C device address format used by HAL APIs. */
    uint8_t i2c_addr;
    /** Temperature calibration coefficient T1. */
    uint16_t T1;
    /** Temperature calibration coefficient T2. */
    int16_t T2;
    /** Temperature calibration coefficient T3. */
    int16_t T3;
    /** Initialization flag: 0 = not ready, 1 = ready. */
    uint8_t is_initialized;
} ic_bmp280_handle_t;

/** Default I2C address in HAL 8-bit format. */
#define IC_BMP280_I2C_ADDR_DEFAULT   (0x76U << 1)

/**
 * Initializes a BMP280 handle and validates sensor identity.
 *
 * @param[in,out] dev is the BMP280 handle to initialize.
 * @param[in]     hi2c is the HAL I2C bus handle used by this device.
 * @param[in]     i2c_addr is the BMP280 address in HAL 8-bit format.
 *
 * @return IC_STATUS_OK on success, otherwise:
 *         - IC_STATUS_BOARD_CONFIG_ERROR    : dev or hi2c is NULL
 *         - IC_STATUS_BUS_ERROR / IC_STATUS_I2C_* : I2C transfer failed
 *         - IC_STATUS_BMP280_NOT_DETECTED   : chip ID is not 0x58 or 0x60
 *         - IC_STATUS_BMP280_READ_FAILED    : calibration read failed
 */
ic_status_t
ic_bmp280_init (ic_bmp280_handle_t* dev,
                I2C_HandleTypeDef* hi2c,
                uint8_t i2c_addr);

/**
 * Reads and compensates current temperature from the sensor.
 *
 * @param[in]  dev is the initialized BMP280 handle.
 * @param[out] temp_c is the measured temperature in degrees Celsius.
 *
 * @return IC_STATUS_OK on success, otherwise:
 *         - IC_STATUS_BOARD_CONFIG_ERROR    : dev or temp_c is NULL
 *         - IC_STATUS_BMP280_INIT_FAILED    : device not initialized
 *         - IC_STATUS_BUS_ERROR / IC_STATUS_I2C_* : I2C transfer failed
 *         - IC_STATUS_BMP280_READ_FAILED    : raw read failed
 */
ic_status_t
ic_bmp280_read_temperature (ic_bmp280_handle_t* dev,
                            float* temp_c);

/** Global BMP280 handle used by convenience APIs. */
extern ic_bmp280_handle_t g_ic_bmp280;

/**
 * Reads temperature using the global BMP280 handle.
 *
 * @param[out] temp_c is the measured temperature in degrees Celsius.
 *
 * @return Same as ic_bmp280_read_temperature().
 */
ic_status_t
ic_bmp280_get_temperature (float* temp_c);

#ifdef __cplusplus
}
#endif

#endif /* IC_BMP280_H */
