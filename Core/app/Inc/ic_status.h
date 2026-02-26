#ifndef IC_STATUS_H
#define IC_STATUS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 通用狀態碼定義
 *
 * 0                : 正常
 * 11 ~ 19 (板子)   : Board / System level 問題
 * 21 ~ 29 (Bus)    : I2C/SPI/UART 等通訊通道問題
 * 31 ~ 39 (LED)    : LED 元件與控制相關問題
 * 41 ~ 49 (BMP280) : BMP280 感測器相關問題
 * 51 ~ 59 (VL53L0X): VL53L0X 感測器相關問題
 * 99               : 未分類 / 未知錯誤
 */

typedef enum
{
    /* 0: 正常 */
    IC_STATUS_OK = 0,

    /* 11–19: 板子 / 系統層問題 */
    IC_STATUS_BOARD_INIT_FAILED      = 11,  /**< 開機 / 初始化流程失敗 */
    IC_STATUS_BOARD_UNSUPPORTED_HW   = 12,  /**< 不支援的板子版本或硬體配置 */
    IC_STATUS_BOARD_CONFIG_ERROR     = 13,  /**< 系統設定錯誤（clock、pinmux 等） */
    IC_STATUS_BOARD_POWER_FAULT      = 14,  /**< 電源相關異常（電壓偵測 fail 等） */
    /* 15–19 保留 */

    /* 21–29: Bus / 通訊相關問題 (I2C/SPI/UART 等) */
    IC_STATUS_BUS_ERROR              = 21,  /**< 一般 bus 錯誤，不特別區分類型 */
    IC_STATUS_I2C_NACK               = 22,  /**< I2C NACK / 裝置沒回應 */
    IC_STATUS_I2C_TIMEOUT            = 23,  /**< I2C 逾時 */
    IC_STATUS_I2C_BUSY               = 24,  /**< I2C bus 忙碌，無法發出要求 */
    /* 25–29 保留 */

    /* 31–39: LED 相關問題 */
    IC_STATUS_LED_INVALID_CHANNEL    = 31,  /**< 傳入的 LED 通道或 ID 不存在 */
    IC_STATUS_LED_GPIO_ERROR         = 32,  /**< GPIO 初始失敗或無法操作 LED pin */
    /* 33–39 保留 */

    /* 41–49: BMP280 相關問題 */
    IC_STATUS_BMP280_INIT_FAILED     = 41,  /**< BMP280 初始化失敗 */
    IC_STATUS_BMP280_NOT_DETECTED    = 42,  /**< BMP280 裝置 ID 不正確或未偵測到 */
    IC_STATUS_BMP280_READ_FAILED     = 43,  /**< 讀取暫存器或資料失敗 */
    IC_STATUS_BMP280_CALIB_INVALID   = 44,  /**< 校正常數錯誤或不合理 */
    /* 45–49 保留 */

    /* 51–59: VL53L0X 相關問題 */
    IC_STATUS_VL53L0X_INIT_FAILED    = 51,  /**< VL53L0X 初始化失敗 */
    IC_STATUS_VL53L0X_NOT_DETECTED   = 52,  /**< VL53L0X 未偵測到或 ID 不正確 */
    IC_STATUS_VL53L0X_READ_FAILED    = 53,  /**< 量測或讀取資料失敗 */
    IC_STATUS_VL53L0X_OUT_OF_RANGE   = 54,  /**< 量測距離超出有效範圍 */
    /* 55–59 保留 */

    /* 通用 fallback */
    IC_STATUS_UNKNOWN                = 99   /**< 未分類 / 未知錯誤 */
} ic_status_t;

/**
 * @brief 回傳狀態碼的簡短文字描述（不含模組分類）
 */
const char *IC_Status_ToString(ic_status_t status);

/**
 * @brief 回傳狀態碼所屬的類別（Board / Bus / LED / BMP280 / VL53L0X / Unknown）
 */
const char *IC_Status_CategoryString(ic_status_t status);


/* 小工具 macro：方便在程式中判斷類型 */

#define IC_STATUS_IS_OK(s)          ((s) == IC_STATUS_OK)
#define IC_STATUS_IS_ERROR(s)       ((s) != IC_STATUS_OK)

#define IC_STATUS_IS_BOARD_ERR(s)   ((s) >= 11 && (s) <= 19)
#define IC_STATUS_IS_BUS_ERR(s)     ((s) >= 21 && (s) <= 29)
#define IC_STATUS_IS_LED_ERR(s)     ((s) >= 31 && (s) <= 39)
#define IC_STATUS_IS_BMP280_ERR(s)  ((s) >= 41 && (s) <= 49)
#define IC_STATUS_IS_VL53L0X_ERR(s) ((s) >= 51 && (s) <= 59)

#ifdef __cplusplus
}
#endif

#endif /* IC_STATUS_H */