#include "vl53l0x.h"
#include <string.h>

/* -------------------------------------------------------------------------- */
/*  Local defines (registers)                                                 */
/* -------------------------------------------------------------------------- */

/*
 * 以下 registers 只是示意，實際值請依 datasheet 或 ST 官方 driver 確認。
 * 可以先用 WHO_AM_I 或類似 ID register 來做基本通訊確認。
 */
#define VL53L0X_REG_IDENTIFICATION_MODEL_ID   0xC0U   /* TODO: 依 datasheet 確認 */
#define VL53L0X_REG_RESULT_RANGE_STATUS      0x14U   /* TODO */
#define VL53L0X_REG_RESULT_RANGE_MILLI_MSB   0x1EU   /* TODO */
#define VL53L0X_REG_RESULT_RANGE_MILLI_LSB   0x1FU   /* TODO */

/* 測試使用時間，實際需依 timing budget 調整 */
#define VL53L0X_I2C_TIMEOUT_MS               50U

/* -------------------------------------------------------------------------- */
/*  Global handle                                                             */
/* -------------------------------------------------------------------------- */

vl53l0x_handle_t g_vl53l0x = {
    .hi2c           = NULL,
    .i2c_addr       = VL53L0X_I2C_ADDR_DEFAULT,
    .is_initialized = 0U
};

/* -------------------------------------------------------------------------- */
/*  Local helper functions                                                    */
/* -------------------------------------------------------------------------- */

static vl53l0x_status_t prv_i2c_read_reg(vl53l0x_handle_t *dev,
                                         uint8_t           reg,
                                         uint8_t          *p_data,
                                         uint16_t          size)
{
    if ((dev == NULL) || (dev->hi2c == NULL))
    {
        return VL53L0X_ERROR;
    }

    if (HAL_I2C_Mem_Read(dev->hi2c,
                         dev->i2c_addr,
                         reg,
                         I2C_MEMADD_SIZE_8BIT,
                         p_data,
                         size,
                         VL53L0X_I2C_TIMEOUT_MS) != HAL_OK)
    {
        return VL53L0X_ERROR;
    }

    return VL53L0X_OK;
}

static vl53l0x_status_t prv_i2c_write_reg(vl53l0x_handle_t *dev,
                                          uint8_t           reg,
                                          const uint8_t    *p_data,
                                          uint16_t          size)
{
    if ((dev == NULL) || (dev->hi2c == NULL))
    {
        return VL53L0X_ERROR;
    }

    if (HAL_I2C_Mem_Write(dev->hi2c,
                          dev->i2c_addr,
                          reg,
                          I2C_MEMADD_SIZE_8BIT,
                          (uint8_t *)p_data,
                          size,
                          VL53L0X_I2C_TIMEOUT_MS) != HAL_OK)
    {
        return VL53L0X_ERROR;
    }

    return VL53L0X_OK;
}

/* -------------------------------------------------------------------------- */
/*  Public functions                                                          */
/* -------------------------------------------------------------------------- */

vl53l0x_status_t vl53l0x_init(vl53l0x_handle_t  *dev,
                              I2C_HandleTypeDef *hi2c,
                              uint8_t            i2c_addr)
{
    uint8_t model_id = 0U;

    if ((dev == NULL) || (hi2c == NULL))
    {
        return VL53L0X_ERROR;
    }

    dev->hi2c           = hi2c;
    dev->i2c_addr       = i2c_addr;
    dev->is_initialized = 0U;

    /* 1) 最基礎：讀一個 ID register 確認 I2C 通了沒 */
    if (prv_i2c_read_reg(dev,
                         VL53L0X_REG_IDENTIFICATION_MODEL_ID,
                         &model_id,
                         1U) != VL53L0X_OK)
    {
        return VL53L0X_ERROR;
    }

    /* 2) TODO: 依 datasheet 檢查 model_id 是否符合預期值，
     *          例如應該是 0xEE 或其它固定值，若不符可以回傳 ERROR
     */
    (void)model_id; /* 暫時避免未使用警告 */

    /* 3) TODO: 依 ST 官方 Init 流程設定 timing budget、VCSEL pulse 等等 */
    /* 例如：
     *   - 關閉 GPIO
     *   - 配置測距模式 (single / continuous)
     *   - 設定測距時間 & 距離模式 (short/long)
     */

    dev->is_initialized = 1U;
    return VL53L0X_OK;
}

vl53l0x_status_t vl53l0x_read_distance_mm(vl53l0x_handle_t *dev,
                                          uint16_t         *distance_mm)
{
    if ((dev == NULL) || (distance_mm == NULL))
    {
        return VL53L0X_ERROR;
    }

    if (dev->is_initialized == 0U)
    {
        return VL53L0X_NOT_INITIALIZED;
    }

    /* TODO:
     *   1) 觸發一次測距 (Single Ranging mode)：
     *        - write 某個 start register
     *   2) 等待量測完成：
     *        - 讀狀態位 (VL53L0X_REG_RESULT_RANGE_STATUS)
     *        - 或簡單 delay 對應的 timing budget
     *   3) 讀出結果 register (mm)
     *
     * 下面的程式碼只是「範例骨架」，實際 register 地址 & bit 定義請依 datasheet 修改。
     */

    vl53l0x_status_t status;
    uint8_t raw_buf[2] = {0U};

    /* 這裡暫時假設量測已經在其它地方啟動（或是跑 continuous 模式），
     * 所以只示範「讀結果」的部分。
     */

    status = prv_i2c_read_reg(dev,
                              VL53L0X_REG_RESULT_RANGE_MILLI_MSB,
                              raw_buf,
                              2U);
    if (status != VL53L0X_OK)
    {
        return status;
    }

    /* 將高低位組合成 16bit 距離值 (mm) */
    *distance_mm = (uint16_t)(((uint16_t)raw_buf[0] << 8) | (uint16_t)raw_buf[1]);

    return VL53L0X_OK;
}

vl53l0x_status_t vl53l0x_get_distance_mm(uint16_t *distance_mm)
{
    return vl53l0x_read_distance_mm(&g_vl53l0x, distance_mm);
}
