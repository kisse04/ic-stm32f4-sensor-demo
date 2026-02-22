#ifndef IC_BMP280_H
#define IC_BMP280_H

#include "stm32f4xx_hal.h"

/*
void ic_bmp280_init(I2C_HandleTypeDef *hi2c);
int32_t ic_bmp280_read_temperature(void);
*/

typedef enum {
    IC_BMP280_OK = 0,
    IC_BMP280_ERROR = -1,
    IC_BMP280_BAD_ID = -2,
    IC_BMP280_NOT_INITIALIZED = -3
} ic_bmp280_status_t;

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t            i2c_addr;
    /* calib 參數... */
    uint16_t           T1;
    int16_t            T2;
    int16_t            T3;
    uint8_t            is_initialized;
} ic_bmp280_handle_t;

/* 預設 I2C addr (7-bit 左移一位給 HAL 用) */
#define IC_BMP280_I2C_ADDR_DEFAULT   (0x76U << 1)

ic_bmp280_status_t ic_bmp280_init(ic_bmp280_handle_t *dev,
                                  I2C_HandleTypeDef  *hi2c,
                                  uint8_t             i2c_addr);

/* 回傳溫度 (0.01 °C 或 float 自選) */
ic_bmp280_status_t ic_bmp280_read_temperature(ic_bmp280_handle_t *dev,
                                              float              *temp_c);

/* 如果你想方便模式，也可以加： */
extern ic_bmp280_handle_t g_ic_bmp280;
ic_bmp280_status_t ic_bmp280_get_temperature(float *temp_c);

#endif /* IC_BMP280_H */
