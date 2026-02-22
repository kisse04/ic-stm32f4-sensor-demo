#include "ic_bmp280.h"
#include <stdint.h>
#include <stdio.h>

// ------------------------------
// BMP280 I2C address / registers
// ------------------------------
#define BMP280_REG_ID         0xD0
#define BMP280_REG_RESET      0xE0
#define BMP280_REG_STATUS     0xF3
#define BMP280_REG_CTRL_MEAS  0xF4
#define BMP280_REG_CONFIG     0xF5
#define BMP280_REG_TEMP_MSB   0xFA  // 0xFA, 0xFB, 0xFC

// calibration registers (T1/T2/T3 位址)
#define BMP280_REG_CALIB_T1   0x88  // T1: 0x88 (LSB), 0x89 (MSB)
#define BMP280_REG_CALIB_T2   0x8A  // T2: 0x8A, 0x8B
#define BMP280_REG_CALIB_T3   0x8C  // T3: 0x8C, 0x8D

// ------------------------------
// Global handle
// ------------------------------
ic_bmp280_handle_t g_ic_bmp280;

// t_fine 只在溫度計算內部用
static int32_t s_t_fine = 0;

// ------------------------------
// Local helpers
// ------------------------------
static int ic_bmp280_read_bytes(ic_bmp280_handle_t *dev,
                                uint8_t reg,
                                uint8_t *buf,
                                uint16_t len)
{
    if (dev == NULL || dev->hi2c == NULL) {
        return -1;
    }

    HAL_StatusTypeDef st = HAL_I2C_Mem_Read(dev->hi2c,
                                            dev->i2c_addr,
                                            reg,
                                            I2C_MEMADD_SIZE_8BIT,
                                            buf,
                                            len,
                                            100);
    return (st == HAL_OK) ? 0 : -1;
}

static int ic_bmp280_write_bytes(ic_bmp280_handle_t *dev,
                                 uint8_t reg,
                                 const uint8_t *buf,
                                 uint16_t len)
{
    if (dev == NULL || dev->hi2c == NULL) {
        return -1;
    }

    HAL_StatusTypeDef st = HAL_I2C_Mem_Write(dev->hi2c,
                                             dev->i2c_addr,
                                             reg,
                                             I2C_MEMADD_SIZE_8BIT,
                                             (uint8_t *)buf,
                                             len,
                                             100);
    return (st == HAL_OK) ? 0 : -1;
}

static ic_bmp280_status_t ic_bmp280_read_calib(ic_bmp280_handle_t *dev)
{
    uint8_t raw[6];

    // T1 (uint16), T2 (int16), T3 (int16)
    if (ic_bmp280_read_bytes(dev, BMP280_REG_CALIB_T1, raw, 6) != 0) {
        return IC_BMP280_ERROR;
    }

    dev->T1 = (uint16_t)(raw[1] << 8 | raw[0]);
    dev->T2 = (int16_t)(raw[3] << 8 | raw[2]);
    dev->T3 = (int16_t)(raw[5] << 8 | raw[4]);

    printf("[BMP280] calib raw: %02X%02X %02X%02X %02X%02X\r\n",
           raw[1], raw[0], raw[3], raw[2], raw[5], raw[4]);
    printf("[BMP280] T1=%u T2=%d T3=%d\r\n",
           dev->T1, dev->T2, dev->T3);

    return IC_BMP280_OK;
}

// datasheet 官方公式 (temperature compensation)
// return: 溫度 * 100, 單位 0.01°C
static int32_t ic_bmp280_compensate_T_int32(ic_bmp280_handle_t *dev,
                                            int32_t adc_T)
{
    int32_t var1, var2, T;

    var1 = ((((adc_T >> 3) - ((int32_t)dev->T1 << 1))) * (int32_t)dev->T2) >> 11;
    var2 = (((((adc_T >> 4) - (int32_t)dev->T1) *
              ((adc_T >> 4) - (int32_t)dev->T1)) >> 12) *
            (int32_t)dev->T3) >> 14;

    s_t_fine = var1 + var2;
    T = (s_t_fine * 5 + 128) >> 8; // T in 0.01 °C

    return T;
}

