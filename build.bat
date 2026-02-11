@echo off
setlocal enabledelayedexpansion

set PROJECT=nucleo_f446_uart_test01
set OUT=out_gcc

set PATH=%PATH%;"C:\ST\STM32CubeIDE_2.0.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.100.202509120712\tools\bin"

if not exist %OUT% mkdir %OUT%

REM ---- Toolchain in PATH ----
where arm-none-eabi-gcc >nul 2>nul
if errorlevel 1 (
  echo arm-none-eabi-gcc not found in PATH
  exit /b 1
)

REM ---- Common flags ----
set CFLAGS=-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
set CFLAGS=%CFLAGS% -O0 -g3 -ffunction-sections -fdata-sections
set CFLAGS=%CFLAGS% -Wall
set CFLAGS=%CFLAGS% -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx

set INCS=-ICore\Inc ^
 -IDrivers\CMSIS\Include ^
 -IDrivers\CMSIS\Device\ST\STM32F4xx\Include ^
 -IDrivers\STM32F4xx_HAL_Driver\Inc

REM ---- Compile your app/core files ----
arm-none-eabi-gcc %CFLAGS% %INCS% -c Core\Src\main.c -o %OUT%\main.o
arm-none-eabi-gcc %CFLAGS% %INCS% -c Core\Src\gpio.c -o %OUT%\gpio.o
arm-none-eabi-gcc %CFLAGS% %INCS% -c Core\Src\usart.c -o %OUT%\usart.o
arm-none-eabi-gcc %CFLAGS% %INCS% -c Core\Src\stm32f4xx_it.c -o %OUT%\stm32f4xx_it.o
arm-none-eabi-gcc %CFLAGS% %INCS% -c Core\Src\stm32f4xx_hal_msp.c -o %OUT%\stm32f4xx_hal_msp.o
arm-none-eabi-gcc %CFLAGS% %INCS% -c Core\Src\system_stm32f4xx.c -o %OUT%\system_stm32f4xx.o
arm-none-eabi-gcc %CFLAGS% %INCS% -c Core\Src\syscalls.c -o %OUT%\syscalls.o
arm-none-eabi-gcc %CFLAGS% %INCS% -c Core\Src\sysmem.c -o %OUT%\sysmem.o

REM ---- Compile HAL modules you actually need ----
arm-none-eabi-gcc %CFLAGS% %INCS% -c Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal.c -o %OUT%\hal.o
arm-none-eabi-gcc %CFLAGS% %INCS% -c Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_rcc.c -o %OUT%\hal_rcc.o
arm-none-eabi-gcc %CFLAGS% %INCS% -c Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_rcc_ex.c -o %OUT%\hal_rcc_ex.o
arm-none-eabi-gcc %CFLAGS% %INCS% -c Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_gpio.c -o %OUT%\hal_gpio.o
arm-none-eabi-gcc %CFLAGS% %INCS% -c Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_uart.c -o %OUT%\hal_uart.o
arm-none-eabi-gcc %CFLAGS% %INCS% -c Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_cortex.c -o %OUT%\hal_cortex.o
arm-none-eabi-gcc %CFLAGS% %INCS% -c Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_pwr.c -o %OUT%\hal_pwr.o
arm-none-eabi-gcc %CFLAGS% %INCS% -c Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_pwr_ex.c -o %OUT%\hal_pwr_ex.o
arm-none-eabi-gcc %CFLAGS% %INCS% -c Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_flash.c -o %OUT%\hal_flash.o
arm-none-eabi-gcc %CFLAGS% %INCS% -c Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_flash_ex.c -o %OUT%\hal_flash_ex.o

REM ---- Startup assembly ----
arm-none-eabi-gcc %CFLAGS% -c Core\Startup\startup_stm32f446retx.s -o %OUT%\startup.o

REM ---- Link ----
arm-none-eabi-gcc %CFLAGS% -TSTM32F446RETX_FLASH.ld -Wl,--gc-sections -Wl,-Map=%OUT%\%PROJECT%.map ^
  %OUT%\startup.o ^
  %OUT%\main.o %OUT%\gpio.o %OUT%\usart.o %OUT%\stm32f4xx_it.o %OUT%\stm32f4xx_hal_msp.o ^
  %OUT%\system_stm32f4xx.o %OUT%\syscalls.o %OUT%\sysmem.o ^
  %OUT%\hal.o %OUT%\hal_rcc.o %OUT%\hal_rcc_ex.o %OUT%\hal_gpio.o %OUT%\hal_uart.o %OUT%\hal_cortex.o ^
  %OUT%\hal_pwr.o %OUT%\hal_pwr_ex.o %OUT%\hal_flash.o %OUT%\hal_flash_ex.o ^
  -o %OUT%\%PROJECT%.elf

REM ---- Create HEX/BIN ----
arm-none-eabi-objcopy -O ihex %OUT%\%PROJECT%.elf %OUT%\%PROJECT%.hex
arm-none-eabi-objcopy -O binary %OUT%\%PROJECT%.elf %OUT%\%PROJECT%.bin
arm-none-eabi-size %OUT%\%PROJECT%.elf

echo Done: %OUT%\%PROJECT%.elf / .hex / .bin
