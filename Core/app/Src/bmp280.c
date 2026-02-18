#include "bmp280.h"
#include <stdio.h>

#define BMP280_ADDR      (0x76 << 1)   // 如果讀不到改 0x77
#define BMP280_REG_ID    0xD0
#define BMP280_REG_CTRL  0xF4
#define BMP280_REG_TEMP  0xFA

static I2C_HandleTypeDef *s_hi2c;

void BMP280_Init(I2C_HandleTypeDef *hi2c)
{
    s_hi2c = hi2c;

    uint8_t id;
    HAL_I2C_Mem_Read(s_hi2c, BMP280_ADDR,
                     BMP280_REG_ID,
                     I2C_MEMADD_SIZE_8BIT,
                     &id, 1, HAL_MAX_DELAY);

    printf("BMP280 ID: 0x%02X\r\n", id);

    uint8_t ctrl = 0x27;   // normal mode
    HAL_I2C_Mem_Write(s_hi2c, BMP280_ADDR,
                      BMP280_REG_CTRL,
                      I2C_MEMADD_SIZE_8BIT,
                      &ctrl, 1, HAL_MAX_DELAY);
}

float BMP280_ReadTemperature(void)
{
    uint8_t data[3];
    HAL_I2C_Mem_Read(s_hi2c, BMP280_ADDR,
                     BMP280_REG_TEMP,
                     I2C_MEMADD_SIZE_8BIT,
                     data, 3, HAL_MAX_DELAY);

    int32_t raw = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);

    return raw / 100000.0f;  // 簡化版
}
