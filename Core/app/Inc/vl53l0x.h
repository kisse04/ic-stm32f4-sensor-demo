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
 *
 * 用來保存 I2C handle & I2C address 等狀態，
 * 目前先假設只有一顆 TOF，必要時可以再多開多個 instance。
 */
typedef struct
{
    I2C_HandleTypeDef *hi2c;
    uint8_t            i2c_addr;       /* 7-bit address << 1，預設 0x29 << 1 */
    uint8_t            is_initialized; /* 0 = not ready, 1 = ready */
} vl53l0x_handle_t;

/**
 * @brief 預設 I2C 位址 (HAL 參數用的左移一位形式)
 */
#define VL53L0X_I2C_ADDR_DEFAULT   (0x29U << 1)

/**
 * @brief 專案全域使用的 TOF handle（可選）
 *
 * 如果你整個專案只會有一顆 VL53L0X，可以直接用這個全域物件，
 * 在 app.c 裡面 include "vl53l0x.h" 就可以用了。
 */
extern vl53l0x_handle_t g_vl53l0x;

/**
 * @brief 初始化 VL53L0X
 *
 * @param dev      指向 handle 的指標（可用 &g_vl53l0x）
 * @param hi2c     指向要使用的 I2C handle（例如 &hi2c1 或 &hi2c2）
 * @param i2c_addr I2C 7bit addr 左移一位（預設可用 VL53L0X_I2C_ADDR_DEFAULT）
 *
 * @return VL53L0X_OK 表示成功
 *
 * TODO: 目前只做基本通訊確認 & 狀態設置，
 *       真正的 tuning / timing config 請依 datasheet 或 ST 官方 Driver 補上。
 */
vl53l0x_status_t vl53l0x_init(vl53l0x_handle_t     *dev,
                              I2C_HandleTypeDef    *hi2c,
                              uint8_t               i2c_addr);

/**
 * @brief 讀取單次測距結果 (blocking)
 *
 * @param dev          handle
 * @param distance_mm  輸出距離 (mm)
 *
 * @return VL53L0X_OK  表示成功
 *
 * TODO: 這裡目前是「骨架」，實際量測流程 (start ranging, wait, read result)
 *       需依 VL53L0X 的 register sequence 補上。
 */
vl53l0x_status_t vl53l0x_read_distance_mm(vl53l0x_handle_t *dev,
                                          uint16_t         *distance_mm);

/**
 * @brief 簡單方便函式：用全域 g_vl53l0x 讀距離
 *
 * @param distance_mm  輸出距離 (mm)
 * @return VL53L0X_OK  表示成功
 *
 * 可讓 app.c 裡面簡化成：
 *   uint16_t d_mm;
 *   if (vl53l0x_get_distance_mm(&d_mm) == VL53L0X_OK) { ... }
 */
vl53l0x_status_t vl53l0x_get_distance_mm(uint16_t *distance_mm);

#ifdef __cplusplus
}
#endif

#endif /* VL53L0X_H */
