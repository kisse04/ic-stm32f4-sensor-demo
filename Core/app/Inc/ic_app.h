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
 * @brief LED Task.
 * 週期性切換 LED（500ms）
 */
void
ic_app_led_task (void* argument);

/**
 * @brief TOF Task.
 * 週期性讀 VL53L0X（250ms）
 */
void
ic_app_tof_task (void* argument);

/**
 * @brief Temperature Task.
 * 週期性讀 BMP280（1000ms）
 */
void
ic_app_temp_task (void* argument);

extern osMutexId_t i2cMutexHandle;

#ifdef __cplusplus
}
#endif

#endif /* IC_APP_H */
