/**
 * UART logger backend implementation and logger task.
 */

#include "ic_logger.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "cmsis_os2.h"
#include "stm32f4xx_hal.h"

/** UART instance used by logger APIs. */
static UART_HandleTypeDef* s_log_uart = NULL;

void
ic_log_init (UART_HandleTypeDef* huart)
{
    s_log_uart = huart;
}

int
ic_log_write (const uint8_t* data, uint16_t len)
{
    if ((s_log_uart == NULL) || (data == NULL) || (len == 0U)) {
        return 0;
    }

    (void) HAL_UART_Transmit (s_log_uart, (uint8_t*) data, len, HAL_MAX_DELAY);
    return (int) len;
}

void
ic_log_task (void* argument)
{
    (void) argument;

    ic_log_msg_t msg;

    for (;;) {
        if (osMessageQueueGet (g_log_queue, &msg, NULL, osWaitForever) == osOK) {
            (void) ic_log_write ((const uint8_t*) msg.buf, msg.len);
        }
    }
}

int
ic_log_printf (const char* fmt, ...)
{
    if ((fmt == NULL) || (g_log_queue == NULL)) {
        return 0;
    }

    ic_log_msg_t msg;
    va_list args;

    va_start (args, fmt);
    int n = vsnprintf (msg.buf, sizeof (msg.buf), fmt, args);
    va_end (args);

    if (n < 0) {
        return 0;
    }

    if (n > (int) sizeof (msg.buf)) {
        n = (int) sizeof (msg.buf);
    }
    msg.len = (uint16_t) n;

    if (osMessageQueuePut (g_log_queue, &msg, 0, 0) != osOK) {
        return 0;
    }

    return n;
}
