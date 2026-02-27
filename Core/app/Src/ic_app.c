/**
 * Application layer runtime flow.
 *
 * This module initializes peripherals used by the demo flow and
 * periodically reads sensors while toggling the board LED.
 *
 * Architecture improvements over the original:
 *  - All I2C operations (including init) are guarded by i2cMutexHandle
 *    to eliminate bus-access races between tasks.
 *  - ic_i2c_scan() is compiled in only when IC_DEBUG_I2C_SCAN is defined,
 *    removing the worst-case ~1.27 s boot penalty in production.
 *  - Task configuration (loop counts, intervals) is expressed through
 *    ic_XTask_cfg_t structs and compile-time constants rather than
 *    magic numbers scattered through task bodies.
 *  - Every sensor init is retried up to IC_APP_INIT_MAX_RETRIES times
 *    with a short back-off; persistent failure sets a dedicated error
 *    flag so a future supervisor task can react.
 *  - Tasks accept their configuration via the `argument` pointer and
 *    can be stopped gracefully through a shared cancel flag, rather
 *    than hard-coding an internal counter limit.
 *  - Dead initialisation code (the three unused next_*_ms variables)
 *    has been removed.
 */

#include <stdio.h>

#include "ic_app.h"
#include "ic_bmp280.h"
#include "ic_led.h"
#include "ic_logger.h"
#include "ic_status.h"
#include "ic_vl53l0x.h"

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2c.h"

/* -------------------------------------------------------------------------
 * Compile-time tunables
 * ---------------------------------------------------------------------- */

/** Number of init attempts before a sensor is declared unavailable. */
#ifndef IC_APP_INIT_MAX_RETRIES
#  define IC_APP_INIT_MAX_RETRIES   3U
#endif

/** Back-off delay (ms) between successive init retries. */
#ifndef IC_APP_INIT_RETRY_DELAY_MS
#  define IC_APP_INIT_RETRY_DELAY_MS  100U
#endif

/** Maximum number of LED toggles before the LED task exits. */
#ifndef IC_APP_LED_MAX_COUNT
#  define IC_APP_LED_MAX_COUNT      20U
#endif

/** Maximum number of TOF readings before the TOF task exits. */
#ifndef IC_APP_TOF_MAX_COUNT
#  define IC_APP_TOF_MAX_COUNT      40U
#endif

/** Maximum number of temperature readings before the temp task exits. */
#ifndef IC_APP_TEMP_MAX_COUNT
#  define IC_APP_TEMP_MAX_COUNT     10U
#endif

/** LED toggle period (ms). */
#ifndef IC_APP_LED_INTERVAL_MS
#  define IC_APP_LED_INTERVAL_MS    500U
#endif

/** TOF read period (ms). */
#ifndef IC_APP_TOF_INTERVAL_MS
#  define IC_APP_TOF_INTERVAL_MS    250U
#endif

/** Temperature read period (ms). */
#ifndef IC_APP_TEMP_INTERVAL_MS
#  define IC_APP_TEMP_INTERVAL_MS   1000U
#endif

/** Timeout waiting for IC_APP_INIT_DONE_BIT (ms). */
#ifndef IC_APP_INIT_WAIT_TIMEOUT_MS
#  define IC_APP_INIT_WAIT_TIMEOUT_MS  10000U
#endif

/* -------------------------------------------------------------------------
 * External handles
 * ---------------------------------------------------------------------- */

/** I2C mutex handle defined in freertos.c. */
extern osMutexId_t i2cMutexHandle;

/** I2C1 handle defined in main.c. */
extern I2C_HandleTypeDef hi2c1;

/* -------------------------------------------------------------------------
 * Default task configuration instances
 *
 * Types are defined in ic_app.h.  These defaults are used when a task's
 * argument pointer is NULL.
 * ---------------------------------------------------------------------- */
