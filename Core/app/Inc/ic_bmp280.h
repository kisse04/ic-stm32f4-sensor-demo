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
 * @param[in,out] dev      BMP280 handle to initialize.
 * @param[in]     hi2c     HAL I2C bus handle used by this device.
 * @param[in]     i2c_addr BMP280 device address in HAL 8-bit format.
 *
 * @return IC_STATUS_OK on success, otherwise an ic_status_t error code.
 *         - IC_STATUS_BOARD_CONFIG_ERROR      : dev 或 hi2c 為 NULL
 *         - IC_STATUS_BUS_ERROR / I2C_xxx     : I2C 讀寫失敗
 *         - IC_STATUS_BMP280_NOT_DETECTED     : chip ID 非 0x58 / 0x60
 *         - IC_STATUS_BMP280_READ_FAILED      : 讀取校正常數失敗
 */
ic_status_t
ic_bmp280_init (ic_bmp280_handle_t* dev,
                I2C_HandleTypeDef* hi2c,
                uint8_t i2c_addr);

/**
 * Reads and compensates current temperature from the sensor.
 *
 * @param[in]  dev    Initialized BMP280 handle.
 * @param[out] temp_c Measured temperature in degrees Celsius.
 *
 * @return IC_STATUS_OK on success, otherwise an ic_status_t error code.
 *         - IC_STATUS_BOARD_CONFIG_ERROR      : dev 或 temp_c 為 NULL
 *         - IC_STATUS_BMP280_INIT_FAILED      : 尚未初始化 (is_initialized == 0)
 *         - IC_STATUS_BUS_ERROR / I2C_xxx     : I2C 讀寫失敗
 *         - IC_STATUS_BMP280_READ_FAILED      : 讀溫度暫存器失敗
 */
ic_status_t
ic_bmp280_read_temperature (ic_bmp280_handle_t* dev,
                            float* temp_c);

/** Global BMP280 handle used by convenience APIs. */
extern ic_bmp280_handle_t g_ic_bmp280;

/**
 * Reads temperature using the global BMP280 handle.
 *
 * @param[out] temp_c Measured temperature in degrees Celsius.
 *
 * @return Same as ic_bmp280_read_temperature().
 */
ic_status_t
ic_bmp280_get_temperature (float* temp_c);

#ifdef __cplusplus
}
#endif

#endif /* IC_BMP280_H */