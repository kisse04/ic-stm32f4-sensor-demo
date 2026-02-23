/**
 * Board LED control helpers.
 */

#include "ic_led.h"
#include "main.h"

void ic_led_init(void)
{
    /* GPIO initialization is performed by MX_GPIO_Init(). */
}

void ic_led_toggle(void)
{
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
}

void ic_led_on(void)
{
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
}

void ic_led_off(void)
{
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
}
