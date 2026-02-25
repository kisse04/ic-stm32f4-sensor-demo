#ifndef IC_LED_H
#define IC_LED_H

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the LED control module.
 *
 * GPIO pin setup is expected to be completed by board init code.
 */
void
ic_led_init (void);

/**
 * Toggles the target LED output state.
 */
void
ic_led_toggle (void);

/**
 * Sets the target LED to ON state.
 */
void
ic_led_on (void);

/**
 * Sets the target LED to OFF state.
 */
void
ic_led_off (void);

#ifdef __cplusplus
}
#endif

#endif /* IC_LED_H */
