#ifndef IC_LOGGER_H
#define IC_LOGGER_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/** Logger severity level. */
typedef enum {
    IC_LOG_LEVEL_INFO,
    IC_LOG_LEVEL_WARN,
    IC_LOG_LEVEL_ERROR,
    IC_LOG_LEVEL_DEBUG
} ic_log_level_t;

/**
 * Initializes logger backend UART.
 *
 * @param[in] huart is the UART handle used for log output.
 */
void ic_log_init(UART_HandleTypeDef *huart);

/**
 * Writes raw log bytes to the logger backend.
 *
 * @param[in] data is the byte buffer to write.
 * @param[in] len is the number of bytes to write.
 *
 * @return Number of bytes written. Returns 0 for invalid input or uninitialized backend.
 */
int ic_log_write(const uint8_t *data, uint16_t len);

/**
 * Writes a formatted log line.
 *
 * @param[in] level is the log severity.
 * @param[in] fmt is printf-style format string.
 * @param[in] ... are format arguments.
 */
void ic_log_printf(ic_log_level_t level, const char *fmt, ...);

/** Shortcut for info level log. */
#define IC_LOGI(fmt, ...) ic_log_printf(IC_LOG_LEVEL_INFO,  fmt, ##__VA_ARGS__)
/** Shortcut for warn level log. */
#define IC_LOGW(fmt, ...) ic_log_printf(IC_LOG_LEVEL_WARN,  fmt, ##__VA_ARGS__)
/** Shortcut for error level log. */
#define IC_LOGE(fmt, ...) ic_log_printf(IC_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
/** Shortcut for debug level log. */
#define IC_LOGD(fmt, ...) ic_log_printf(IC_LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)

#endif /* IC_LOGGER_H */
