/**
 * VL53L0X time-of-flight sensor driver.
 *
 * This implementation provides a minimal single-shot ranging flow.
 */

#include "ic_vl53l0x.h"

/** Register addresses required for simplified ranging flow. */
#define IC_VL53L0X_REG_SYSRANGE_START                 0x00U
#define IC_VL53L0X_REG_SYSTEM_INTERRUPT_CONFIG_GPIO   0x0AU
#define IC_VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR         0x0BU
#define IC_VL53L0X_REG_RESULT_INTERRUPT_STATUS        0x13U
#define IC_VL53L0X_REG_RESULT_RANGE_STATUS            0x14U
#define IC_VL53L0X_REG_IDENTIFICATION_MODEL_ID        0xC0U

/** RESULT_RANGE_STATUS + 10 contains a 16-bit range result in millimeters. */
#define IC_VL53L0X_REG_RESULT_DISTANCE_MILLI_HI       (IC_VL53L0X_REG_RESULT_RANGE_STATUS + 10U)

/** Global VL53L0X handle for convenience APIs. */
ic_vl53l0x_handle_t g_ic_vl53l0x = {
    .hi2c = NULL,
    .i2c_addr = IC_VL53L0X_I2C_ADDR_DEFAULT,
    .is_initialized = 0U
};

/** Reads bytes from a VL53L0X register range. */
static ic_vl53l0x_status_t
ic_vl53l0x_i2c_read_reg (ic_vl53l0x_handle_t* dev,
                         uint8_t reg,
                         uint8_t* p_data,
                         uint16_t size)
{
    if ((dev == NULL) || (dev->hi2c == NULL)) {
        return IC_VL53L0X_ERROR;
    }

    if (HAL_I2C_Mem_Read (dev->hi2c,
                          dev->i2c_addr,
                          reg,
                          I2C_MEMADD_SIZE_8BIT,
                          p_data,
                          size,
                          IC_VL53L0X_RANGE_TIMEOUT_MS) != HAL_OK) {
        return IC_VL53L0X_ERROR;
    }

    return IC_VL53L0X_OK;
}

/** Writes bytes into a VL53L0X register range. */
static ic_vl53l0x_status_t
ic_vl53l0x_i2c_write_reg (ic_vl53l0x_handle_t* dev,
                          uint8_t reg,
                          const uint8_t* p_data,
                          uint16_t size)
{
    if ((dev == NULL) || (dev->hi2c == NULL)) {
        return IC_VL53L0X_ERROR;
    }

    if (HAL_I2C_Mem_Write (dev->hi2c,
                           dev->i2c_addr,
                           reg,
                           I2C_MEMADD_SIZE_8BIT,
                           (uint8_t*) p_data,
                           size,
                           IC_VL53L0X_RANGE_TIMEOUT_MS) != HAL_OK) {
        return IC_VL53L0X_ERROR;
    }

    return IC_VL53L0X_OK;
}

ic_vl53l0x_status_t
ic_vl53l0x_init (ic_vl53l0x_handle_t* dev,
                 I2C_HandleTypeDef* hi2c,
                 uint8_t i2c_addr)
{
    uint8_t model_id = 0U;
    uint8_t tmp = 0U;

    if ((dev == NULL) || (hi2c == NULL)) {
        return IC_VL53L0X_ERROR;
    }

    dev->hi2c = hi2c;
    dev->i2c_addr = i2c_addr;
    dev->is_initialized = 0U;

    if (ic_vl53l0x_i2c_read_reg (dev,
                                IC_VL53L0X_REG_IDENTIFICATION_MODEL_ID,
                                &model_id,
                                1U) != IC_VL53L0X_OK) {
        return IC_VL53L0X_ERROR;
    }

    /* Model ID is read for bus/device validation in this simplified flow. */
    (void) model_id;

    tmp = 0x04U;
    (void) ic_vl53l0x_i2c_write_reg (dev,
                                    IC_VL53L0X_REG_SYSTEM_INTERRUPT_CONFIG_GPIO,
                                    &tmp,
                                    1U);

    tmp = 0x01U;
    (void) ic_vl53l0x_i2c_write_reg (dev,
                                    IC_VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR,
                                    &tmp,
                                    1U);

    dev->is_initialized = 1U;
    return IC_VL53L0X_OK;
}

ic_vl53l0x_status_t
ic_vl53l0x_read_distance_mm (ic_vl53l0x_handle_t* dev,
                             uint16_t* distance_mm)
{
    uint8_t buf[2] = { 0U };
    uint8_t status_reg = 0U;
    uint8_t cmd = 0U;
    uint32_t start_tick = 0U;

    if ((dev == NULL) || (distance_mm == NULL)) {
        return IC_VL53L0X_ERROR;
    }
    if (dev->is_initialized == 0U) {
        return IC_VL53L0X_NOT_INITIALIZED;
    }

    cmd = 0x01U;
    if (ic_vl53l0x_i2c_write_reg (dev,
                                 IC_VL53L0X_REG_SYSRANGE_START,
                                 &cmd,
                                 1U) != IC_VL53L0X_OK) {
        return IC_VL53L0X_ERROR;
    }

    start_tick = HAL_GetTick ();
    do {
        if (ic_vl53l0x_i2c_read_reg (dev,
                                    IC_VL53L0X_REG_RESULT_INTERRUPT_STATUS,
                                    &status_reg,
                                    1U) != IC_VL53L0X_OK) {
            return IC_VL53L0X_ERROR;
        }

        if ((status_reg & 0x07U) != 0U) {
            break;
        }
    } while ((HAL_GetTick () - start_tick) < IC_VL53L0X_RANGE_TIMEOUT_MS);

    if ((status_reg & 0x07U) == 0U) {
        return IC_VL53L0X_TIMEOUT;
    }

    if (ic_vl53l0x_i2c_read_reg (dev,
                                IC_VL53L0X_REG_RESULT_DISTANCE_MILLI_HI,
                                buf,
                                2U) != IC_VL53L0X_OK) {
        return IC_VL53L0X_ERROR;
    }

    *distance_mm = (uint16_t) ((((uint16_t) buf[0]) << 8) | (uint16_t) buf[1]);

    cmd = 0x01U;
    (void) ic_vl53l0x_i2c_write_reg (dev,
                                    IC_VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR,
                                    &cmd,
                                    1U);

    return IC_VL53L0X_OK;
}

ic_vl53l0x_status_t
ic_vl53l0x_get_distance_mm (uint16_t* distance_mm)
{
    return ic_vl53l0x_read_distance_mm (&g_ic_vl53l0x, distance_mm);
}
