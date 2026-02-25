/**
 * UART logger backend implementation + logger task.
 */

#include "ic_logger.h"

#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

/** UART instance used by logger APIs. */
static UART_HandleTypeDef *s_log_uart = NULL;

/* g_log_queue 的定義在 freertos.c，
 * 這裡只需要透過 header 的 extern 使用即可。
 * extern osMessageQueueId_t g_log_queue;  // 已在 ic_logger.h 宣告
 */

void ic_log_init(UART_HandleTypeDef *huart)
{
    s_log_uart = huart;
}

int ic_log_write(const uint8_t *data, uint16_t len)
{
    if ((s_log_uart == NULL) || (data == NULL) || (len == 0U)) {
        return 0;
    }

    (void)HAL_UART_Transmit(s_log_uart, (uint8_t *)data, len, HAL_MAX_DELAY);
    return (int)len;
}

/* ============================================================
 *  Logger Task：負責從 queue 裡撈出訊息並輸出 UART
 * ============================================================ */

void ic_log_task(void *argument)
{
    (void)argument;

    ic_log_msg_t msg;

    for (;;)
    {
        /* 阻塞等待一筆 log 訊息 */
        if (osMessageQueueGet(g_log_queue, &msg, NULL, osWaitForever) == osOK)
        {
            (void)ic_log_write((const uint8_t *)msg.buf, msg.len);
        }
    }
}

/* ============================================================
 *  非同步 printf：將字串排進 queue
 * ============================================================ */

int ic_log_printf(const char *fmt, ...)
{
    if ((fmt == NULL) || (g_log_queue == NULL)) {
        return 0;
    }

    ic_log_msg_t msg;
    va_list args;

    va_start(args, fmt);
    int n = vsnprintf(msg.buf, sizeof(msg.buf), fmt, args);
    va_end(args);

    if (n < 0) {
        return 0;
    }

    if (n > (int)sizeof(msg.buf)) {
        n = (int)sizeof(msg.buf);  // 超出就截斷
    }
    msg.len = (uint16_t)n;

    /* 不阻塞：queue 滿了就丟這筆 log */
    if (osMessageQueuePut(g_log_queue, &msg, 0, 0) != osOK) {
        return 0;
    }

    return n;
}