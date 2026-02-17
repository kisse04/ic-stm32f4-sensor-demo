@echo off
setlocal enabledelayedexpansion

REM =========================================
REM  Project Name & Output Folder
REM =========================================
set PROJECT=nucleo_f446_uart_test01
set OUT=out_gcc

REM =========================================
REM  Add ARM GCC Toolchain to PATH
REM =========================================
set "GCC_BIN=C:\ST\STM32CubeIDE_2.0.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.100.202509120712\tools\bin"
set "PATH=%GCC_BIN%;%PATH%"

REM Check compiler exist
where arm-none-eabi-gcc >nul 2>nul
if errorlevel 1 (
  echo [ERROR] arm-none-eabi-gcc not found in PATH
  exit /b 1
)

echo ========================================
echo Building %PROJECT% using GCC...
echo ========================================

if not exist %OUT% mkdir %OUT%

REM =========================================
REM  Include Paths
REM =========================================
set INC= ^
  -ICore\Inc ^
  -IDrivers\CMSIS\Include ^
  -IDrivers\CMSIS\Device\ST\STM32F4xx\Include ^
  -IDrivers\STM32F4xx_HAL_Driver\Inc

REM =========================================
REM  Compiler flags
REM =========================================
set CFLAGS= -mcpu=cortex-m4 -mthumb -O0 -g -Wall -ffunction-sections -fdata-sections

REM =========================================
REM  Linker script & Flags
REM =========================================
set LDSCRIPT=STM32F446RETX_FLASH.ld
set LDFLAGS= -T %LDSCRIPT% -Wl,--gc-sections -Wl,-Map=%OUT%\%PROJECT%.map

REM =========================================
REM  Compile all .c in Core/Src
REM =========================================
echo [1/3] Compiling Core/Src...
for %%f in (Core\Src\*.c) do (
  echo Compiling %%f
  arm-none-eabi-gcc %CFLAGS% %INC% -c %%f -o %OUT%\%%~nf.o
)

REM =========================================
REM  Compile HAL drivers
REM =========================================
echo [2/3] Compiling HAL Drivers...
for %%f in (Drivers\STM32F4xx_HAL_Driver\Src\*.c) do (
  echo Compiling %%f
  arm-none-eabi-gcc %CFLAGS% %INC% -c %%f -o %OUT%\%%~nf.o
)

REM =========================================
REM  Link
REM =========================================
echo [3/3] Linking...
arm-none-eabi-gcc %CFLAGS% %OUT%\*.o %LDFLAGS% -o %OUT%\%PROJECT%.elf

if errorlevel 1 (
  echo [ERROR] Linking failed
  exit /b 1
)

REM =========================================
REM Create hex & bin
REM =========================================
arm-none-eabi-objcopy -O ihex %OUT%\%PROJECT%.elf %OUT%\%PROJECT%.hex
arm-none-eabi-objcopy -O binary %OUT%\%PROJECT%.elf %OUT%\%PROJECT%.bin

REM Program size
arm-none-eabi-size %OUT%\%PROJECT%.elf

echo ========================================
echo Build DONE! ELF/HEX/BIN created in %OUT%
echo ========================================
