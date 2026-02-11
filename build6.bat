@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

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
REM  Compiler flags (select MCU + HAL)
REM =========================================
set "CFLAGS= -mcpu=cortex-m4 -mthumb -O0 -g -Wall -ffunction-sections -fdata-sections -DSTM32F446xx -DUSE_HAL_DRIVER"

REM =========================================
REM  Linker script & Flags
REM  + 指定 entry = Reset_Handler（保險）
REM =========================================
set "LDSCRIPT=STM32F446RETX_FLASH.ld"
set "LDFLAGS= -T %LDSCRIPT% -Wl,--gc-sections -Wl,-Map=%OUT%\%PROJECT%.map -Wl,-e,Reset_Handler"

REM =========================================
REM  Compile all .c in Core/Src
REM =========================================
echo [1/4] Compiling Core/Src...
echo [1/4] Compiling Core/Src...>>"%LOG%"

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
echo [2/4] Compiling HAL Drivers...
echo [2/4] Compiling HAL Drivers...>>"%LOG%"

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
REM  Compile Startup (重要：提供 Reset_Handler + vector table)
REM =========================================
echo [3/4] Compiling Startup...
echo [3/4] Compiling Startup...>>"%LOG%"

set "STARTUP_S=Core\Startup\startup_stm32f446retx.s"
set "STARTUP_O=%OUT%\startup_stm32f446retx.o"

if not exist "%STARTUP_S%" (
  echo [ERROR] Startup file not found: %STARTUP_S%
  echo [ERROR] Startup file not found: %STARTUP_S%>>"%LOG%"
  exit /b 1
)

echo ---- Compiling %STARTUP_S% ---->>"%LOG%"
arm-none-eabi-gcc %CFLAGS% -x assembler-with-cpp -c "%STARTUP_S%" -o "%STARTUP_O%" >>"%LOG%" 2>&1
if errorlevel 1 (
  echo [ERROR] Startup compile failed
  echo [ERROR] Startup compile failed>>"%LOG%"
  exit /b 1
)

REM =========================================
REM  Link (response file, use forward slashes)
REM =========================================
echo [4/4] Linking...
echo [4/4] Linking...>>"%LOG%"

if not exist "%OUT%" mkdir "%OUT%"

set "RSP=%OUT%\objects.rsp"
del /q "%RSP%" 2>nul

REM 產生絕對路徑，並把 \ 轉成 /，避免 ld 在 rsp 裡吃掉反斜線
for /f "delims=" %%p in ('dir /b /s /a:-d "%OUT%\*.o" 2^>nul') do (
  set "P=%%p"
  set "P=!P:\=/!"
  echo !P!>>"%RSP%"
)

for %%A in ("%RSP%") do set "RSPSIZE=%%~zA"
if not exist "%RSP%" (
  echo [ERROR] Response file not created: %RSP%
  echo [ERROR] Response file not created: %RSP%>>"%LOG%"
  exit /b 1
)
if "%RSPSIZE%"=="0" (
  echo [ERROR] Response file is empty: %RSP%
  echo [ERROR] Response file is empty: %RSP%>>"%LOG%"
  exit /b 1
)

echo ---- objects.rsp ---->>"%LOG%"
type "%RSP%" >>"%LOG%"
echo --------------------- >>"%LOG%"

echo ---- Link command ---->>"%LOG%"
echo arm-none-eabi-gcc %CFLAGS% @"%RSP%" %LDFLAGS% -o "%OUT%\%PROJECT%.elf">>"%LOG%"

arm-none-eabi-gcc %CFLAGS% @"%RSP%" %LDFLAGS% -o "%OUT%\%PROJECT%.elf" >>"%LOG%" 2>&1
if errorlevel 1 (
  echo [ERROR] Linking failed
  echo [ERROR] Linking failed>>"%LOG%"
  exit /b 1
)

echo [OK] Linking finished.
echo [OK] Linking finished.>>"%LOG%"


