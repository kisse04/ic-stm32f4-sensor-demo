#include "ic_logger.h"
#include "stm32f4xx_hal.h"   // HAL_UART_Transmit, HAL_MAX_DELAY
#include <stddef.h>          // NULL

static UART_HandleTypeDef* s_log_uart = NULL;

void ic_log_init(UART_HandleTypeDef* huart)
{
    s_log_uart = huart;
}

int ic_log_write(const uint8_t* data, uint16_t len)
{
    if ((s_log_uart == NULL) || (data == NULL) || (len == 0U)) {
        return 0;
    }

    (void)HAL_UART_Transmit(s_log_uart, (uint8_t*)data, (uint16_t)len, HAL_MAX_DELAY);
    return (int)len;
}
