#include <stdio.h>
#include "ic_app.h"
#include "ic_led.h"
#include "ic_logger.h"
#include "ic_bmp280.h"
#include "ic_vl53l0x.h"

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2c.h"

extern I2C_HandleTypeDef hi2c1;

static uint32_t s_next_toggle_ms = 0;
static int      s_count = 0;
static int      s_done = 0;
//static uint8_t blink_count = 0;

static uint32_t s_tof_count = 0U;

void ic_App_Init(void)
{
  ic_Led_Init();
  printf("Boot OK!\r\n");

  s_next_toggle_ms = HAL_GetTick() + 500;
  s_count = 0;
  s_done = 0;
  
  ic_I2C_Scan();                // 先掃描
  ic_BMP280_Init(&hi2c1);       // 初始化 sensor

  /* 例如 TOF 掛在 I2C1，就用 &hi2c1 */
  if (ic_vl53l0x_init(&g_vl53l0x, &hi2c1, ic_VL53L0X_I2C_ADDR_DEFAULT) != VL53L0X_OK)
  {
    printf("VL53L0X init failed\r\n");
  }
  else
  {
    printf("VL53L0X init OK\r\n");
  }


}

void ic_I2C_Scan(void)
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

void ic_App_Run(void)
{
  if (s_done) {
    return;
  }

  uint32_t now = HAL_GetTick();
  if ((int32_t)(now - s_next_toggle_ms) >= 0)
  {
  
    //print temperature from BMP280
    int32_t temp = ic_BMP280_ReadTemperature();
    printf("Blink %d - Temp: %ld.%02ld C\r\n", s_count, temp/100, temp%100);

    //print distence from VL53L0X
    uint16_t tof_distance_mm = 0U;
    ic_vl53l0x_status_t st;

    /* 取得距離 (mm) */
    st = ic_vl53l0x_get_distance_mm(&tof_distance_mm);

    if (st == VL53L0X_OK)
    {
        s_tof_count++;
        printf("Tof detect %lu - distance: %u mm\r\n",
               (unsigned long)s_tof_count,
               (unsigned int)tof_distance_mm);
    }
    else
    {
        printf("Tof read failed, status=%d\r\n", (int)st);
    }

    //print LED toggle with STMboard NUCLEO F446RE original function
    ic_Led_Toggle();
    printf("LED toggle %d\r\n", s_count + 1);

    s_count++;

    if (s_count >= 20)
    {
      ic_Led_On();
      printf("Done. LED on.\r\n");
      s_done = 1;
      return;
    }

    s_next_toggle_ms += 500;
  }
}
