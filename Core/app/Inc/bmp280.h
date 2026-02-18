#ifndef BMP280_H
#define BMP280_H

#include "stm32f4xx_hal.h"

void BMP280_Init(I2C_HandleTypeDef *hi2c);
int32_t BMP280_ReadTemperature(void);



#endif
