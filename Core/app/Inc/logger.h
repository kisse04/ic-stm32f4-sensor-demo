#pragma once
#include "stm32f4xx_hal.h"

void Log_Init(UART_HandleTypeDef* huart);
int  Log_Write(const uint8_t* data, uint16_t len);
