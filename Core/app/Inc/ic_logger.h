#ifndef IC_LOGGER_H
#define IC_LOGGER_H

#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include <stdint.h>

/** Logger severity level. （目前可以先不使用，保留日後擴充） */
typedef enum {
    IC_LOG_LEVEL_INFO,
    IC_LOG_LEVEL_WARN,
    IC_LOG_LEVEL_ERROR,
    IC_LOG_LEVEL_DEBUG
} ic_log_level_t;

/* ============================================================
 *  低階 backend：UART 綁定 ＆ 直接寫
 * ============================================================ */

/**
 * @brief 初始化 logger backend，用哪一個 UART 來輸出 log。
 */
void ic_log_init(UART_HandleTypeDef *huart);

/**
 * @brief 直接 blocking 寫 UART（底層用 HAL_UART_Transmit）。
 * @return 實際寫出的 byte 數
 */
int ic_log_write(const uint8_t *data, uint16_t len);

/* ============================================================
 *  Logger Task + Queue 介面
 * ============================================================ */

/**
 * @brief Log 訊息結構。整個結構會被複製進 queue。
 * buf 長度可視需求調整（128 / 256 皆可）。
 */
typedef struct
{
    uint16_t len;
    char     buf[128];
} ic_log_msg_t;

/**
 * @brief Log queue handle
 *
 * 由 freertos.c 定義，例如：
 *   osMessageQueueId_t g_log_queue;
 *   g_log_queue = osMessageQueueNew(16, sizeof(ic_log_msg_t), &attr);
 */
extern osMessageQueueId_t g_log_queue;

/**
 * @brief Logger RTOS task，負責從 queue 裡撈 log，寫到 UART。
 *
 * 在 freertos.c 裡：
 *   osThreadNew(ic_log_task, NULL, &logTask_attributes);
 */
void ic_log_task(void *argument);

/**
 * @brief 非阻塞版 printf：把格式化字串排進 queue，由 logger task 寫出。
 *
 * @note 若 queue 已滿，目前的策略是丟棄該筆 log（回傳 0）。
 * @return 寫入 buffer 的字數（被截斷則為 buffer 大小），失敗回 0。
 */
int ic_log_printf(const char *fmt, ...);

#endif /* IC_LOGGER_H */