#include "led.h"
#include "main.h"   // 這裡用到 LD2_GPIO_Port / LD2_Pin

void Led_Init(void)
{
  // GPIO 已由 MX_GPIO_Init() 初始化；這裡先不做事也可以
}

void Led_Toggle(void)
{
  HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
}

void Led_On(void)
{
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
}