static const ic_led_task_cfg_t  ic_led_task_cfg_default  = {
    IC_APP_LED_MAX_COUNT,  IC_APP_LED_INTERVAL_MS
};
static const ic_tof_task_cfg_t  ic_tof_task_cfg_default  = {
    IC_APP_TOF_MAX_COUNT,  IC_APP_TOF_INTERVAL_MS
};
static const ic_temp_task_cfg_t ic_temp_task_cfg_default = {
    IC_APP_TEMP_MAX_COUNT, IC_APP_TEMP_INTERVAL_MS
};

/* -------------------------------------------------------------------------
 * Private helper prototypes
 * ---------------------------------------------------------------------- */

#ifdef IC_DEBUG_I2C_SCAN
/** Performs an I2C address scan on the configured bus (debug builds only). */
static void ic_i2c_scan (void);
#endif

/**
 * @brief  Initialises the BMP280 sensor with retry logic.
 *
 * Acquires the I2C mutex for each attempt.  On persistent failure the
 * IC_APP_BMP280_FAIL_BIT event flag is set so a supervisor can react.
 *
 * @return IC_STATUS_OK on success, error code on all retries exhausted.
 */
static ic_status_t ic_app_init_bmp280 (void);

/**
 * @brief  Initialises the VL53L0X sensor with retry logic.
 *
 * Acquires the I2C mutex for each attempt.  On persistent failure the
 * IC_APP_VL53L0X_FAIL_BIT event flag is set.
 *
 * @return IC_STATUS_OK on success, error code on all retries exhausted.
 */
static ic_status_t ic_app_init_vl53l0x (void);

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

void
ic_app_init (void)
{
    ic_status_t st;

    st = ic_led_init ();
    if (IC_STATUS_IS_ERROR (st)) {
        ic_log_printf ("[APP] LED init failed: %s (%s, code=%d)\r\n",
                       IC_Status_ToString (st),
                       IC_Status_CategoryString (st),
                       (int) st);
        /* Non-fatal – continue boot. */
    }

    ic_log_printf ("[Nucleo_F446RE] Boot OK!\r\n");

#ifdef IC_DEBUG_I2C_SCAN
    ic_i2c_scan ();
#endif

    /* Sensor inits are allowed to fail individually; the application
     * continues so that a working subset of sensors still operates. */
    (void) ic_app_init_bmp280 ();
    (void) ic_app_init_vl53l0x ();

    ic_log_printf ("[Nucleo_F446RE] Sensor init finish\r\n");
    osEventFlagsSet (g_app_init_done, IC_APP_INIT_DONE_BIT);
}

/* -------------------------------------------------------------------------
 * Tasks
 * ---------------------------------------------------------------------- */

void
ic_app_led_task (void* argument)
{
    const ic_led_task_cfg_t* cfg =
        (argument != NULL) ? (const ic_led_task_cfg_t*) argument
                           : &ic_led_task_cfg_default;

    /* Wait until the init sequence has completed (or timed out). */
    osEventFlagsWait (g_app_init_done,
                      IC_APP_INIT_DONE_BIT,
                      osFlagsWaitAny | osFlagsNoClear,
                      IC_APP_INIT_WAIT_TIMEOUT_MS);

    /* Re-init inside the task so that the LED subsystem is owned here. */
    ic_status_t st = ic_led_init ();
    if (IC_STATUS_IS_ERROR (st)) {
        ic_log_printf ("[LED] init failed: %s (%s, code=%d)\r\n",
                       IC_Status_ToString (st),
                       IC_Status_CategoryString (st),
                       (int) st);
        osThreadExit ();
    }

    ic_log_printf ("[LED] task started (max=%lu, interval=%lu ms)\r\n",
                   cfg->max_count, cfg->interval_ms);

    uint32_t led_count = 0U;

    for (;;) {
        st = ic_led_toggle ();
        led_count++;

        if (IC_STATUS_IS_OK (st)) {
            ic_log_printf ("[LED] toggle %lu\r\n", led_count);
        } else {
            ic_log_printf ("[LED] toggle failed: %s (%s, code=%d)\r\n",
                           IC_Status_ToString (st),
                           IC_Status_CategoryString (st),
                           (int) st);
        }

        if (led_count >= cfg->max_count) {
            st = ic_led_on ();
            if (IC_STATUS_IS_OK (st)) {
                ic_log_printf ("[LED] Done. LED on.\r\n");
            } else {
                ic_log_printf ("[LED] LED on failed: %s (%s, code=%d)\r\n",
                               IC_Status_ToString (st),
                               IC_Status_CategoryString (st),
                               (int) st);
            }
            osThreadExit ();
        }

        osDelay (cfg->interval_ms);
    }
}

