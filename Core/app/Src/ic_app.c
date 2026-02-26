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
#include "ic_status.h"

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2c.h"

/** I2C mutex handle defined in freertos.c. */
extern osMutexId_t i2cMutexHandle;

/** I2C1 handle defined in main.c. */
extern I2C_HandleTypeDef hi2c1;

/** Performs an I2C address scan on the configured bus. */
static void
ic_i2c_scan (void);

void
ic_app_init (void)
{
    ic_led_init ();
    ic_log_printf ("[Nucleo_F446RE] Boot OK!\r\n");

    /* Initialize schedule relative to the current tick. */
    uint32_t now = HAL_GetTick ();
    uint32_t next_led_ms = now + 500U;    /* 0.5 s LED toggle. */
    uint32_t next_tof_ms = now + 250U;    /* 0.25 s TOF read. */
    uint32_t next_temp_ms = now + 1000U;  /* 1 s temperature read. */
    (void) next_led_ms;
    (void) next_tof_ms;
    (void) next_temp_ms;

    ic_i2c_scan ();

    /*ic_bmp280_status_t bmp_st = ic_bmp280_init (&g_ic_bmp280,
                                                &hi2c1,
                                                IC_BMP280_I2C_ADDR_DEFAULT);
    if (bmp_st != IC_BMP280_OK) {
        ic_log_printf ("[BMP280] init failed, status=%d\r\n", (int) bmp_st);
    }*/
    ic_app_init_bmp280 ();
    ic_app_init_vl53l0x ();
    /*
    if (ic_vl53l0x_init (&g_ic_vl53l0x,
                         &hi2c1,
                         IC_VL53L0X_I2C_ADDR_DEFAULT) != IC_VL53L0X_OK) {
        ic_log_printf ("[VL53L0X] init failed\r\n");
    }
    */
    ic_log_printf ("[Nucleo_F446RE] Sensor init finish\r\n");
    osEventFlagsSet (g_app_init_done, IC_APP_INIT_DONE_BIT);
}

void ic_app_init_bmp280(void)
{
    ic_status_t bmp_st = ic_bmp280_init (&g_ic_bmp280,
                                         &hi2c1,
                                         IC_BMP280_I2C_ADDR_DEFAULT);
    if (IC_STATUS_IS_ERROR (bmp_st)) {
         ic_log_printf ("[BMP280] init failed: %s (%s, code=%d)\r\n",
                        IC_Status_ToString (bmp_st),
                        IC_Status_CategoryString (bmp_st),
                        (int) bmp_st);
    } else {
     ic_log_printf ("[BMP280] init OK\r\n");
    }
}

void ic_app_init_vl53l0x(void)
{
    ic_status_t tof_st = ic_vl53l0x_init (&g_ic_vl53l0x,
                                          &hi2c1,
                                          IC_VL53L0X_I2C_ADDR_DEFAULT);

    if (IC_STATUS_IS_ERROR (tof_st)) {
      ic_log_printf ("[VL53L0X] init failed: %s (%s, code=%d)\r\n",
                      IC_Status_ToString (tof_st),
                      IC_Status_CategoryString (tof_st),
                       (int) tof_st);
    } else {
     ic_log_printf ("[VL53L0X] init OK\r\n");
    }
}


static void
ic_i2c_scan (void)
{
    ic_log_printf ("[Nucleo_F446RE] Scanning I2C...\r\n");

    for (uint8_t addr = 1U; addr < 128U; addr++) {
        if (HAL_I2C_IsDeviceReady (&hi2c1, (uint16_t) (addr << 1), 1U, 10U) == HAL_OK) {
            ic_log_printf ("[Nucleo_F446RE] Found device at 0x%02X\r\n", addr);
        }
    }
}

