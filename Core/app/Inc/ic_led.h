#ifndef IC_LED_H
#define IC_LED_H

#include "stm32f4xx_hal.h"
#include "ic_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes LED GPIO resources.
 *
 * The LED GPIOs must be configured by MX_GPIO_Init() before calling this
 * function.
 *
 * @return IC_STATUS_OK on success, otherwise IC_STATUS_LED_GPIO_ERROR.
 */
ic_status_t
ic_led_init (void);

/**
 * Toggles the LED state.
 *
 * @return IC_STATUS_OK on success, otherwise IC_STATUS_LED_GPIO_ERROR.
 */
ic_status_t
ic_led_toggle (void);

/**
 * Turns the LED on.
 *
 * @return IC_STATUS_OK on success, otherwise IC_STATUS_LED_GPIO_ERROR.
 */
ic_status_t
ic_led_on (void);

/**
 * Turns the LED off.
 *
 * @return IC_STATUS_OK on success, otherwise IC_STATUS_LED_GPIO_ERROR.
 */
ic_status_t
ic_led_off (void);

#ifdef __cplusplus
}
#endif

#endif /* IC_LED_H */
