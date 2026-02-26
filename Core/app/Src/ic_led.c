#include "ic_led.h"
#include "main.h"   // 使用 LD2_GPIO_Port / LD2_Pin

/** 簡單的初始化旗標：0 = 未初始化, 1 = 已初始化 */
static uint8_t s_led_initialized = 0U;

/* 內部小工具：確認初始化狀態 */
static ic_status_t ic_led_ensure_initialized(void)
{
    if (s_led_initialized == 0U) {
        /* 這裡用 LED GPIO 錯誤來表示 "還沒準備好" */
        return IC_STATUS_LED_GPIO_ERROR;
    }
    return IC_STATUS_OK;
}

ic_status_t ic_led_init(void)
{
    /* 目前假設 MX_GPIO_Init() 已在 system init 裡呼叫 */
    s_led_initialized = 1U;
    return IC_STATUS_OK;
}

ic_status_t ic_led_toggle(void)
{
    ic_status_t st = ic_led_ensure_initialized();
    if (!IC_STATUS_IS_OK(st)) {
        return st;
    }

    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    return IC_STATUS_OK;
}

ic_status_t ic_led_on(void)
{
    ic_status_t st = ic_led_ensure_initialized();
    if (!IC_STATUS_IS_OK(st)) {
        return st;
    }

    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
    return IC_STATUS_OK;
}

ic_status_t ic_led_off(void)
{
    ic_status_t st = ic_led_ensure_initialized();
    if (!IC_STATUS_IS_OK(st)) {
        return st;
    }

    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
    return IC_STATUS_OK;
}