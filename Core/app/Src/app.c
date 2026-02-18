#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "app.h"
#include "led.h"
#include "logger.h"
#include "bmp280.h"

extern I2C_HandleTypeDef hi2c1;

static uint32_t s_next_toggle_ms = 0;
static int      s_count = 0;
static int      s_done = 0;
//static uint8_t blink_count = 0;

void App_Init(void)
{
  Led_Init();
  printf("Boot OK!\r\n");

  s_next_toggle_ms = HAL_GetTick() + 500;
  s_count = 0;
  s_done = 0;
  
  I2C_Scan();                // 先掃描
  BMP280_Init(&hi2c1);       // 初始化 sensor

}

void I2C_Scan(void)
{
    printf("Scanning I2C...\r\n");

    for (uint8_t addr = 1; addr < 128; addr++)
    {
        if (HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 10) == HAL_OK)
        {
            printf("Found device at 0x%02X\r\n", addr);
        }
    }
}

void App_Run(void)
{
  if (s_done) {
    return;
  }

  uint32_t now = HAL_GetTick();
  if ((int32_t)(now - s_next_toggle_ms) >= 0)
  {
    Led_Toggle();
    
    float temp = BMP280_ReadTemperature();
    printf("Blink %d - Temp: %.2f C\r\n", s_count, temp);
    printf("LED toggle %d\r\n", s_count + 1);

    s_count++;

    if (s_count >= 20)
    {
      Led_On();
      printf("Done. LED on.\r\n");
      s_done = 1;
      return;
    }

    s_next_toggle_ms += 500;
  }
}