// ------------------------------
// Public API
// ------------------------------

ic_bmp280_status_t ic_bmp280_init(ic_bmp280_handle_t *dev,
                                  I2C_HandleTypeDef  *hi2c,
                                  uint8_t             i2c_addr)
{
    if (dev == NULL || hi2c == NULL) {
        return IC_BMP280_ERROR;
    }

    dev->hi2c         = hi2c;
    dev->i2c_addr     = i2c_addr;
    dev->T1           = 0;
    dev->T2           = 0;
    dev->T3           = 0;
    dev->is_initialized = 0;

    // 讀 ID
    uint8_t id = 0;
    if (ic_bmp280_read_bytes(dev, BMP280_REG_ID, &id, 1) != 0) {
        printf("[BMP280] read ID failed\r\n");
        return IC_BMP280_ERROR;
    }

    printf("[BMP280] chip_id read id=0x%02X (expect 0x58 or 0x60)\r\n", id);

    if (id != 0x58 && id != 0x60) {
        return IC_BMP280_BAD_ID;
    }

    // 讀 calibration 參數
    ic_bmp280_status_t st = ic_bmp280_read_calib(dev);
    if (st != IC_BMP280_OK) {
        return st;
    }

    // 設定 ctrl_meas：osrs_t = x1, osrs_p = x1, mode = normal
    uint8_t ctrl_meas = (1U << 5) | (1U << 2) | 0x03U;
    if (ic_bmp280_write_bytes(dev, BMP280_REG_CTRL_MEAS, &ctrl_meas, 1) != 0) {
        printf("[BMP280] write ctrl_meas failed\r\n");
        return IC_BMP280_ERROR;
    }

    // config 預設就好（也可以視需要寫）
    // uint8_t config = 0x00;
    // ic_bmp280_write_bytes(dev, BMP280_REG_CONFIG, &config, 1);

    dev->is_initialized = 1;

    printf("[BMP280] init OK\r\n");
    return IC_BMP280_OK;
}

ic_bmp280_status_t ic_bmp280_read_temperature(ic_bmp280_handle_t *dev,
                                              float              *temp_c)
{
    if (dev == NULL || temp_c == NULL) {
        return IC_BMP280_ERROR;
    }
    if (dev->is_initialized == 0) {
        return IC_BMP280_NOT_INITIALIZED;
    }

    // 狀態暫存器可選擇性檢查 (busy bit)
    uint8_t status = 0;
    if (ic_bmp280_read_bytes(dev, BMP280_REG_STATUS, &status, 1) != 0) {
        return IC_BMP280_ERROR;
    }
    // bit3 measuring, bit0 im_update; 這裡簡單略過 polling，直接讀值即可

    // 讀三個溫度暫存器
    uint8_t t[3] = {0};
    if (ic_bmp280_read_bytes(dev, BMP280_REG_TEMP_MSB, t, 3) != 0) {
        printf("[BMP280] read temp raw failed\r\n");
        return IC_BMP280_ERROR;
    }

    printf("[BMP280] temp raw: %02X %02X %02X\r\n", t[0], t[1], t[2]);

    int32_t adc_T = ((int32_t)t[0] << 12) |
                    ((int32_t)t[1] << 4)  |
                    ((int32_t)t[2] >> 4);

    int32_t temp_x100 = ic_bmp280_compensate_T_int32(dev, adc_T);
    *temp_c = temp_x100 / 100.0f;

    return IC_BMP280_OK;
}

ic_bmp280_status_t ic_bmp280_get_temperature(float *temp_c)
{
    return ic_bmp280_read_temperature(&g_ic_bmp280, temp_c);
}