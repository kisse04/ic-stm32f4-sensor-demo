@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

REM =========================
REM User Config
REM =========================
set "PROJECT=nucleo_f446_uart_test01"
set "OUT=out_gcc"
set "LOG=log.txt"
set "STARTUP_S=Core\Startup\startup_stm32f446retx.s"
set "LDSCRIPT=STM32F446RETX_FLASH.ld"
set "OBJCOPY_TIMEOUT_SEC=20"

REM ARM GCC Toolchain bin (CubeIDE bundle)
set "GCC_BIN=C:\ST\STM32CubeIDE_2.0.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.100.202509120712\tools\bin"
set "PATH=%GCC_BIN%;%PATH%"

REM =========================
REM Init Log
REM =========================
del /q "%LOG%" 2>nul
call :log "===== Build Log - %DATE% %TIME% ====="
call :log "PROJECT=%PROJECT%"
call :log "OUT=%OUT%"
call :log "GCC_BIN=%GCC_BIN%"
call :log "PATH=%PATH%"
call :log ""

echo ========================================
echo Building %PROJECT% using GCC...
echo ========================================
call :log "========================================"
call :log "Building %PROJECT% using GCC..."
call :log "========================================"

if not exist "%OUT%" mkdir "%OUT%"

REM =========================
REM Tool check
REM =========================
where arm-none-eabi-gcc >nul 2>nul || call :die "[ERROR] arm-none-eabi-gcc not found in PATH"

REM =========================
REM Include / Flags
REM =========================
set "INC= -ICore\Inc -IDrivers\CMSIS\Include -IDrivers\CMSIS\Device\ST\STM32F4xx\Include -IDrivers\STM32F4xx_HAL_Driver\Inc"
set "CFLAGS= -mcpu=cortex-m4 -mthumb -O0 -g -Wall -ffunction-sections -fdata-sections -DSTM32F446xx -DUSE_HAL_DRIVER"
set "LDFLAGS= -T %LDSCRIPT% -Wl,--gc-sections -Wl,-Map=%OUT%\%PROJECT%.map -Wl,-e,Reset_Handler"

REM =========================
REM [1/4] Compile Core
REM =========================
call :step "[1/4] Compiling Core/Src..."
for %%f in (Core\Src\*.c) do (
  call :log ""
  call :log "---- Compiling %%f ----"
  echo   Compiling %%f
  call :run arm-none-eabi-gcc %CFLAGS% %INC% -c "%%f" -o "%OUT%\%%~nf.o" || call :die "[ERROR] Compile failed: %%f"
)

REM =========================
REM [2/4] Compile HAL
REM =========================
call :step "[2/4] Compiling HAL Drivers..."
for %%f in (Drivers\STM32F4xx_HAL_Driver\Src\*.c) do (
  call :log ""
  call :log "---- Compiling %%f ----"
  echo   Compiling %%f
  call :run arm-none-eabi-gcc %CFLAGS% %INC% -c "%%f" -o "%OUT%\%%~nf.o" || call :die "[ERROR] Compile failed: %%f"
)

REM =========================
REM [3/4] Compile Startup
REM =========================
call :step "[3/4] Compiling Startup..."
if not exist "%STARTUP_S%" call :die "[ERROR] Startup file not found: %STARTUP_S%"

call :log ""
call :log "---- Compiling %STARTUP_S% ----"
call :run arm-none-eabi-gcc %CFLAGS% -x assembler-with-cpp -c "%STARTUP_S%" -o "%OUT%\startup_stm32f446retx.o" || call :die "[ERROR] Startup compile failed"

REM =========================
REM [4/4] Link via response file
REM =========================
call :step "[4/4] Linking..."
set "RSP=%OUT%\objects.rsp"
del /q "%RSP%" 2>nul

REM generate absolute paths; replace \ with / for ld response file
for /f "delims=" %%p in ('dir /b /s /a:-d "%OUT%\*.o" 2^>nul') do (
  set "P=%%p"
  set "P=!P:\=/!"
  echo !P!>>"%RSP%"
)

