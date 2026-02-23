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

extern I2C_HandleTypeDef hi2c1;

/** Next LED/sensor task trigger timestamp in milliseconds. */
static uint32_t s_next_toggle_ms = 0U;
/** Number of executed loop iterations. */
static int s_count = 0;
/** Completion flag for one-shot demo loop. */
static int s_done = 0;
/** Number of successful ToF reads. */
static uint32_t s_tof_count = 0U;

/** Performs an I2C address scan on the configured bus. */
void ic_I2C_Scan(void);

void ic_app_init(void)
{
    ic_led_init();
    printf("Boot OK!\r\n");

    s_next_toggle_ms = HAL_GetTick() + 500U;
    s_count = 0;
    s_done = 0;
    s_tof_count = 0U;

    ic_I2C_Scan();

    ic_bmp280_status_t bmp_st = ic_bmp280_init(&g_ic_bmp280,
                                                &hi2c1,
                                                IC_BMP280_I2C_ADDR_DEFAULT);
    if (bmp_st != IC_BMP280_OK) {
        printf("[BMP280] init failed, status=%d\r\n", (int)bmp_st);
    }

    if (ic_vl53l0x_init(&g_ic_vl53l0x, &hi2c1, IC_VL53L0X_I2C_ADDR_DEFAULT) != IC_VL53L0X_OK) {
        printf("VL53L0X init failed\r\n");
    } else {
        printf("VL53L0X init OK\r\n");
    }
}

void ic_I2C_Scan(void)
{
    printf("Scanning I2C...\r\n");

    for (uint8_t addr = 1U; addr < 128U; addr++) {
        if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(addr << 1), 1U, 10U) == HAL_OK) {
            printf("Found device at 0x%02X\r\n", addr);
        }
    }
}

void ic_app_run(void)
{
    if (s_done) {
        return;
    }

    const uint32_t now = HAL_GetTick();
    if ((int32_t)(now - s_next_toggle_ms) >= 0) {
        float temp_c = 0.0f;
        ic_bmp280_status_t bmp_st = ic_bmp280_get_temperature(&temp_c);

        if (bmp_st == IC_BMP280_OK) {
            const int32_t temp_x100 = (int32_t)(temp_c * 100.0f);
            printf("Blink %lu - Temp: %ld.%02ld C\r\n",
                   (unsigned long)s_count,
                   temp_x100 / 100,
                   temp_x100 % 100);
        } else {
            printf("BMP280 read failed, status=%d\r\n", (int)bmp_st);
        }

        uint16_t tof_distance_mm = 0U;
        ic_vl53l0x_status_t tof_st = ic_vl53l0x_get_distance_mm(&tof_distance_mm);

        if (tof_st == IC_VL53L0X_OK) {
            s_tof_count++;
            printf("Tof detect %lu - distance: %u mm\r\n",
                   (unsigned long)s_tof_count,
                   (unsigned int)tof_distance_mm);
        } else {
            printf("Tof read failed, status=%d\r\n", (int)tof_st);
        }

        ic_led_toggle();
        printf("LED toggle %d\r\n", s_count + 1);

        s_count++;
        if (s_count >= 20) {
            ic_led_on();
            printf("Done. LED on.\r\n");
            s_done = 1;
            return;
        }

        s_next_toggle_ms += 500U;
    }
}
