# ic-stm32f4-sensor-demo
更新日期：2026-02-24


STM32F446RE (NUCLEO-F446RE) + BMP280 溫度感測器 + VL53L0X ToF 距離感測器
示範如何使用 I2C 讀取多顆感測器，並透過 UART 輸出 log。

> **EN summary**  
> This repository contains a bare-metal STM32F446RE (NUCLEO-F446RE) demo project integrating two I2C sensors (BMP280 for temperature and VL53L0X ToF distance).  
> It showcases:
> - Clean separation between App / Drivers / HAL (CubeMX generated)
> - Handle-based sensor drivers with status enums
> - A lightweight logging abstraction on top of UART (ic_logger)
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
   - `ic_app_init()`：初始化 LED、感測器與 I2C 掃描
   - `ic_app_led_task()`：500ms 週期切換 LED（20 次後結束）
   - `ic_app_tof_task()`：250ms 週期讀 VL53L0X（40 次後結束）
   - `ic_app_temp_task()`：1000ms 週期讀 BMP280 溫度（10 次後結束）
2. **RTOS 中間層（FreeRTOS/CMSIS-RTOS v2）**
   - `Core/Src/freertos.c`：任務/排程初始化與 RTOS 啟動點
   - `Middlewares/Third_Party/FreeRTOS`：FreeRTOS 核心與 CMSIS-RTOS v2 介面
3. **Driver / Middleware 層**
   - `ic_bmp280`：BMP280 驅動
   - `ic_vl53l0x`：VL53L0X 驅動
   - `ic_led`：板上 LED 控制
   - `ic_logger`：log 介面（目前實作為 UART 輸出）
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

  +--------+
  | ic_led |
  +--------+ --> GPIO

  +---------------------------+
  |   FreeRTOS / CMSIS-RTOS   |
  +---------------------------+
         | (task schedule)
         +--> ic_app_*_task()
```

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
      ic_vl53l0x.h     # VL53L0X driver API

    Src/
      ic_app.c
      ic_bmp280.c
      ic_led.c
      ic_logger_uart.c
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
  - 執行 I2C 掃描
  - 初始化 BMP280 / VL53L0X
- `ic_app_led_task()`
  - 500ms 週期切換 LED
- `ic_app_tof_task()`
  - 250ms 週期讀取 VL53L0X 距離
  - I2C 存取前後使用 mutex
- `ic_app_temp_task()`
  - 1000ms 週期讀取 BMP280 溫度
  - I2C 存取前後使用 mutex

### 5.2 ic_bmp280

```c
typedef enum {
    IC_BMP280_OK = 0,
    IC_BMP280_ERROR = -1,
    IC_BMP280_BAD_ID = -2,
    IC_BMP280_NOT_INITIALIZED = -3
} ic_bmp280_status_t;

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t            i2c_addr;
    uint16_t           T1;
    int16_t            T2;
    int16_t            T3;
    uint8_t            is_initialized;
} ic_bmp280_handle_t;

ic_bmp280_status_t ic_bmp280_init(ic_bmp280_handle_t *dev,
                                  I2C_HandleTypeDef  *hi2c,
                                  uint8_t             i2c_addr);

ic_bmp280_status_t ic_bmp280_read_temperature(ic_bmp280_handle_t *dev,
                                              float              *temp_c);
```

### 5.3 ic_vl53l0x

```c
typedef enum {
    IC_VL53L0X_OK = 0,
    IC_VL53L0X_ERROR = -1,
    IC_VL53L0X_TIMEOUT = -2,
    IC_VL53L0X_NOT_INITIALIZED = -3
} ic_vl53l0x_status_t;

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t            i2c_addr;
    uint8_t            is_initialized;
} ic_vl53l0x_handle_t;

ic_vl53l0x_status_t ic_vl53l0x_init(ic_vl53l0x_handle_t *dev,
                                   I2C_HandleTypeDef  *hi2c,
                                   uint8_t             i2c_addr);

