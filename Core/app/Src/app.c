#include "app.h"
#include "led.h"
#include <stdio.h>
#include "stm32f4xx_hal.h"

static uint32_t s_next_toggle_ms = 0;
static int      s_count = 0;
static int      s_done = 0;

void App_Init(void)
{
  Led_Init();
  printf("Boot OK!\r\n");

  s_next_toggle_ms = HAL_GetTick() + 500;
  s_count = 0;
  s_done = 0;
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
