#ifndef IC_LOGGER_H
#define IC_LOGGER_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef enum {
    IC_LOG_LEVEL_INFO,
    IC_LOG_LEVEL_WARN,
    IC_LOG_LEVEL_ERROR,
    IC_LOG_LEVEL_DEBUG
} ic_log_level_t;

/* 綁定底層輸出埠，例如 huart2 */
void ic_log_init(UART_HandleTypeDef *huart);

/* 低階 API：輸出 raw buffer */
int  ic_log_write(const uint8_t *data, uint16_t len);

/* 高階 API：printf 風格 */
void ic_log_printf(ic_log_level_t level, const char *fmt, ...);

/* 宏：方便使用 */
#define IC_LOGI(fmt, ...) ic_log_printf(IC_LOG_LEVEL_INFO,  fmt, ##__VA_ARGS__)
#define IC_LOGW(fmt, ...) ic_log_printf(IC_LOG_LEVEL_WARN,  fmt, ##__VA_ARGS__)
#define IC_LOGE(fmt, ...) ic_log_printf(IC_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define IC_LOGD(fmt, ...) ic_log_printf(IC_LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)

#endif /* IC_LOGGER_H */