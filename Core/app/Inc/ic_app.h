#ifndef IC_APP_H
#define IC_APP_H

#include "cmsis_os2.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes application modules and runtime state.
 *
 * This function is expected to be called once during system startup
 * before entering the main application loop.
 */
void
ic_app_init (void);

/**
 * LED task entry.
 *
 * The task period is configured in the task implementation.
 */
void
ic_app_led_task (void* argument);

/**
 * Time-of-flight (VL53L0X) task entry.
 *
 * The task period is configured in the task implementation.
 */
void
ic_app_tof_task (void* argument);

/**
 * Temperature (BMP280) task entry.
 *
 * The task period is configured in the task implementation.
 */
void
ic_app_temp_task (void* argument);

/** Mutex protecting shared I2C access. */
extern osMutexId_t i2cMutexHandle;

/** Application init completion event flags. */
extern osEventFlagsId_t g_app_init_done;

/** Event bit indicating application init completion. */
#define IC_APP_INIT_DONE_BIT  (1U << 0)

#ifdef __cplusplus
}
#endif

#endif /* IC_APP_H */
