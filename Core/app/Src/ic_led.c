/**
 * LED control driver.
 *
 * Provides basic init and GPIO control helpers for the on-board LED.
 */

#include "ic_led.h"
#include "main.h"

/** Initialization flag: 0 = not ready, 1 = ready. */
static uint8_t s_led_initialized = 0U;

/** Ensures the LED driver has been initialized. */
static ic_status_t
ic_led_ensure_initialized (void)
{
    if (s_led_initialized == 0U) {
        return IC_STATUS_LED_GPIO_ERROR;
    }

    return IC_STATUS_OK;
}

ic_status_t
ic_led_init (void)
{
    /* MX_GPIO_Init() is expected to configure the LED pins. */
    s_led_initialized = 1U;
    return IC_STATUS_OK;
}

ic_status_t
ic_led_toggle (void)
{
    ic_status_t st = ic_led_ensure_initialized ();
    if (!IC_STATUS_IS_OK (st)) {
        return st;
    }

    HAL_GPIO_TogglePin (LD2_GPIO_Port, LD2_Pin);
    return IC_STATUS_OK;
}

ic_status_t
ic_led_on (void)
{
    ic_status_t st = ic_led_ensure_initialized ();
    if (!IC_STATUS_IS_OK (st)) {
        return st;
    }

    HAL_GPIO_WritePin (LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
    return IC_STATUS_OK;
}

ic_status_t
ic_led_off (void)
{
    ic_status_t st = ic_led_ensure_initialized ();
    if (!IC_STATUS_IS_OK (st)) {
        return st;
    }

    HAL_GPIO_WritePin (LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
    return IC_STATUS_OK;
}