void
ic_app_led_task (void* argument)
{
    (void) argument;

    osEventFlagsWait (g_app_init_done,
                      IC_APP_INIT_DONE_BIT,
                      osFlagsWaitAny | osFlagsNoClear,
                      10000U);

    ic_log_printf ("[LED] task started\r\n");
    uint32_t   led_count = 0;
    ic_status_t st;

    /* 如果你有在別的地方呼叫 ic_led_init()，這段可以省略；
       如果沒有，建議在這裡補一次，確保 flag 被設起來。 */
    st = ic_led_init();
    if (IC_STATUS_IS_ERROR(st)) {
        ic_log_printf("[LED] init failed: %s (%s, code=%d)\r\n",
                      IC_Status_ToString(st),
                      IC_Status_CategoryString(st),
                      (int)st);
        osThreadExit();   /* 或 while(1) 卡住，看你要怎麼設計 */
    }

    for (;;) {
        st = ic_led_toggle();
        led_count++;

        if (IC_STATUS_IS_OK(st)) {
            ic_log_printf ("[LED] toggle %lu\r\n", led_count);
        } else {
            ic_log_printf ("[LED] toggle failed: %s (%s, code=%d)\r\n",
                           IC_Status_ToString(st),
                           IC_Status_CategoryString(st),
                           (int)st);
        }

        if (led_count >= 20U) {
            st = ic_led_on();
            if (IC_STATUS_IS_OK(st)) {
                ic_log_printf ("[LED] Done. LED on.\r\n");
            } else {
                ic_log_printf ("[LED] LED on failed: %s (%s, code=%d)\r\n",
                               IC_Status_ToString(st),
                               IC_Status_CategoryString(st),
                               (int)st);
            }
            osThreadExit ();
        }

        osDelay (500U);
    }
}

void
ic_app_tof_task (void* argument)
{
    (void) argument;

    osEventFlagsWait (g_app_init_done,
                      IC_APP_INIT_DONE_BIT,
                      osFlagsWaitAny | osFlagsNoClear,
                      10000U);

    ic_log_printf ("[VL53L0X] Tof task started\r\n");
    uint32_t tof_count = 0;

    for (;;) {
        uint16_t   distance_mm = 0;
        ic_status_t st;

        osMutexAcquire (i2cMutexHandle, osWaitForever);
        st = ic_vl53l0x_get_distance_mm (&distance_mm);
        osMutexRelease (i2cMutexHandle);

        if (IC_STATUS_IS_OK (st)) {
            tof_count++;
            ic_log_printf ("[VL53L0X] Tof detect %lu - distance: %u mm\r\n",
                           tof_count,
                           distance_mm);
        } else {
            ic_log_printf ("[VL53L0X] Tof read failed: %s (%s, code=%d)\r\n",
                           IC_Status_ToString (st),
                           IC_Status_CategoryString (st),
                           (int) st);
        }

        if (tof_count >= 40U) {
            ic_log_printf ("[VL53L0X] Done. Tof stopped.\r\n");
            osThreadExit ();
        }

        osDelay (250U);
    }
}

void
ic_app_temp_task (void* argument)
{
    (void) argument;

    osEventFlagsWait (g_app_init_done,
                      IC_APP_INIT_DONE_BIT,
                      osFlagsWaitAny | osFlagsNoClear,
                      10000U);

    ic_log_printf ("[BMP280] Temp task started\r\n");
    uint32_t temp_count = 0;

    for (;;) {
        float temp_c = 0.0f;
        //ic_bmp280_status_t st;
        ic_status_t st;

        osMutexAcquire (i2cMutexHandle, osWaitForever);
        st = ic_bmp280_get_temperature (&temp_c);
        osMutexRelease (i2cMutexHandle);

        if (IC_STATUS_IS_OK (st)) {
            temp_count++;

            int32_t temp_x100 = (int32_t) (temp_c * 100.0f);
            ic_log_printf ("[BMP280] Temp detect %lu - Temp: %ld.%02ld C\r\n",
                           temp_count,
                           temp_x100 / 100,
                           temp_x100 % 100);
        } else {
            ic_log_printf ("[BMP280] Temp read failed: %s (%s, code=%d)\r\n",
                           IC_Status_ToString (st),
                           IC_Status_CategoryString (st),
                           (int) st);
        }

        if (temp_count >= 10) {
            ic_log_printf ("[BMP280] Done. Temp stopped.\r\n");
            osThreadExit ();
        }

        osDelay (1000);
    }
}
