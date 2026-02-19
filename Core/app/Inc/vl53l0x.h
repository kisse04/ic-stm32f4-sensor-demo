#ifndef VL53L0X_H
#define VL53L0X_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdint.h>

/**
 * @brief VL53L0X return status
 */
typedef enum
{
    VL53L0X_OK = 0,
    VL53L0X_ERROR = -1,
    VL53L0X_TIMEOUT = -2,
    VL53L0X_NOT_INITIALIZED = -3
} vl53l0x_status_t;

/**
 * @brief VL53L0X device handle
 */
typedef struct
{
    I2C_HandleTypeDef *hi2c;
    uint8_t            i2c_addr;       /* 7-bit addr 左移一位（HAL 用） */
    uint8_t            is_initialized; /* 0 = not ready, 1 = ready */
} vl53l0x_handle_t;

/* 預設 I2C address (HAL 需要左移一位) */
#define VL53L0X_I2C_ADDR_DEFAULT   (0x29U << 1)

/* 單次量測 timeout (ms) */
#define VL53L0X_RANGE_TIMEOUT_MS   (100U)

/* 全域 handle：整個專案只用一顆的情況 */
extern vl53l0x_handle_t g_vl53l0x;

/* 初始化（簡化版） */
vl53l0x_status_t vl53l0x_init(vl53l0x_handle_t     *dev,
                              I2C_HandleTypeDef    *hi2c,
                              uint8_t               i2c_addr);

/* 觸發單次量測並讀出距離（mm） */
vl53l0x_status_t vl53l0x_read_distance_mm(vl53l0x_handle_t *dev,
                                          uint16_t         *distance_mm);

/* 方便函式：用全域 g_vl53l0x 讀距離 */
vl53l0x_status_t vl53l0x_get_distance_mm(uint16_t *distance_mm);

#ifdef __cplusplus
}
#endif

#endif /* VL53L0X_H */