echo [NEXT] Starting objcopy...
echo [NEXT] Starting objcopy...>>"%LOG%"

REM =========================================
REM Create hex & bin
REM =========================================
echo [5/6] Checking ELF...
echo [5/6] Checking ELF...>>"%LOG%"

if not exist "%OUT%\%PROJECT%.elf" (
  echo [ERROR] ELF not found: %OUT%\%PROJECT%.elf
  echo [ERROR] ELF not found: %OUT%\%PROJECT%.elf>>"%LOG%"
  exit /b 1
)

REM 用 readelf 檢查 ELF 格式，這一步很快且能驗證 objcopy 應該能吃
where arm-none-eabi-readelf >nul 2>nul
if not errorlevel 1 (
  echo ---- readelf -h ---->>"%LOG%"
  call arm-none-eabi-readelf -h "%OUT%\%PROJECT%.elf" >>"%LOG%" 2>&1
)

echo [6/6] Objcopy HEX...
echo [6/6] Objcopy HEX...>>"%LOG%"

REM 清掉舊檔
del /q "%OUT%\%PROJECT%.hex" 2>nul
del /q "%OUT%\%PROJECT%.bin" 2>nul

REM 用絕對路徑執行 objcopy（避免任何 PATH/工作目錄問題）
set "OBJCOPY=%GCC_BIN%\arm-none-eabi-objcopy.exe"

REM 產生 HEX：用 start /b 讓它在背景跑，然後做 watchdog
start "objcopy_hex" /b "%OBJCOPY%" -S -O ihex "%OUT%\%PROJECT%.elf" "%OUT%\%PROJECT%.hex" >>"%LOG%" 2>&1
set "RC=999"
for /l %%i in (1,1,20) do (
  if exist "%OUT%\%PROJECT%.hex" (
    set "RC=0"
    goto :hex_done
  )
  timeout /t 1 /nobreak >nul
)

REM 超過 20 秒還沒生成檔，強制殺掉 objcopy
echo [ERROR] objcopy (hex) TIMEOUT (killing objcopy)
echo [ERROR] objcopy (hex) TIMEOUT (killing objcopy)>>"%LOG%"
taskkill /f /im arm-none-eabi-objcopy.exe >>"%LOG%" 2>&1
exit /b 1

:hex_done
echo [OK] HEX created.
echo [OK] HEX created.>>"%LOG%"

echo [6/6] Objcopy BIN...
echo [6/6] Objcopy BIN...>>"%LOG%"

start "objcopy_bin" /b "%OBJCOPY%" -S -O binary "%OUT%\%PROJECT%.elf" "%OUT%\%PROJECT%.bin" >>"%LOG%" 2>&1
set "RC=999"
for /l %%i in (1,1,20) do (
  if exist "%OUT%\%PROJECT%.bin" (
    set "RC=0"
    goto :bin_done
  )
  timeout /t 1 /nobreak >nul
)

echo [ERROR] objcopy (bin) TIMEOUT (killing objcopy)
echo [ERROR] objcopy (bin) TIMEOUT (killing objcopy)>>"%LOG%"
taskkill /f /im arm-none-eabi-objcopy.exe >>"%LOG%" 2>&1
exit /b 1

:bin_done
echo [OK] BIN created.
echo [OK] BIN created.>>"%LOG%"

echo [6/6] Size...
echo [6/6] Size...>>"%LOG%"

if not exist "%OUT%\%PROJECT%.elf" (
  echo [ERROR] ELF not found for size: %OUT%\%PROJECT%.elf
  echo [ERROR] ELF not found for size: %OUT%\%PROJECT%.elf>>"%LOG%"
  exit /b 1
)

call arm-none-eabi-size "%OUT%\%PROJECT%.elf" >>"%LOG%" 2>&1
if errorlevel 1 (
  echo [ERROR] size failed
  echo [ERROR] size failed>>"%LOG%"
  exit /b 1
)