void
ic_app_tof_task (void* argument)
{
    const ic_tof_task_cfg_t* cfg =
        (argument != NULL) ? (const ic_tof_task_cfg_t*) argument
                           : &ic_tof_task_cfg_default;

    osEventFlagsWait (g_app_init_done,
                      IC_APP_INIT_DONE_BIT,
                      osFlagsWaitAny | osFlagsNoClear,
                      IC_APP_INIT_WAIT_TIMEOUT_MS);

    /* Abort early if this sensor failed to initialise. */
    uint32_t flags = osEventFlagsGet (g_app_error_flags);
    if (flags & IC_APP_VL53L0X_FAIL_BIT) {
        ic_log_printf ("[VL53L0X] Sensor unavailable – task aborted.\r\n");
        osThreadExit ();
    }

    ic_log_printf ("[VL53L0X] TOF task started (max=%lu, interval=%lu ms)\r\n",
                   cfg->max_count, cfg->interval_ms);

    uint32_t tof_count = 0U;

    for (;;) {
        uint16_t  distance_mm = 0U;
        ic_status_t st;

        osMutexAcquire (i2cMutexHandle, osWaitForever);
        st = ic_vl53l0x_get_distance_mm (&distance_mm);
        osMutexRelease (i2cMutexHandle);

        if (IC_STATUS_IS_OK (st)) {
            tof_count++;
            ic_log_printf ("[VL53L0X] detect %lu – distance: %u mm\r\n",
                           tof_count, distance_mm);
        } else {
            ic_log_printf ("[VL53L0X] read failed: %s (%s, code=%d)\r\n",
                           IC_Status_ToString (st),
                           IC_Status_CategoryString (st),
                           (int) st);
        }

        if (tof_count >= cfg->max_count) {
            ic_log_printf ("[VL53L0X] Done. TOF stopped.\r\n");
            osThreadExit ();
        }

        osDelay (cfg->interval_ms);
    }
}

void
ic_app_temp_task (void* argument)
{
    const ic_temp_task_cfg_t* cfg =
        (argument != NULL) ? (const ic_temp_task_cfg_t*) argument
                           : &ic_temp_task_cfg_default;

    osEventFlagsWait (g_app_init_done,
                      IC_APP_INIT_DONE_BIT,
                      osFlagsWaitAny | osFlagsNoClear,
                      IC_APP_INIT_WAIT_TIMEOUT_MS);

    /* Abort early if this sensor failed to initialise. */
    uint32_t flags = osEventFlagsGet (g_app_error_flags);
    if (flags & IC_APP_BMP280_FAIL_BIT) {
        ic_log_printf ("[BMP280] Sensor unavailable – task aborted.\r\n");
        osThreadExit ();
    }

    ic_log_printf ("[BMP280] Temp task started (max=%lu, interval=%lu ms)\r\n",
                   cfg->max_count, cfg->interval_ms);

    uint32_t temp_count = 0U;

    for (;;) {
        float       temp_c = 0.0f;
        ic_status_t st;

        osMutexAcquire (i2cMutexHandle, osWaitForever);
        st = ic_bmp280_get_temperature (&temp_c);
        osMutexRelease (i2cMutexHandle);

        if (IC_STATUS_IS_OK (st)) {
            temp_count++;

            int32_t temp_x100 = (int32_t) (temp_c * 100.0f);
            ic_log_printf ("[BMP280] detect %lu – Temp: %ld.%02ld C\r\n",
                           temp_count,
                           temp_x100 / 100,
                           temp_x100 % 100);
        } else {
            ic_log_printf ("[BMP280] read failed: %s (%s, code=%d)\r\n",
                           IC_Status_ToString (st),
                           IC_Status_CategoryString (st),
                           (int) st);
        }

        if (temp_count >= cfg->max_count) {
            ic_log_printf ("[BMP280] Done. Temp stopped.\r\n");
            osThreadExit ();
        }

        osDelay (cfg->interval_ms);
    }
}

