# ic-stm32f4-sensor-demo

**README Languages:** 
[English](https://github.com/kisse04/ic-stm32f4-sensor-demo/blob/main/readme.md) | [中文說明](https://github.com/kisse04/ic-stm32f4-sensor-demo/blob/main/readme_zh-TW.md)

更新日期：2026-02-27


STM32F446RE (NUCLEO-F446RE) + BMP280 溫度感測器 + VL53L0X ToF 距離感測器
示範如何使用 I2C 讀取多顆感測器，並透過 UART 輸出 log。

> **EN summary**  
> This repository contains an STM32F446RE (NUCLEO-F446RE) demo project integrating two I2C sensors (BMP280 for temperature and VL53L0X ToF distance).  
> It showcases:
> - Clean separation between App / Drivers / HAL (CubeMX generated)
> - Handle-based sensor drivers with shared status codes (ic_status)
> - A lightweight logging abstraction on top of UART with an RTOS queue/task (ic_logger)
> - FreeRTOS/CMSIS-RTOS v2 task scheduling for the app flow
> - Retry-based sensor init, task-level graceful abort, and I2C mutex protection
> - Optional non-stop task mode (`max_count=0`) and dropped-log counting on queue overflow
> - A custom GCC+Python build flow decoupled from STM32CubeIDE

---

## 1. 專案目標

- 透過 STM32F4 Nucleo 開發板，整合兩顆 I2C 感測器：
  - BMP280：溫度 / 氣壓（此專案目前示範溫度讀取）
  - VL53L0X：ToF 距離量測
- 提供一套 **ic_ 前綴** 的驅動 API，方便後續擴充或移植到其他專案。
- 示範：
  - I2C 多裝置併聯
  - UART log 抽象化（ic_logger）
  - App / Drivers / HAL 的分層設計

---

## 2. 硬體環境

- 開發板：NUCLEO-F446RE
- 感測器：
  - BMP280 模組（3.3V）
  - VL53L0X 模組（3.3V）
- 介面：
  - I2C1：同一條 bus 併聯 BMP280 + VL53L0X
  - USART2：透過 ST-LINK Virtual COM Port 連到 PC（Putty/TeraTerm）

> - SCL/SDA 腳位（同時使用 PB8/PB9 D15/D14）
> - VCC / GND 接腳

---

## 3. 軟體架構

專案分為四層：

1. **App 層 (`ic_app`)**
   - `ic_app_init()`：初始化 LED，並以 retry 機制初始化感測器
   - `ic_app_led_task()`：預設 500ms 週期切換 LED（預設 20 次後結束）
   - `ic_app_tof_task()`：預設 250ms 週期讀 VL53L0X（預設 40 次後結束）
   - `ic_app_temp_task()`：預設 1000ms 週期讀 BMP280 溫度（預設 10 次後結束）
   - 透過 task argument 傳入 `ic_*_task_cfg_t` 可覆寫每個 task 的次數/週期（`max_count=0` 代表持續運行，不自動結束）
2. **RTOS 中間層（FreeRTOS/CMSIS-RTOS v2）**
   - `Core/Src/freertos.c`：任務/排程初始化與 RTOS 啟動點
   - Logger queue + logger task（`g_log_queue`, `ic_log_task`）
   - I2C mutex（`i2cMutexHandle`）與 app event flags（`g_app_init_done`, `g_app_error_flags`）
   - `Middlewares/Third_Party/FreeRTOS`：FreeRTOS 核心與 CMSIS-RTOS v2 介面
3. **Driver / Middleware 層**
   - `ic_bmp280`：BMP280 驅動
   - `ic_vl53l0x`：VL53L0X 驅動
   - `ic_led`：板上 LED 控制
   - `ic_logger`：log 介面（目前實作為 UART 輸出）
   - `ic_status`：統一的狀態碼與分類函式
4. **HAL / BSP 層（CubeMX 生成）**
   - GPIO / I2C / USART 初始設定
   - clock / 中斷 / 系統啟動程式碼

簡化架構圖：

```
          +----------------------+
          |      ic_app          |
          |  - ic_app_init()     |
          |  - ic_app_*_task()   |
          +----------+-----------+
                     |
        +------------+-------------+
        |                          |
  +-----v------+            +------v------+
  | ic_bmp280 |            | ic_vl53l0x  |
  +-----------+            +-------------+
        |                          |
  +-----v------+            +------v------+
  |   I2C1     | (stm32 HAL drivers)     |
  +------------+-------------------------+

  +-----------+     +-------------+
  | ic_logger | --> |   USART2    |
  +-----------+     +-------------+
        ^
        |
   log queue/task

  +--------+
  | ic_led |
  +--------+ --> GPIO

  +---------------------------+
  |   FreeRTOS / CMSIS-RTOS   |
  +---------------------------+
         | (task schedule)
         +--> ic_app_*_task()
```

### 3.1 接線圖（易讀版）

1. 共用 I2C 匯流排（兩顆感測器共用）
   - SCL：`D15 (PB8)`
   - SDA：`D14 (PB9)`
2. 連接 BMP280（3.3V）
3. 連接 VL53L0X（3.3V）

BMP280 接線對照

| BMP280 腳位 | NUCLEO 接頭 | NUCLEO 腳位 | 功能 |
| --- | --- | --- | --- |
| VCC | CN8 Pin 4 | +3V3 | 電源 3.3V |
| GND | CN8 Pin 6 | GND | 接地 |
| SCL | CN5 Pin 10 | D15 (PB8) | I2C1 時鐘 |
| SDA | CN5 Pin 9 | D14 (PB9) | I2C1 資料 |

VL53L0X 接線對照

| VL53L0X 腳位 | NUCLEO 接頭 | NUCLEO 腳位 | 功能 |
| --- | --- | --- | --- |
| VCC | CN7 Pin 16 | +3V3 | 電源 3.3V |
| GND | CN7 Pin 20 | GND | 接地 |
| SCL | CN10 Pin 3 | D15 (PB8) | I2C1 時鐘（共用） |
| SDA | CN10 Pin 5 | D14 (PB9) | I2C1 資料（共用） |

I2C 位址對照

| 感測器 | I2C 位址 | 備註 |
| --- | --- | --- |
| BMP280 | `0x76` | SDO 接 GND 時為此位址 |
| VL53L0X | `0x29` | 預設位址 |


---

## 4. 專案目錄結構

```
Core/
  Inc/
    main.h
    gpio.h
    i2c.h
    usart.h
    stm32f4xx_hal_conf.h
    stm32f4xx_it.h

  Src/
    main.c
    freertos.c
    gpio.c
    i2c.c
    usart.c
    stm32f4xx_hal_msp.c
    stm32f4xx_it.c
    system_stm32f4xx.c
    syscalls.c
    sysmem.c

  app/
    Inc/
      ic_app.h         # App entry
      ic_bmp280.h      # BMP280 driver API
      ic_led.h         # LED 控制 API
      ic_logger.h      # logger API (log level, printf)
      ic_status.h      # status code helpers
      ic_vl53l0x.h     # VL53L0X driver API

    Src/
      ic_app.c
      ic_bmp280.c
      ic_led.c
      ic_logger_uart.c
      ic_status.c
      ic_vl53l0x.c

Middlewares/
  Third_Party/
    FreeRTOS/
      Source/
        CMSIS_RTOS_V2/
        include/
        portable/
```

---

## 5. 主要模組說明

### 5.1 ic_app

```c
void ic_app_init(void);
void ic_app_led_task(void *argument);
void ic_app_tof_task(void *argument);
void ic_app_temp_task(void *argument);
```

- `ic_app_init()`
  - 初始化 LED
  - 初始化 BMP280 / VL53L0X（每顆最多重試 `IC_APP_INIT_MAX_RETRIES` 次）
  - 若感測器持續失敗，設定對應 error bit（`IC_APP_BMP280_FAIL_BIT` / `IC_APP_VL53L0X_FAIL_BIT`）
  - `IC_DEBUG_I2C_SCAN` 定義存在時才執行 I2C 掃描（避免 production 開機延遲）
- `ic_app_led_task()`
  - 等待 `IC_APP_INIT_DONE_BIT`
  - 依 `ic_led_task_cfg_t` 週期切換 LED（預設 500ms）
- `ic_app_tof_task()`
  - 等待 `IC_APP_INIT_DONE_BIT`
  - 若 `IC_APP_VL53L0X_FAIL_BIT` 已設置，task 會直接結束
  - 依 `ic_tof_task_cfg_t` 週期讀取 VL53L0X（預設 250ms）
  - 所有 I2C 存取都使用 `i2cMutexHandle` 保護
- `ic_app_temp_task()`
  - 等待 `IC_APP_INIT_DONE_BIT`
  - 若 `IC_APP_BMP280_FAIL_BIT` 已設置，task 會直接結束
  - 依 `ic_temp_task_cfg_t` 週期讀取 BMP280（預設 1000ms）
  - 所有 I2C 存取都使用 `i2cMutexHandle` 保護

### 5.2 ic_bmp280

```c
typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t            i2c_addr;
    uint16_t           T1;
    int16_t            T2;
    int16_t            T3;
    uint8_t            is_initialized;
} ic_bmp280_handle_t;

ic_status_t ic_bmp280_init(ic_bmp280_handle_t *dev,
                           I2C_HandleTypeDef  *hi2c,
                           uint8_t             i2c_addr);

ic_status_t ic_bmp280_read_temperature(ic_bmp280_handle_t *dev,
                                       float              *temp_c);
```

### 5.3 ic_vl53l0x

```c
typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t            i2c_addr;
    uint8_t            is_initialized;
} ic_vl53l0x_handle_t;

ic_status_t ic_vl53l0x_init(ic_vl53l0x_handle_t *dev,
                            I2C_HandleTypeDef  *hi2c,
                            uint8_t             i2c_addr);

ic_status_t ic_vl53l0x_read_distance_mm(ic_vl53l0x_handle_t *dev,
                                        uint16_t           *distance_mm);
```

### 5.4 ic_logger

```c
typedef enum {
    IC_LOG_LEVEL_INFO,
    IC_LOG_LEVEL_WARN,
    IC_LOG_LEVEL_ERROR,
    IC_LOG_LEVEL_DEBUG
} ic_log_level_t;

void ic_log_init(UART_HandleTypeDef *huart);

int ic_log_write(const uint8_t *data, uint16_t len);

void ic_log_task(void *argument);

int ic_log_printf(const char *fmt, ...);
uint32_t ic_log_get_dropped_count(void);
```

- `ic_log_printf()`: returns written length on success; returns `-1` when queue is full (message dropped); returns `0` on format error.
- `ic_log_get_dropped_count()`: returns cumulative dropped log message count since `ic_log_init()`.

### 5.5 ic_led

```c
void ic_led_init(void);
void ic_led_toggle(void);
void ic_led_on(void);
void ic_led_off(void);
```

### 5.6 ic_status

```c
typedef enum {
    IC_STATUS_OK = 0,
    IC_STATUS_BOARD_INIT_FAILED = 11,
    IC_STATUS_BOARD_UNSUPPORTED_HW = 12,
    IC_STATUS_BOARD_CONFIG_ERROR = 13,
    IC_STATUS_BOARD_POWER_FAULT = 14,
    IC_STATUS_BUS_ERROR = 21,
    IC_STATUS_I2C_NACK = 22,
    IC_STATUS_I2C_TIMEOUT = 23,
    IC_STATUS_I2C_BUSY = 24,
    IC_STATUS_LED_INVALID_CHANNEL = 31,
    IC_STATUS_LED_GPIO_ERROR = 32,
    IC_STATUS_BMP280_INIT_FAILED = 41,
    IC_STATUS_BMP280_NOT_DETECTED = 42,
    IC_STATUS_BMP280_READ_FAILED = 43,
    IC_STATUS_BMP280_CALIB_INVALID = 44,
    IC_STATUS_VL53L0X_INIT_FAILED = 51,
    IC_STATUS_VL53L0X_NOT_DETECTED = 52,
    IC_STATUS_VL53L0X_READ_FAILED = 53,
    IC_STATUS_VL53L0X_OUT_OF_RANGE = 54,
    IC_STATUS_UNKNOWN = 99
} ic_status_t;

const char *IC_Status_ToString(ic_status_t status);
const char *IC_Status_CategoryString(ic_status_t status);
```

---

## 6. 建置與燒錄

### 6.1 使用 GCC + Python build 腳本

```bash
# 清除舊的輸出
python build.py --clean

# 編譯
python build.py

# debug build（會開啟 IC_DEBUG_I2C_SCAN）
python build.py --debug

# 編譯成功後自動燒錄
python build.py --flash

# debug build + 自動燒錄
python build.py --debug --flash
```

編譯成功後，會在 `out_gcc/`（或你設定的輸出資料夾）產生 `.elf` / `.hex`。

可選設定（優先序：CLI 參數 > 環境變數 > `build.py` 內建預設）：

```bash
set STM32_GCC_BIN=C:\...\gnu-tools-for-stm32\...\tools\bin
set STM32_PROGRAMMER_BIN=C:\...\cubeprogrammer\...\tools\bin
python build.py --gcc-bin "C:\path\to\gcc\bin" --programmer-bin "C:\path\to\programmer\bin" --flash
```

### 6.2 使用 STM32_Programmer_CLI 手動燒錄（可選）

```bash
set PATH=%PATH%;C:\ST\STM32CubeIDE_2.0.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.300.202508131133\tools\bin
STM32_Programmer_CLI.exe -c port=SWD -w out_gcc/your_project.hex -v -rst
```

---

## 7. 執行 & UART log 範例

使用 Putty 連線至 ST-LINK Virtual COM：

- Serial line：`COM3`
- Baud rate：`115200`（請依實際設定調整）
- Data bits：`8`
- Parity：`None`
- Flow control：`None`

示例輸出（實際 boot log）：

```
[Nucleo_F446RE] Boot OK!
[Nucleo_F446RE] Scanning I2C...
[Nucleo_F446RE] Found device at 0x29
[Nucleo_F446RE] Found device at 0x76
[BMP280] chip_id read id=0x58 (expect 0x58 or 0x60)
[BMP280] calib raw: 6AB4 670E FC18
[BMP280] T1=27316 T2=26382 T3=-1000
[BMP280] init OK
[Nucleo_F446RE] Sensor init finish
[LED] task started
[LED] toggle 1
[VL53L0X] Tof task started
[VL53L0X] Tof detect 1 - distance: 135 mm
[BMP280] Temp task started
[BMP280] Temp detect 0 - Temp: 29.55 C
```