ic_vl53l0x_status_t ic_vl53l0x_read_distance_mm(ic_vl53l0x_handle_t *dev,
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

void ic_log_printf(ic_log_level_t level, const char *fmt, ...);

#define IC_LOGI(fmt, ...) ic_log_printf(IC_LOG_LEVEL_INFO,  fmt, ##__VA_ARGS__)
#define IC_LOGW(fmt, ...) ic_log_printf(IC_LOG_LEVEL_WARN,  fmt, ##__VA_ARGS__)
#define IC_LOGE(fmt, ...) ic_log_printf(IC_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define IC_LOGD(fmt, ...) ic_log_printf(IC_LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
```

### 5.5 ic_led

```c
void ic_led_init(void);
void ic_led_toggle(void);
void ic_led_on(void);
void ic_led_off(void);
```

---

## 6. 建置與燒錄

### 6.1 使用 GCC + Python build 腳本

```bash
# 清除舊的輸出
python build.py --clean

# 編譯
python build.py
```

編譯成功後，會在 `out_gcc/`（或你設定的輸出資料夾）產生 `.elf` / `.hex`。

### 6.2 使用 STM32_Programmer_CLI 燒錄

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
Boot OK!
Scanning I2C...
Found device at 0x29
Found device at 0x76
[BMP280] chip_id read id=0x58 (expect 0x58 or 0x60)
[BMP280] calib raw: 6AB4 670E FC18
[BMP280] T1=27316 T2=26382 T3=-1000
[BMP280] init OK
VL53L0X init OK
Tof detect 1 - distance: 135 mm
[BMP280] temp raw: 81 B8 00
Temp: 29.55 C
LED toggle 1
Tof detect 2 - distance: 138 mm
LED toggle 2
Tof detect 3 - distance: 139 mm
Tof detect 4 - distance: 139 mm
[BMP280] temp raw: 81 B7 00
Temp: 29.54 C
LED toggle 3
Tof detect 5 - distance: 136 mm
Tof detect 6 - distance: 135 mm
LED toggle 4
Tof detect 7 - distance: 137 mm
[BMP280] temp raw: 81 B9 00
Temp: 29.55 C
LED toggle 5
Tof detect 8 - distance: 136 mm
Tof detect 9 - distance: 137 mm
LED toggle 6
Tof detect 10 - distance: 135 mm
Tof detect 11 - distance: 136 mm
[BMP280] temp raw: 81 BA 00
Temp: 29.56 C
LED toggle 7
Tof detect 12 - distance: 137 mm
LED toggle 8
Tof detect 13 - distance: 136 mm
Tof detect 14 - distance: 136 mm
[BMP280] temp raw: 81 B9 00
Temp: 29.55 C
LED toggle 9
Tof detect 15 - distance: 138 mm
Tof detect 16 - distance: 137 mm
LED toggle 10
Tof detect 17 - distance: 138 mm
[BMP280] temp raw: 81 B9 00
Temp: 29.55 C
LED toggle 11
Tof detect 18 - distance: 137 mm
Tof detect 19 - distance: 135 mm
LED toggle 12
Tof detect 20 - distance: 138 mm
Tof detect 21 - distance: 137 mm
[BMP280] temp raw: 81 B9 00
Temp: 29.55 C
LED toggle 13
Tof detect 22 - distance: 136 mm
LED toggle 14
Tof detect 23 - distance: 136 mm
Tof detect 24 - distance: 135 mm
[BMP280] temp raw: 81 B8 00
Temp: 29.55 C
LED toggle 15
Tof detect 25 - distance: 137 mm
Tof detect 26 - distance: 137 mm
LED toggle 16
Tof detect 27 - distance: 135 mm
[BMP280] temp raw: 81 B9 00
Temp: 29.55 C
LED toggle 17
Tof detect 28 - distance: 135 mm
Tof detect 29 - distance: 136 mm
LED toggle 18
Tof detect 30 - distance: 137 mm
Tof detect 31 - distance: 136 mm
[BMP280] temp raw: 81 B8 00
Temp: 29.55 C
Done. temp stopped.
LED toggle 19
Tof detect 32 - distance: 132 mm
LED toggle 20
Done. LED on.
Tof detect 33 - distance: 136 mm
Tof detect 34 - distance: 137 mm
Tof detect 35 - distance: 138 mm
Tof detect 36 - distance: 135 mm
Tof detect 37 - distance: 136 mm
Tof detect 38 - distance: 136 mm
Tof detect 39 - distance: 136 mm
Tof detect 40 - distance: 136 mm
Done. tof stopped.
```