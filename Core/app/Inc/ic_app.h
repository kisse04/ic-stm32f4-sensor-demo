#ifndef IC_APP_H
#define IC_APP_H

#include "cmsis_os2.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Compile-time tunables
 *
 * All values may be overridden at the compiler command line, e.g.:
 *   -DIC_APP_TOF_MAX_COUNT=20
 * ======================================================================= */

/** Number of sensor init attempts before a device is declared unavailable. */
#ifndef IC_APP_INIT_MAX_RETRIES
#  define IC_APP_INIT_MAX_RETRIES        3U
#endif

/** Back-off delay (ms) between successive sensor init retries. */
#ifndef IC_APP_INIT_RETRY_DELAY_MS
#  define IC_APP_INIT_RETRY_DELAY_MS     100U
#endif

/** Maximum number of LED toggles before the LED task exits. */
#ifndef IC_APP_LED_MAX_COUNT
#  define IC_APP_LED_MAX_COUNT           20U
#endif

/** Maximum number of TOF readings before the TOF task exits. */
#ifndef IC_APP_TOF_MAX_COUNT
#  define IC_APP_TOF_MAX_COUNT           40U
#endif

/** Maximum number of temperature readings before the temp task exits. */
#ifndef IC_APP_TEMP_MAX_COUNT
#  define IC_APP_TEMP_MAX_COUNT          10U
#endif

/** LED toggle period (ms). */
#ifndef IC_APP_LED_INTERVAL_MS
#  define IC_APP_LED_INTERVAL_MS         500U
#endif

/** TOF read period (ms). */
#ifndef IC_APP_TOF_INTERVAL_MS
#  define IC_APP_TOF_INTERVAL_MS         250U
#endif

/** Temperature read period (ms). */
#ifndef IC_APP_TEMP_INTERVAL_MS
#  define IC_APP_TEMP_INTERVAL_MS        1000U
#endif

/** Timeout (ms) each task waits for IC_APP_INIT_DONE_BIT before proceeding. */
#ifndef IC_APP_INIT_WAIT_TIMEOUT_MS
#  define IC_APP_INIT_WAIT_TIMEOUT_MS    10000U
#endif

/* =========================================================================
 * Event-flag bits
 * ======================================================================= */

/** Set by ic_app_init() when all sensors have been attempted. */
#define IC_APP_INIT_DONE_BIT         (1U << 0)

/** Set when the BMP280 sensor fails all init retries. */
#define IC_APP_BMP280_FAIL_BIT       (1U << 1)

/** Set when the VL53L0X sensor fails all init retries. */
#define IC_APP_VL53L0X_FAIL_BIT      (1U << 2)

/* =========================================================================
 * Shared RTOS handles
 *
 * These objects must be created (e.g. in freertos.c or main.c) before
 * ic_app_init() is called.
 * ======================================================================= */

/** Mutex protecting shared I2C bus access. Defined in freertos.c. */
extern osMutexId_t i2cMutexHandle;

/**
 * Event flags signalling application init completion.
 * Bits: IC_APP_INIT_DONE_BIT.
 * Must be created before ic_app_init() is called.
 */
extern osEventFlagsId_t g_app_init_done;

/**
 * Event flags carrying per-sensor error state.
 * Bits: IC_APP_BMP280_FAIL_BIT, IC_APP_VL53L0X_FAIL_BIT.
 * Must be created before ic_app_init() is called.
 */
extern osEventFlagsId_t g_app_error_flags;

/* =========================================================================
 * Task configuration structures
 *
 * Allocate one of these and pass its address as the `argument` parameter
 * when creating the corresponding RTOS task.  Passing NULL causes the
 * task to fall back to the compile-time defaults above.
 * ======================================================================= */

/** Configuration for ic_app_led_task(). */
typedef struct {
    uint32_t max_count;   /**< Number of LED toggles before the task exits.
                           *   Set to 0 to run indefinitely.                */
    uint32_t interval_ms; /**< Delay between toggles (ms).                  */
} ic_led_task_cfg_t;

/** Configuration for ic_app_tof_task(). */
typedef struct {
    uint32_t max_count;   /**< Number of TOF readings before the task exits.
                           *   Set to 0 to run indefinitely.                */
    uint32_t interval_ms; /**< Delay between readings (ms).                  */
} ic_tof_task_cfg_t;

/** Configuration for ic_app_temp_task(). */
typedef struct {
    uint32_t max_count;   /**< Number of temperature readings before exit.
                           *   Set to 0 to run indefinitely.                */
    uint32_t interval_ms; /**< Delay between readings (ms).                  */
} ic_temp_task_cfg_t;

/* =========================================================================
 * Public API
 * ======================================================================= */

/**
 * @brief  Initialises all application peripherals and sensors.
 *
 * Performs the following in order:
 *  1. Initialises the board LED.
 *  2. Optionally scans the I2C bus (requires IC_DEBUG_I2C_SCAN at build time).
 *  3. Attempts to initialise the BMP280 and VL53L0X sensors, each with up to
 *     IC_APP_INIT_MAX_RETRIES attempts.  A failed sensor sets the corresponding
 *     bit in g_app_error_flags but does not abort the boot sequence.
 *  4. Sets IC_APP_INIT_DONE_BIT in g_app_init_done so waiting tasks unblock.
 *
 * Must be called once from the startup task before any sensor task is allowed
 * to run.  Both g_app_init_done and g_app_error_flags must be created before
 * this function is called.
 */
void ic_app_init (void);

/**
 * @brief  LED toggle task entry point.
 *
 * Waits for IC_APP_INIT_DONE_BIT, then toggles the board LED at the
 * configured interval.  Exits after cfg->max_count toggles and turns
 * the LED on as a final indicator.
 *
 * @param argument  Pointer to an ic_led_task_cfg_t, or NULL to use defaults.
 */
void ic_app_led_task (void* argument);

/**
 * @brief  Time-of-flight (VL53L0X) task entry point.
 *
 * Waits for IC_APP_INIT_DONE_BIT, aborts immediately if
 * IC_APP_VL53L0X_FAIL_BIT is set, then reads distance at the configured
 * interval.  Exits after cfg->max_count successful readings.
 *
 * All I2C accesses are protected by i2cMutexHandle.
 *
 * @param argument  Pointer to an ic_tof_task_cfg_t, or NULL to use defaults.
 */
void ic_app_tof_task (void* argument);

/**
 * @brief  Temperature (BMP280) task entry point.
 *
 * Waits for IC_APP_INIT_DONE_BIT, aborts immediately if
 * IC_APP_BMP280_FAIL_BIT is set, then reads temperature at the configured
 * interval.  Exits after cfg->max_count successful readings.
 *
 * All I2C accesses are protected by i2cMutexHandle.
 *
 * @param argument  Pointer to an ic_temp_task_cfg_t, or NULL to use defaults.
 */
void ic_app_temp_task (void* argument);

#ifdef __cplusplus
}
#endif

#endif /* IC_APP_H */