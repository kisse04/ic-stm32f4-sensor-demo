/**
 * Application layer runtime flow.
 *
 * This module initializes peripherals used by the demo flow and
 * periodically reads sensors while toggling the board LED.
 */

#include <stdio.h>

#include "ic_app.h"
#include "ic_bmp280.h"
#include "ic_led.h"
#include "ic_logger.h"
#include "ic_vl53l0x.h"

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2c.h"

extern osMutexId_t i2cMutexHandle;  // 在 freertos.c 建立

extern I2C_HandleTypeDef hi2c1;

static uint32_t s_led_count = 0;
static uint32_t s_tof_count = 0;
static uint8_t  s_done      = 0;

/* Next scheduled time for each periodic task (ms since boot). */
static uint32_t s_next_led_ms   = 0;
static uint32_t s_next_tof_ms   = 0;
static uint32_t s_next_temp_ms  = 0;

/**
 * Performs an I2C address scan on the configured bus.
 */
void ic_I2C_Scan (void);

void
ic_app_init (void)
{
    ic_led_init ();
    printf ("Boot OK!\r\n");

    s_led_count = 0;
    s_tof_count = 0;
    s_done      = 0;

    /* Initialize schedule relative to the current tick. */
    uint32_t now = HAL_GetTick ();
    s_next_led_ms  = now + 500U;   /* 0.5 s LED toggle. */
    s_next_tof_ms  = now + 250U;   /* 0.25 s TOF read. */
    s_next_temp_ms = now + 1000U;  /* 1 s temperature read. */

    ic_I2C_Scan ();

    ic_bmp280_status_t bmp_st = ic_bmp280_init (&g_ic_bmp280,
                                                &hi2c1,
                                                IC_BMP280_I2C_ADDR_DEFAULT);
    if (bmp_st != IC_BMP280_OK) {
        printf ("[BMP280] init failed, status=%d\r\n", (int) bmp_st);
    }

    if (ic_vl53l0x_init (&g_ic_vl53l0x, &hi2c1, IC_VL53L0X_I2C_ADDR_DEFAULT) != IC_VL53L0X_OK) {
        printf ("VL53L0X init failed\r\n");
    }
    else {
        printf ("VL53L0X init OK\r\n");
    }
}

void
ic_I2C_Scan (void)
{
    printf ("Scanning I2C...\r\n");

    for (uint8_t addr = 1U; addr < 128U; addr++) {
        if (HAL_I2C_IsDeviceReady (&hi2c1, (uint16_t) (addr << 1), 1U, 10U) == HAL_OK) {
            printf ("Found device at 0x%02X\r\n", addr);
        }
    }
}

void ic_app_led_task(void *argument)
{
    uint32_t led_count = 0;

    for (;;)
    {
        ic_led_toggle();
        led_count++;

        printf("LED toggle %lu\r\n", (unsigned long)led_count);

        if (led_count >= 20) {
            ic_led_on();
            printf("Done. LED on.\r\n");
            // 之後如果不想再動，可以暫停自己
            osThreadExit();
        }

        osDelay(500);    // 500 ms 週期
    }
}

void ic_app_tof_task(void *argument)
{
    uint32_t tof_count = 0;

    for (;;)
    {
        uint16_t distance_mm = 0;
        ic_vl53l0x_status_t st;

        // 共享 I2C，進出都要拿 mutex
        osMutexAcquire(i2cMutexHandle, osWaitForever);
        st = ic_vl53l0x_get_distance_mm(&distance_mm);
        osMutexRelease(i2cMutexHandle);

        if (st == IC_VL53L0X_OK) {
            tof_count++;
            printf("Tof detect %lu - distance: %u mm\r\n",
                   (unsigned long)tof_count,
                   (unsigned int)distance_mm);
        } else {
            printf("Tof read failed, status=%d\r\n", (int)st);
        }
        
        if (tof_count >= 40) {
            printf("Done. tof stopped.\r\n");
            // 之後如果不想再動，可以暫停自己
            osThreadExit();
        }

        osDelay(250);  // 250 ms 週期
    }
}

void ic_app_temp_task(void *argument)
{
    uint32_t temp_count = 0;

    for (;;)
    {
        float temp_c = 0.0f;
        ic_bmp280_status_t st;

        osMutexAcquire(i2cMutexHandle, osWaitForever);
        st = ic_bmp280_get_temperature(&temp_c);
        osMutexRelease(i2cMutexHandle);

        if (st == IC_BMP280_OK) {
            int32_t temp_x100 = (int32_t)(temp_c * 100.0f);
            printf("Temp: %ld.%02ld C\r\n",
                   temp_x100 / 100,
                   temp_x100 % 100);
            temp_count++;
        } else {
            printf("BMP280 read failed, status=%d\r\n", (int)st);
        }

        if (temp_count >= 10) {
            printf("Done. temp stopped.\r\n");
            // 之後如果不想再動，可以暫停自己
            osThreadExit();
        }


        osDelay(1000);   // 1 s 週期
    }
}
