#ifndef IC_LOGGER_H
#define IC_LOGGER_H

#include <stdint.h>

#include "cmsis_os2.h"
#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Logger severity level. */
typedef enum {
    IC_LOG_LEVEL_INFO,
    IC_LOG_LEVEL_WARN,
    IC_LOG_LEVEL_ERROR,
    IC_LOG_LEVEL_DEBUG
} ic_log_level_t;

/**
 * Initializes the logger backend with a UART handle.
 *
 * @param[in] huart is the UART handle used for log output.
 */
void
ic_log_init (UART_HandleTypeDef* huart);

/**
 * Writes log data in a blocking manner using HAL_UART_Transmit.
 *
 * @param[in] data is the buffer to transmit.
 * @param[in] len is the number of bytes to transmit.
 *
 * @return Number of bytes written, or a negative value on error.
 */
int
ic_log_write (const uint8_t* data, uint16_t len);

/**
 * Log message format for the logger queue.
 */
typedef struct {
    uint16_t len;
    char buf[128];
} ic_log_msg_t;

/**
 * Logger queue handle.
 *
 * Created in freertos.c, for example:
 *   osMessageQueueId_t g_log_queue;
 *   g_log_queue = osMessageQueueNew(16, sizeof(ic_log_msg_t), &attr);
 */
extern osMessageQueueId_t g_log_queue;

/**
 * Logger RTOS task entry.
 *
 * Created in freertos.c, for example:
 *   osThreadNew(ic_log_task, NULL, &logTask_attributes);
 */
void
ic_log_task (void* argument);

/**
 * Formats a log message and enqueues it for the logger task.
 *
 * @param[in] fmt is a printf-style format string.
 *
 * @return Number of bytes written into the queue buffer, or 0 on failure.
 */
int
ic_log_printf (const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* IC_LOGGER_H */
