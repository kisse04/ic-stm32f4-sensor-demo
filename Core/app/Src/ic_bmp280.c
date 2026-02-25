/**
 * BMP280 temperature sensor driver.
 *
 * This module provides a lightweight BMP280 initialization and
 * temperature read flow over STM32 HAL I2C APIs.
 */

#include "ic_bmp280.h"

#include <stdint.h>
#include <stdio.h>

/** BMP280 register map. */
#define BMP280_REG_ID         0xD0U
#define BMP280_REG_RESET      0xE0U
#define BMP280_REG_STATUS     0xF3U
#define BMP280_REG_CTRL_MEAS  0xF4U
#define BMP280_REG_CONFIG     0xF5U
#define BMP280_REG_TEMP_MSB   0xFAU

/** Temperature calibration register base addresses. */
#define BMP280_REG_CALIB_T1   0x88U
#define BMP280_REG_CALIB_T2   0x8AU
#define BMP280_REG_CALIB_T3   0x8CU

/** Global BMP280 handle for convenience APIs. */
ic_bmp280_handle_t g_ic_bmp280;

/** Internal fine temperature value used by compensation formula. */
static int32_t s_t_fine = 0;

/** Reads bytes from a BMP280 register range. */
static int ic_bmp280_read_bytes(ic_bmp280_handle_t *dev,
                                uint8_t reg,
                                uint8_t *buf,
                                uint16_t len)
{
    if ((dev == NULL) || (dev->hi2c == NULL)) {
        return -1;
    }

    HAL_StatusTypeDef st = HAL_I2C_Mem_Read(dev->hi2c,
                                            dev->i2c_addr,
                                            reg,
                                            I2C_MEMADD_SIZE_8BIT,
                                            buf,
                                            len,
                                            100U);
    return (st == HAL_OK) ? 0 : -1;
}

/** Writes bytes into a BMP280 register range. */
static int ic_bmp280_write_bytes(ic_bmp280_handle_t *dev,
                                 uint8_t reg,
                                 const uint8_t *buf,
                                 uint16_t len)
{
    if ((dev == NULL) || (dev->hi2c == NULL)) {
        return -1;
    }

    HAL_StatusTypeDef st = HAL_I2C_Mem_Write(dev->hi2c,
                                             dev->i2c_addr,
                                             reg,
                                             I2C_MEMADD_SIZE_8BIT,
                                             (uint8_t *)buf,
                                             len,
                                             100U);
    return (st == HAL_OK) ? 0 : -1;
}

/** Reads temperature calibration coefficients T1/T2/T3 from sensor NVM. */
static ic_bmp280_status_t ic_bmp280_read_calib(ic_bmp280_handle_t *dev)
{
    uint8_t raw[6];

    if (ic_bmp280_read_bytes(dev, BMP280_REG_CALIB_T1, raw, 6U) != 0) {
        return IC_BMP280_ERROR;
    }

    dev->T1 = (uint16_t)((((uint16_t)raw[1]) << 8) | raw[0]);
    dev->T2 = (int16_t)((((uint16_t)raw[3]) << 8) | raw[2]);
    dev->T3 = (int16_t)((((uint16_t)raw[5]) << 8) | raw[4]);

    ic_log_printf("[BMP280] calib raw: %02X%02X %02X%02X %02X%02X\r\n",
           raw[1], raw[0], raw[3], raw[2], raw[5], raw[4]);
    ic_log_printf("[BMP280] T1=%u T2=%d T3=%d\r\n", dev->T1, dev->T2, dev->T3);

    return IC_BMP280_OK;
}

/**
 * Converts raw ADC temperature to 0.01 C units using datasheet compensation.
 */
static int32_t ic_bmp280_compensate_T_int32(ic_bmp280_handle_t *dev,
                                            int32_t adc_T)
{
    int32_t var1;
    int32_t var2;

    var1 = ((((adc_T >> 3) - ((int32_t)dev->T1 << 1))) * (int32_t)dev->T2) >> 11;
    var2 = (((((adc_T >> 4) - (int32_t)dev->T1) *
              ((adc_T >> 4) - (int32_t)dev->T1)) >> 12) *
            (int32_t)dev->T3) >> 14;

    s_t_fine = var1 + var2;
    return (s_t_fine * 5 + 128) >> 8;
}

ic_bmp280_status_t ic_bmp280_init(ic_bmp280_handle_t *dev,
                                  I2C_HandleTypeDef *hi2c,
                                  uint8_t i2c_addr)
{
    if ((dev == NULL) || (hi2c == NULL)) {
        return IC_BMP280_ERROR;
    }

    dev->hi2c = hi2c;
    dev->i2c_addr = i2c_addr;
    dev->T1 = 0U;
    dev->T2 = 0;
    dev->T3 = 0;
    dev->is_initialized = 0U;

    uint8_t id = 0U;
    if (ic_bmp280_read_bytes(dev, BMP280_REG_ID, &id, 1U) != 0) {
        ic_log_printf("[BMP280] read ID failed\r\n");
        return IC_BMP280_ERROR;
    }

    ic_log_printf("[BMP280] chip_id read id=0x%02X (expect 0x58 or 0x60)\r\n", id);
    if ((id != 0x58U) && (id != 0x60U)) {
        return IC_BMP280_BAD_ID;
    }

    ic_bmp280_status_t st = ic_bmp280_read_calib(dev);
    if (st != IC_BMP280_OK) {
        return st;
    }

    {
        const uint8_t ctrl_meas = (uint8_t)((1U << 5) | (1U << 2) | 0x03U);
        if (ic_bmp280_write_bytes(dev, BMP280_REG_CTRL_MEAS, &ctrl_meas, 1U) != 0) {
            ic_log_printf("[BMP280] write ctrl_meas failed\r\n");
            return IC_BMP280_ERROR;
        }
    }

    dev->is_initialized = 1U;
    ic_log_printf("[BMP280] init OK\r\n");
    return IC_BMP280_OK;
}

ic_bmp280_status_t ic_bmp280_read_temperature(ic_bmp280_handle_t *dev,
                                              float *temp_c)
{
    if ((dev == NULL) || (temp_c == NULL)) {
        return IC_BMP280_ERROR;
    }
    if (dev->is_initialized == 0U) {
        return IC_BMP280_NOT_INITIALIZED;
    }

    {
        uint8_t status = 0U;
        if (ic_bmp280_read_bytes(dev, BMP280_REG_STATUS, &status, 1U) != 0) {
            return IC_BMP280_ERROR;
        }
        (void)status;
    }

    uint8_t t[3] = {0U};
    if (ic_bmp280_read_bytes(dev, BMP280_REG_TEMP_MSB, t, 3U) != 0) {
        ic_log_printf("[BMP280] read temp raw failed\r\n");
        return IC_BMP280_ERROR;
    }

    //ic_log_printf("[BMP280] temp raw: %02X %02X %02X\r\n", t[0], t[1], t[2]);

    const int32_t adc_T = (((int32_t)t[0]) << 12) |
                          (((int32_t)t[1]) << 4) |
                          (((int32_t)t[2]) >> 4);

    const int32_t temp_x100 = ic_bmp280_compensate_T_int32(dev, adc_T);
    *temp_c = temp_x100 / 100.0f;

    return IC_BMP280_OK;
}

ic_bmp280_status_t ic_bmp280_get_temperature(float *temp_c)
{
    return ic_bmp280_read_temperature(&g_ic_bmp280, temp_c);
}
