#ifndef IC_VL53L0X_H
#define IC_VL53L0X_H

#include <stdint.h>

#include "stm32f4xx_hal.h"
#include "ic_status.h"

#ifdef __cplusplus
extern "C" {
#endif

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
 * @brief Initializes a VL53L0X handle.
 *
 * @param[in,out] dev      VL53L0X handle to initialize.
 * @param[in]     hi2c     HAL I2C bus handle used by this device.
 * @param[in]     i2c_addr VL53L0X address in HAL 8-bit format.
 *
 * @return IC_STATUS_OK on success, otherwise:
 *         - IC_STATUS_BOARD_CONFIG_ERROR    : dev 或 hi2c 為 NULL
 *         - IC_STATUS_BUS_ERROR / I2C_xxx   : I2C 讀寫失敗
 *         - IC_STATUS_VL53L0X_NOT_DETECTED  : model ID 不正確
 *         - IC_STATUS_VL53L0X_INIT_FAILED   : 其他初始化流程錯誤
 */
ic_status_t
ic_vl53l0x_init (ic_vl53l0x_handle_t* dev,
                 I2C_HandleTypeDef* hi2c,
                 uint8_t i2c_addr);

/**
 * @brief Performs a single-shot ranging measurement.
 *
 * @param[in]  dev         Initialized VL53L0X handle.
 * @param[out] distance_mm Measured distance in millimeters.
 *
 * @return IC_STATUS_OK on success, otherwise:
 *         - IC_STATUS_BOARD_CONFIG_ERROR    : dev 或 distance_mm 為 NULL
 *         - IC_STATUS_VL53L0X_INIT_FAILED   : 尚未初始化
 *         - IC_STATUS_BUS_ERROR / I2C_xxx   : I2C 讀寫失敗
 *         - IC_STATUS_VL53L0X_READ_FAILED   : 狀態 / 距離暫存器讀取失敗
 *         - IC_STATUS_VL53L0X_OUT_OF_RANGE  : 量測超出有效範圍
 */
ic_status_t
ic_vl53l0x_read_distance_mm (ic_vl53l0x_handle_t* dev,
                             uint16_t* distance_mm);

/**
 * @brief Reads distance using the global VL53L0X handle.
 *
 * @param[out] distance_mm Measured distance in millimeters.
 *
 * @return Same as ic_vl53l0x_read_distance_mm().
 */
ic_status_t
ic_vl53l0x_get_distance_mm (uint16_t* distance_mm);

#ifdef __cplusplus
}
#endif

#endif /* IC_VL53L0X_H */