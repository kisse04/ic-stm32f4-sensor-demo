#ifndef IC_VL53L0X_H
#define IC_VL53L0X_H

#include <stdint.h>

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/** VL53L0X API return status. */
typedef enum {
    /** Operation completed successfully. */
    IC_VL53L0X_OK = 0,
    /** Generic I2C or device operation error. */
    IC_VL53L0X_ERROR = -1,
    /** Measurement did not complete before timeout. */
    IC_VL53L0X_TIMEOUT = -2,
    /** API called before successful initialization. */
    IC_VL53L0X_NOT_INITIALIZED = -3
} ic_vl53l0x_status_t;

/** VL53L0X device handle. */
typedef struct {
    /** HAL I2C handle used for sensor communication. */
    I2C_HandleTypeDef *hi2c;
    /** 8-bit I2C device address format used by HAL APIs. */
    uint8_t            i2c_addr;
    /** Initialization flag: 0 = not ready, 1 = ready. */
    uint8_t            is_initialized;
} ic_vl53l0x_handle_t;

/** Default I2C address in HAL 8-bit format. */
#define IC_VL53L0X_I2C_ADDR_DEFAULT   (0x29U << 1)

/** Timeout for one ranging operation in milliseconds. */
#define IC_VL53L0X_RANGE_TIMEOUT_MS   (100U)

/** Global VL53L0X handle used by convenience APIs. */
extern ic_vl53l0x_handle_t g_ic_vl53l0x;

/**
 * Initializes a VL53L0X handle.
 *
 * @param[in,out] dev is the VL53L0X handle to initialize.
 * @param[in] hi2c is the HAL I2C bus handle used by this device.
 * @param[in] i2c_addr is the VL53L0X address in HAL 8-bit format.
 *
 * @return IC_VL53L0X_OK if successful, otherwise an error status.
 */
ic_vl53l0x_status_t
ic_vl53l0x_init (ic_vl53l0x_handle_t* dev,
                 I2C_HandleTypeDef* hi2c,
                 uint8_t i2c_addr);

/**
 * Performs a single-shot ranging measurement.
 *
 * @param[in] dev is the initialized VL53L0X handle.
 * @param[out] distance_mm is the measured distance in millimeters.
 *
 * @return IC_VL53L0X_OK if successful, otherwise an error status.
 */
ic_vl53l0x_status_t
ic_vl53l0x_read_distance_mm (ic_vl53l0x_handle_t* dev,
                             uint16_t* distance_mm);

/**
 * Reads distance using the global VL53L0X handle.
 *
 * @param[out] distance_mm is the measured distance in millimeters.
 *
 * @return IC_VL53L0X_OK if successful, otherwise an error status.
 */
ic_vl53l0x_status_t
ic_vl53l0x_get_distance_mm (uint16_t* distance_mm);

#ifdef __cplusplus
}
#endif

#endif /* IC_VL53L0X_H */
