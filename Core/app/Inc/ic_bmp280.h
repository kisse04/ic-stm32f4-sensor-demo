#ifndef IC_BMP280_H
#define IC_BMP280_H

#include <stdint.h>

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/** BMP280 API return status. */
typedef enum {
    /** Operation completed successfully. */
    IC_BMP280_OK = 0,
    /** Generic I2C or device operation error. */
    IC_BMP280_ERROR = -1,
    /** Unexpected chip ID was read from the sensor. */
    IC_BMP280_BAD_ID = -2,
    /** API called before successful initialization. */
    IC_BMP280_NOT_INITIALIZED = -3
} ic_bmp280_status_t;

/** BMP280 device handle and calibration cache. */
typedef struct {
    /** HAL I2C handle used for sensor communication. */
    I2C_HandleTypeDef *hi2c;
    /** 8-bit I2C device address format used by HAL APIs. */
    uint8_t            i2c_addr;
    /** Temperature calibration coefficient T1. */
    uint16_t           T1;
    /** Temperature calibration coefficient T2. */
    int16_t            T2;
    /** Temperature calibration coefficient T3. */
    int16_t            T3;
    /** Initialization flag: 0 = not ready, 1 = ready. */
    uint8_t            is_initialized;
} ic_bmp280_handle_t;

/** Default I2C address in HAL 8-bit format. */
#define IC_BMP280_I2C_ADDR_DEFAULT   (0x76U << 1)

/**
 * Initializes a BMP280 handle and validates sensor identity.
 *
 * @param[in,out] dev is the BMP280 handle to initialize.
 * @param[in] hi2c is the HAL I2C bus handle used by this device.
 * @param[in] i2c_addr is the BMP280 device address in HAL 8-bit format.
 *
 * @return IC_BMP280_OK if successful, otherwise an error status.
 */
ic_bmp280_status_t
ic_bmp280_init (ic_bmp280_handle_t* dev,
                I2C_HandleTypeDef* hi2c,
                uint8_t i2c_addr);

/**
 * Reads and compensates current temperature from the sensor.
 *
 * @param[in] dev is the initialized BMP280 handle.
 * @param[out] temp_c is the measured temperature in degrees Celsius.
 *
 * @return IC_BMP280_OK if successful, otherwise an error status.
 */
ic_bmp280_status_t
ic_bmp280_read_temperature (ic_bmp280_handle_t* dev,
                            float* temp_c);

/** Global BMP280 handle used by convenience APIs. */
extern ic_bmp280_handle_t g_ic_bmp280;

/**
 * Reads temperature using the global BMP280 handle.
 *
 * @param[out] temp_c is the measured temperature in degrees Celsius.
 *
 * @return IC_BMP280_OK if successful, otherwise an error status.
 */
ic_bmp280_status_t
ic_bmp280_get_temperature (float* temp_c);

#ifdef __cplusplus
}
#endif

#endif /* IC_BMP280_H */
