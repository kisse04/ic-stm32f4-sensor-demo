#pragma once
#include "stm32f4xx_hal.h"

void ic_Log_Init(UART_HandleTypeDef* huart);
int  ic_Log_Write(const uint8_t* data, uint16_t len);
