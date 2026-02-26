#ifndef IC_LED_H
#define IC_LED_H

#include "stm32f4xx_hal.h"
#include "ic_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 LED 模組。
 *
 * 目前假設 LED GPIO 由 MX_GPIO_Init() 完成，
 * 這裡只做內部狀態標記，未來如有多顆 LED 可擴充。
 *
 * @return IC_STATUS_OK 一般情況下應該都會成功。
 *         若未來需要檢查 GPIO 狀態，可回傳：
 *         - IC_STATUS_LED_GPIO_ERROR 等。
 */
ic_status_t ic_led_init(void);

/**
 * @brief 切換 LED 狀態 (toggle)。
 *
 * @return IC_STATUS_OK                         : 操作成功
 *         IC_STATUS_LED_GPIO_ERROR / 其他錯誤 : 若未初始化或 GPIO 操作異常
 */
ic_status_t ic_led_toggle(void);

/**
 * @brief 將 LED 亮起 (ON)。
 */
ic_status_t ic_led_on(void);

/**
 * @brief 將 LED 熄滅 (OFF)。
 */
ic_status_t ic_led_off(void);

#ifdef __cplusplus
}
#endif

#endif /* IC_LED_H */