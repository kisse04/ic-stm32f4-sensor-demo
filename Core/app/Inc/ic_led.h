#ifndef IC_LED_H
#define IC_LED_H

#include "stm32f4xx_hal.h"

void ic_led_init(void);
void ic_led_toggle(void);
void ic_led_on(void);
void ic_led_off(void);

#endif /* IC_LED_H */