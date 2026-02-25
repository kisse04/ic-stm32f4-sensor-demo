Copyright (c) 2026. All rights reserved.

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

