Copyright (c) 2026. All rights reserved.

================================================================================
RELEASE 2026-02-27 (latest)
- Added non-stop detect support for app tasks:
  - `ic_led_task_cfg_t.max_count = 0` means run forever.
  - `ic_tof_task_cfg_t.max_count = 0` means run forever.
  - `ic_temp_task_cfg_t.max_count = 0` means run forever.
  - Updated runtime loops in `ic_app_*_task()` to respect infinite mode.

- Added logger queue-full handling and dropped-count API:
  - `ic_log_printf()` now returns `-1` when queue is full (message dropped).
  - Added `ic_log_get_dropped_count()` to query cumulative dropped messages.
  - Added runtime warning log in TOF task when dropped messages are detected.

- Enhanced build script with debug/flash workflow:
  - Added `--debug` to inject `-DIC_DEBUG_I2C_SCAN`.
  - Added `--flash` to auto-run `STM32_Programmer_CLI.exe` after successful build.
  - Added `--programmer-bin`, `STM32_PROGRAMMER_BIN`, and fallback programmer path.
  - Added centralized default constants for project/output/log/linker script paths.

- Updated documentation:
  - readme_zh-TW.md

================================================================================
RELEASE 2026-02-27
- Refactor `ic_app` startup/task flow for stronger runtime robustness:
  - Added bounded retry init for BMP280/VL53L0X (`IC_APP_INIT_MAX_RETRIES`, `IC_APP_INIT_RETRY_DELAY_MS`).
  - Added per-sensor fail flags (`IC_APP_BMP280_FAIL_BIT`, `IC_APP_VL53L0X_FAIL_BIT`) in `g_app_error_flags`.
  - Sensor tasks now abort gracefully when corresponding fail flag is set.

- Improved I2C safety and production behavior:
  - Guarded sensor init and runtime sensor reads with `i2cMutexHandle`.
  - Wrapped I2C scan behind `IC_DEBUG_I2C_SCAN` to remove unnecessary boot delay in production.

- Improved app task configurability:
  - Added `ic_led_task_cfg_t`, `ic_tof_task_cfg_t`, `ic_temp_task_cfg_t` argument-driven configuration.
  - Replaced hardcoded loop/interval values with compile-time tunables and task config defaults.

- Updated documentation to match current app behavior and APIs:
  - readme.md
================================================================================
RELEASE 2026-02-26
- Normalized comments, spacing, and API descriptions for Core/app modules:
  - Core/app/Inc/ic_app.h
  - Core/app/Inc/ic_bmp280.h
  - Core/app/Inc/ic_led.h
  - Core/app/Inc/ic_logger.h
  - Core/app/Inc/ic_status.h
  - Core/app/Inc/ic_vl53l0x.h
  - Core/app/Src/ic_app.c
  - Core/app/Src/ic_bmp280.c
  - Core/app/Src/ic_led.c
  - Core/app/Src/ic_logger_uart.c
  - Core/app/Src/ic_status.c
  - Core/app/Src/ic_vl53l0x.c

- Updated documentation to match the unified ic_status API and module list:
  - readme.md

================================================================================
RELEASE 2026-02-25
- Refactor: aligned Core/app headers and sources to embedded-style formatting.

- Updated RTOS application flow and logging integration:
  - Core/Src/main.c
  - Core/Src/freertos.c
  - Core/app/Src/ic_app.c
  - Core/app/Src/ic_logger_uart.c

- Updated project documentation to reflect RTOS tasks, logger queue, and API changes:
  - readme.md

================================================================================
RELEASE 2026-02-24
- Refactor: added RTOS middle layer.

- Updated RTOS integration and application flow:
  - Core/Src/main.c
  - Core/Src/freertos.c
  - Core/app/Src/ic_app.c

- Documented RTOS task model (LED/TOF/Temp), I2C mutex usage, and task intervals:
  - readme.md

================================================================================
RELEASE 2026-02-23
- Updated source file documentation style and text formatting for:
  - Core/app/Src/ic_app.c
  - Core/app/Src/ic_bmp280.c
  - Core/app/Src/ic_led.c
  - Core/app/Src/ic_logger_uart.c
  - Core/app/Src/ic_vl53l0x.c

- Updated header file API comments and descriptions for:
  - Core/app/Inc/ic_app.h
  - Core/app/Inc/ic_bmp280.h
  - Core/app/Inc/ic_led.h
  - Core/app/Inc/ic_logger.h
  - Core/app/Inc/ic_vl53l0x.h

================================================================================
RELEASE 2026-02-22
- Added English summary updates.

- Removed legacy CubeIDE/meta/temp files from repository.

- Added HAL and CMSIS drivers.

- Added .gitignore rules for STM32 project outputs and temporary files.

- Added project README content.

- Refactored and unified ic_* API naming and driver layout.

- Reforged project naming/identity details.

================================================================================
RELEASE 2026-02-19
- Added TOF initialization flow.

- Improved TOF behavior to stable working state.

================================================================================
RELEASE 2026-02-18
- Improved dual-sensor integration from partial operation to working state.

- Updated mid-debug version for temporary temperature-read flow verification.

- Added BMP280 initialization flow.

================================================================================
RELEASE 2026-02-17
- Fixed python and flash script issues.

================================================================================
RELEASE 2026-02-11
- Modularized application components (logger/LED) and fixed build script.

- Initial baseline CubeIDE project import.

