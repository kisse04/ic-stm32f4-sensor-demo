@echo off
setlocal

REM =========================================
REM  Project Name & Output Folder
REM =========================================
set "PROJECT=nucleo_f446_uart_test01"
set "OUT=out_gcc"
set "LOG=log.txt"

REM =========================================
REM  Add ARM GCC Toolchain to PATH
REM =========================================
set "GCC_BIN=C:\ST\STM32CubeIDE_2.0.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.100.202509120712\tools\bin"
set "PATH=%GCC_BIN%;%PATH%"

REM =========================================
REM  Init log
REM =========================================
del /q "%LOG%" 2>nul
echo ===== Build Log - %DATE% %TIME% =====>>"%LOG%"
echo PROJECT=%PROJECT%>>"%LOG%"
echo OUT=%OUT%>>"%LOG%"
echo GCC_BIN=%GCC_BIN%>>"%LOG%"
echo PATH=%PATH%>>"%LOG%"
echo.>>"%LOG%"

REM Check compiler exist
where arm-none-eabi-gcc >nul 2>nul
if errorlevel 1 (
  echo [ERROR] arm-none-eabi-gcc not found in PATH
  echo [ERROR] arm-none-eabi-gcc not found in PATH>>"%LOG%"
  exit /b 1
)

echo ========================================
echo Building %PROJECT% using GCC...
echo ========================================
echo ========================================>>"%LOG%"
echo Building %PROJECT% using GCC...>>"%LOG%"
echo ========================================>>"%LOG%"

if not exist "%OUT%" mkdir "%OUT%"

REM =========================================
REM  Include Paths
REM =========================================
set "INC= -ICore\Inc -IDrivers\CMSIS\Include -IDrivers\CMSIS\Device\ST\STM32F4xx\Include -IDrivers\STM32F4xx_HAL_Driver\Inc"

REM =========================================
REM  Compiler flags  (關鍵：一定要選到 F446xx + USE_HAL_DRIVER)
REM =========================================
set "CFLAGS= -mcpu=cortex-m4 -mthumb -O0 -g -Wall -ffunction-sections -fdata-sections -DSTM32F446xx -DUSE_HAL_DRIVER"

REM =========================================
REM  Linker script & Flags
REM =========================================
set "LDSCRIPT=STM32F446RETX_FLASH.ld"
set "LDFLAGS= -T %LDSCRIPT% -Wl,--gc-sections -Wl,-Map=%OUT%\%PROJECT%.map"

REM =========================================
REM  Compile all .c in Core/Src
REM =========================================
echo [1/3] Compiling Core/Src...
echo [1/3] Compiling Core/Src...>>"%LOG%"

for %%f in (Core\Src\*.c) do (
  echo   Compiling %%f
  echo.>>"%LOG%"
  echo ---- Compiling %%f ---->>"%LOG%"
  arm-none-eabi-gcc %CFLAGS% %INC% -c "%%f" -o "%OUT%\%%~nf.o" >>"%LOG%" 2>&1
  if errorlevel 1 (
    echo [ERROR] Compile failed: %%f
    echo [ERROR] Compile failed: %%f>>"%LOG%"
    exit /b 1
  )
)

REM =========================================
REM  Compile HAL drivers
REM =========================================
echo [2/3] Compiling HAL Drivers...
echo [2/3] Compiling HAL Drivers...>>"%LOG%"

for %%f in (Drivers\STM32F4xx_HAL_Driver\Src\*.c) do (
  echo   Compiling %%f
  echo.>>"%LOG%"
  echo ---- Compiling %%f ---->>"%LOG%"
  arm-none-eabi-gcc %CFLAGS% %INC% -c "%%f" -o "%OUT%\%%~nf.o" >>"%LOG%" 2>&1
  if errorlevel 1 (
    echo [ERROR] Compile failed: %%f
    echo [ERROR] Compile failed: %%f>>"%LOG%"
    exit /b 1
  )
)

REM =========================================
REM  Link (avoid wildcard expansion issue)
REM =========================================
echo [3/3] Linking...
echo [3/3] Linking...>>"%LOG%"

setlocal enabledelayedexpansion
set "OBJS="

REM 用 dir 列出 out_gcc 裡所有 .o，逐一拼成 linker 參數
for /f "delims=" %%o in ('dir /b /a:-d "%OUT%\*.o" 2^>nul') do (
  set "OBJS=!OBJS! "%OUT%\%%o""
)

if not defined OBJS (
  echo [ERROR] No .o files found in %OUT%
  echo [ERROR] No .o files found in %OUT%>>"%LOG%"
  exit /b 1
)

echo Linking objects: !OBJS!>>"%LOG%"

arm-none-eabi-gcc %CFLAGS% !OBJS! %LDFLAGS% -o "%OUT%\%PROJECT%.elf" >>"%LOG%" 2>&1
if errorlevel 1 (
  echo [ERROR] Linking failed
  echo [ERROR] Linking failed>>"%LOG%"
  exit /b 1
)

REM =========================================
REM Create hex & bin
REM =========================================
arm-none-eabi-objcopy -O ihex "%OUT%\%PROJECT%.elf" "%OUT%\%PROJECT%.hex" >>"%LOG%" 2>&1
arm-none-eabi-objcopy -O binary "%OUT%\%PROJECT%.elf" "%OUT%\%PROJECT%.bin" >>"%LOG%" 2>&1

REM Program size
arm-none-eabi-size "%OUT%\%PROJECT%.elf" >>"%LOG%" 2>&1

echo ========================================
echo Build DONE! ELF/HEX/BIN created in %OUT%
echo Log saved to %LOG%
echo ========================================
echo ========================================>>"%LOG%"
echo Build DONE! ELF/HEX/BIN created in %OUT%>>"%LOG%"
echo ========================================>>"%LOG%"

endlocal
