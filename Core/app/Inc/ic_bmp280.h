#ifndef IC_BMP280_H
#define IC_BMP280_H

#include "stm32f4xx_hal.h"

void ic_BMP280_Init(I2C_HandleTypeDef *hi2c);
int32_t ic_BMP280_ReadTemperature(void);



#endif