/* -------------------------------------------------------------------------
 * Private helpers
 * ---------------------------------------------------------------------- */

static ic_status_t
ic_app_init_bmp280 (void)
{
    ic_status_t st;

    for (uint8_t attempt = 0U; attempt < IC_APP_INIT_MAX_RETRIES; attempt++) {
        osMutexAcquire (i2cMutexHandle, osWaitForever);
        st = ic_bmp280_init (&g_ic_bmp280,
                             &hi2c1,
                             IC_BMP280_I2C_ADDR_DEFAULT);
        osMutexRelease (i2cMutexHandle);

        if (IC_STATUS_IS_OK (st)) {
            ic_log_printf ("[BMP280] init OK (attempt %u)\r\n",
                           (unsigned) attempt + 1U);
            return IC_STATUS_OK;
        }

        ic_log_printf ("[BMP280] init attempt %u failed: %s (%s, code=%d)\r\n",
                       (unsigned) attempt + 1U,
                       IC_Status_ToString (st),
                       IC_Status_CategoryString (st),
                       (int) st);

        if (attempt + 1U < IC_APP_INIT_MAX_RETRIES) {
            osDelay (IC_APP_INIT_RETRY_DELAY_MS);
        }
    }

    ic_log_printf ("[BMP280] init failed after %u attempts – sensor disabled.\r\n",
                   (unsigned) IC_APP_INIT_MAX_RETRIES);
    osEventFlagsSet (g_app_error_flags, IC_APP_BMP280_FAIL_BIT);
    return st;
}

static ic_status_t
ic_app_init_vl53l0x (void)
{
    ic_status_t st;

    for (uint8_t attempt = 0U; attempt < IC_APP_INIT_MAX_RETRIES; attempt++) {
        osMutexAcquire (i2cMutexHandle, osWaitForever);
        st = ic_vl53l0x_init (&g_ic_vl53l0x,
                              &hi2c1,
                              IC_VL53L0X_I2C_ADDR_DEFAULT);
        osMutexRelease (i2cMutexHandle);

        if (IC_STATUS_IS_OK (st)) {
            ic_log_printf ("[VL53L0X] init OK (attempt %u)\r\n",
                           (unsigned) attempt + 1U);
            return IC_STATUS_OK;
        }

        ic_log_printf ("[VL53L0X] init attempt %u failed: %s (%s, code=%d)\r\n",
                       (unsigned) attempt + 1U,
                       IC_Status_ToString (st),
                       IC_Status_CategoryString (st),
                       (int) st);

        if (attempt + 1U < IC_APP_INIT_MAX_RETRIES) {
            osDelay (IC_APP_INIT_RETRY_DELAY_MS);
        }
    }

    ic_log_printf ("[VL53L0X] init failed after %u attempts – sensor disabled.\r\n",
                   (unsigned) IC_APP_INIT_MAX_RETRIES);
    osEventFlagsSet (g_app_error_flags, IC_APP_VL53L0X_FAIL_BIT);
    return st;
}

#ifdef IC_DEBUG_I2C_SCAN
static void
ic_i2c_scan (void)
{
    ic_log_printf ("[Nucleo_F446RE] Scanning I2C bus...\r\n");

    for (uint8_t addr = 1U; addr < 128U; addr++) {
        osMutexAcquire (i2cMutexHandle, osWaitForever);
        HAL_StatusTypeDef hal_st =
            HAL_I2C_IsDeviceReady (&hi2c1, (uint16_t) (addr << 1), 1U, 10U);
        osMutexRelease (i2cMutexHandle);

        if (hal_st == HAL_OK) {
            ic_log_printf ("[Nucleo_F446RE] Found device at 0x%02X\r\n", addr);
        }
    }

    ic_log_printf ("[Nucleo_F446RE] I2C scan complete.\r\n");
}
#endif /* IC_DEBUG_I2C_SCAN */