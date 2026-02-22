#include "ic_vl53l0x.h"
#include <string.h>

/* -------------------------------------------------------------------------- */
/*  Register addresses (subset, enough for simple single-shot ranging)        */
/* -------------------------------------------------------------------------- */

#define ic_VL53L0X_REG_SYSRANGE_START              0x00U
#define ic_VL53L0X_REG_SYSTEM_INTERRUPT_CONFIG_GPIO 0x0AU
#define ic_VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR      0x0BU
#define ic_VL53L0X_REG_RESULT_INTERRUPT_STATUS     0x13U
#define ic_VL53L0X_REG_RESULT_RANGE_STATUS         0x14U
#define ic_VL53L0X_REG_IDENTIFICATION_MODEL_ID     0xC0U

/* RESULT_RANGE_STATUS + 10 會是 16-bit 距離值的位置 (mm) */
#define ic_VL53L0X_REG_RESULT_DISTANCE_MILLI_HI    (ic_VL53L0X_REG_RESULT_RANGE_STATUS + 10U)

/* -------------------------------------------------------------------------- */
/*  Global handle                                                             */
/* -------------------------------------------------------------------------- */

ic_vl53l0x_handle_t g_vl53l0x = {
    .hi2c           = NULL,
    .i2c_addr       = ic_VL53L0X_I2C_ADDR_DEFAULT,
    .is_initialized = 0U
};

/* -------------------------------------------------------------------------- */
/*  Local helper functions                                                    */
/* -------------------------------------------------------------------------- */

static ic_vl53l0x_status_t prv_i2c_read_reg(ic_vl53l0x_handle_t *dev,
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
                         ic_VL53L0X_RANGE_TIMEOUT_MS) != HAL_OK)
    {
        return VL53L0X_ERROR;
    }

    return VL53L0X_OK;
}

static ic_vl53l0x_status_t prv_i2c_write_reg(ic_vl53l0x_handle_t *dev,
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
                          ic_VL53L0X_RANGE_TIMEOUT_MS) != HAL_OK)
    {
        return VL53L0X_ERROR;
    }

    return VL53L0X_OK;
}

/* -------------------------------------------------------------------------- */
/*  Public functions                                                          */
/* -------------------------------------------------------------------------- */

ic_vl53l0x_status_t ic_vl53l0x_init(ic_vl53l0x_handle_t  *dev,
                              I2C_HandleTypeDef *hi2c,
                              uint8_t            i2c_addr)
{
    uint8_t model_id = 0U;
    uint8_t tmp      = 0U;

    if ((dev == NULL) || (hi2c == NULL))
    {
        return VL53L0X_ERROR;
    }

    dev->hi2c           = hi2c;
    dev->i2c_addr       = i2c_addr;
    dev->is_initialized = 0U;

    /* 1) 簡單讀 model ID，確認 I2C 有通 */
    if (prv_i2c_read_reg(dev,
                         ic_VL53L0X_REG_IDENTIFICATION_MODEL_ID,
                         &model_id,
                         1U) != VL53L0X_OK)
    {
        return VL53L0X_ERROR;
    }

    /* 大部分模組這裡會回 0xEE，但有些 clone 可能不是，這邊只要能讀到就當 OK */
    (void)model_id;

    /* 2) 設定 GPIO interrupt 行為：新 sample ready 時觸發
     *    這裡即使你不用實體 GPIO 也沒關係，方便我們用 STATUS 暫存器判斷。
     */
    tmp = 0x04U; /* "new sample ready" */
    (void)prv_i2c_write_reg(dev,
                            ic_VL53L0X_REG_SYSTEM_INTERRUPT_CONFIG_GPIO,
                            &tmp,
                            1U);

    /* 3) 清一次中斷旗標，保證狀態乾淨 */
    tmp = 0x01U;
    (void)prv_i2c_write_reg(dev,
                            ic_VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR,
                            &tmp,
                            1U);

    dev->is_initialized = 1U;
    return VL53L0X_OK;
}

ic_vl53l0x_status_t ic_vl53l0x_read_distance_mm(ic_vl53l0x_handle_t *dev,
                                          uint16_t         *distance_mm)
{
    uint8_t  buf[2] = {0U};
    uint8_t  status_reg;
    uint8_t  cmd;
    uint32_t start_tick;

    if ((dev == NULL) || (distance_mm == NULL))
    {
        return VL53L0X_ERROR;
    }

    if (dev->is_initialized == 0U)
    {
        return VL53L0X_NOT_INITIALIZED;
    }

    /* 1) 啟動單次量測 (single shot): SYSRANGE_START = 0x01 */
    cmd = 0x01U;
    if (prv_i2c_write_reg(dev,
                          ic_VL53L0X_REG_SYSRANGE_START,
                          &cmd,
                          1U) != VL53L0X_OK)
    {
        return VL53L0X_ERROR;
    }

    /* 2) 等待量測完成：poll RESULT_INTERRUPT_STATUS 直到低 3 bits 非 0 或 timeout */
    start_tick = HAL_GetTick();
    do
    {
        if (prv_i2c_read_reg(dev,
                             ic_VL53L0X_REG_RESULT_INTERRUPT_STATUS,
                             &status_reg,
                             1U) != VL53L0X_OK)
        {
            return VL53L0X_ERROR;
        }

        if ((status_reg & 0x07U) != 0U)
        {
            break; /* 有新數據 */
        }
    } while ((HAL_GetTick() - start_tick) < ic_VL53L0X_RANGE_TIMEOUT_MS);

    if ((status_reg & 0x07U) == 0U)
    {
        /* timeout，沒有等到完成 */
        return VL53L0X_TIMEOUT;
    }

    /* 3) 從 RESULT_RANGE_STATUS + 10 讀取 16-bit 距離 (mm) */
    if (prv_i2c_read_reg(dev,
                         ic_VL53L0X_REG_RESULT_DISTANCE_MILLI_HI,
                         buf,
                         2U) != VL53L0X_OK)
    {
        return VL53L0X_ERROR;
    }

    *distance_mm = (uint16_t)(((uint16_t)buf[0] << 8) | (uint16_t)buf[1]);

    /* 4) 清中斷旗標，下一次量測前要先清掉 */
    cmd = 0x01U;
    (void)prv_i2c_write_reg(dev,
                            ic_VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR,
                            &cmd,
                            1U);

    return VL53L0X_OK;
}

ic_vl53l0x_status_t ic_vl53l0x_get_distance_mm(uint16_t *distance_mm)
{
    return ic_vl53l0x_read_distance_mm(&g_vl53l0x, distance_mm);
}