if not exist "%RSP%" call :die "[ERROR] Response file not created: %RSP%"
for %%A in ("%RSP%") do set "RSPSIZE=%%~zA"
if "%RSPSIZE%"=="0" call :die "[ERROR] Response file is empty: %RSP%"

call :log ""
call :log "---- objects.rsp ----"
type "%RSP%" >>"%LOG%"
call :log "---------------------"
call :log "---- Link command ----"
call :log "arm-none-eabi-gcc %CFLAGS% @""%RSP%"" %LDFLAGS% -o ""%OUT%\%PROJECT%.elf"""

call :run arm-none-eabi-gcc %CFLAGS% @"%RSP%" %LDFLAGS% -o "%OUT%\%PROJECT%.elf" || call :die "[ERROR] Linking failed"
echo [OK] Linking finished.
call :log "[OK] Linking finished."

REM =========================
REM Post: check + readelf
REM =========================
call :step "[5/6] Checking ELF..."
if not exist "%OUT%\%PROJECT%.elf" call :die "[ERROR] ELF not found: %OUT%\%PROJECT%.elf"

where arm-none-eabi-readelf >nul 2>nul
if not errorlevel 1 (
  call :log "---- readelf -h ----"
  call :run arm-none-eabi-readelf -h "%OUT%\%PROJECT%.elf"
)

REM =========================
REM Objcopy (watchdog)
REM =========================
call :step "[6/6] Objcopy HEX..."
del /q "%OUT%\%PROJECT%.hex" 2>nul
del /q "%OUT%\%PROJECT%.bin" 2>nul

set "OBJCOPY=%GCC_BIN%\arm-none-eabi-objcopy.exe"
call :objcopy_watch ihex "%OUT%\%PROJECT%.elf" "%OUT%\%PROJECT%.hex" "%OBJCOPY_TIMEOUT_SEC%" || call :die "[ERROR] objcopy (hex) failed/time out"
echo [OK] HEX created.
call :log "[OK] HEX created."

call :step "[6/6] Objcopy BIN..."
call :objcopy_watch binary "%OUT%\%PROJECT%.elf" "%OUT%\%PROJECT%.bin" "%OBJCOPY_TIMEOUT_SEC%" || call :die "[ERROR] objcopy (bin) failed/time out"
echo [OK] BIN created.
call :log "[OK] BIN created."

REM =========================
REM Size
REM =========================
call :step "[6/6] Size..."
call :run arm-none-eabi-size "%OUT%\%PROJECT%.elf" || call :die "[ERROR] size failed"

echo ========================================
echo Build DONE! ELF/HEX/BIN created in %OUT%
echo Log saved to %LOG%
echo ========================================
call :log "========================================"
call :log "Build DONE! ELF/HEX/BIN created in %OUT%"
call :log "Log saved to %LOG%"
call :log "========================================"

exit /b 0


REM ============================================================
REM Functions
REM ============================================================

:log
>>"%LOG%" echo %~1
exit /b 0

:step
echo %~1
call :log %~1
exit /b 0

:die
echo %~1
call :log %~1
exit /b 1

:run
REM run command, log stdout/stderr, return errorlevel
%* >>"%LOG%" 2>&1
exit /b %errorlevel%

:objcopy_watch
REM args: <fmt> <in_elf> <out_file> <timeout_sec>
set "FMT=%~1"
set "IN=%~2"
set "OUTF=%~3"
set "T=%~4"

REM start in background, redirect to log
start "objcopy_%FMT%" /b "%OBJCOPY%" -S -O %FMT% "%IN%" "%OUTF%" >>"%LOG%" 2>&1

for /l %%i in (1,1,%T%) do (
  if exist "%OUTF%" exit /b 0
  timeout /t 1 /nobreak >nul
)

call :log "[ERROR] objcopy (%FMT%) TIMEOUT (killing arm-none-eabi-objcopy.exe)"
taskkill /f /im arm-none-eabi-objcopy.exe >>"%LOG%" 2>&1
exit /b 124
