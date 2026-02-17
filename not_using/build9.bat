@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

REM ======================================================
REM Project Config
REM ======================================================
set "PROJECT=nucleo_f446_uart_test01"
set "OUT=out_gcc"
set "LOG=log.txt"

REM ======================================================
REM Toolchain
REM ======================================================
set "GCC_BIN=C:\ST\STM32CubeIDE_2.0.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.100.202509120712\tools\bin"
set "PATH=%GCC_BIN%;%PATH%"

REM ======================================================
REM Initialize log
REM ======================================================
del /q "%LOG%" 2>nul

call :log "===== Build Log - %DATE% %TIME% ====="
call :log "PWD=%CD%"
call :log "PROJECT=%PROJECT%"
call :log "OUT=%OUT%"
call :log "GCC_BIN=%GCC_BIN%"

for /f "delims=" %%v in ('arm-none-eabi-gcc --version 2^>nul ^| findstr /i "gcc"') do (
    call :log "GCC_VERSION=%%v"
    goto :_gccver_done
)
:_gccver_done

for /f "delims=" %%v in ('arm-none-eabi-objcopy --version 2^>nul ^| findstr /i "objcopy"') do (
    call :log "OBJCOPY_VERSION=%%v"
    goto :_obj_ver_done
)
:_obj_ver_done
call :log ""

REM ======================================================
REM Check compiler exist
REM ======================================================
where arm-none-eabi-gcc >nul 2>nul
if errorlevel 1 call :die "arm-none-eabi-gcc not found in PATH"

echo ========================================
echo Building %PROJECT% using GCC...
echo ========================================

if not exist "%OUT%" mkdir "%OUT%"

REM ======================================================
REM Include paths / flags
REM ======================================================
set "INC= -ICore\Inc -IDrivers\CMSIS\Include -IDrivers\CMSIS\Device\ST\STM32F4xx\Include -IDrivers\STM32F4xx_HAL_Driver\Inc"
set "CFLAGS= -mcpu=cortex-m4 -mthumb -O0 -g -Wall -ffunction-sections -fdata-sections -DSTM32F446xx -DUSE_HAL_DRIVER"
set "LDSCRIPT=STM32F446RETX_FLASH.ld"
set "LDFLAGS= -T %LDSCRIPT% -Wl,--gc-sections -Wl,-Map=%OUT%\%PROJECT%.map -Wl,-e,Reset_Handler"

REM ======================================================
REM [1/7] Compile Core/Src
REM ======================================================
echo [1/7] Compiling Core/Src...
call :log "[1/7] Compiling Core/Src..."

for %%f in (Core\Src\*.c) do (
    echo   Compiling %%f
    call :log "---- Compiling %%f ----"
    call :run arm-none-eabi-gcc %CFLAGS% %INC% -c "%%f" -o "%OUT%\%%~nf.o" || call :die "Compile failed: %%f"
)

REM ======================================================
REM [2/7] Compile HAL Drivers
REM ======================================================
echo [2/7] Compiling HAL Drivers...
call :log "[2/7] Compiling HAL Drivers..."

for %%f in (Drivers\STM32F4xx_HAL_Driver\Src\*.c) do (
    echo   Compiling %%f
    call :log "---- Compiling %%f ----"
    call :run arm-none-eabi-gcc %CFLAGS% %INC% -c "%%f" -o "%OUT%\%%~nf.o" || call :die "Compile failed: %%f"
)

REM ======================================================
REM [3/7] Compile Startup
REM ======================================================
echo [3/7] Compiling Startup...
call :log "[3/7] Compiling Startup..."

set "STARTUP_S=Core\Startup\startup_stm32f446retx.s"
set "STARTUP_O=%OUT%\startup_stm32f446retx.o"

if not exist "%STARTUP_S%" call :die "Startup file missing: %STARTUP_S%"

call :log "---- Compiling startup ----"
call :run arm-none-eabi-gcc %CFLAGS% -x assembler-with-cpp -c "%STARTUP_S%" -o "%STARTUP_O%" || call :die "Startup compile failed"

REM ======================================================
REM [4/7] Link (RSP)
REM ======================================================
echo [4/7] Linking...
call :log "[4/7] Linking..."

set "RSP=%OUT%\objects.rsp"
del /q "%RSP%" 2>nul

for /f "delims=" %%p in ('dir /b /s /a:-d "%OUT%\*.o"') do (
    set "P=%%p"
    set "P=!P:\=/!"
    echo !P!>>"%RSP%"
)

for %%A in ("%RSP%") do set "RSPSIZE=%%~zA"
if "%RSPSIZE%"=="0" call :die "Rsp file empty"

call :log "RSP=%RSP% (size=%RSPSIZE% bytes)"
for /f %%n in ('find /v /c "" ^< "%RSP%"') do set "OBJCOUNT=%%n"
call :log "OBJ_COUNT=%OBJCOUNT%"
call :log ""

call :log "---- Link command ----"
call :log "arm-none-eabi-gcc %CFLAGS% @%RSP% %LDFLAGS% -o %OUT%\%PROJECT%.elf"

call :run arm-none-eabi-gcc %CFLAGS% @"%RSP%" %LDFLAGS% -o "%OUT%\%PROJECT%.elf" || call :die "Linking failed"

echo [OK] Linking finished.
call :log "[OK] Linking finished."

REM ======================================================
REM [5/7] Check ELF (summary only)
REM ======================================================
echo [5/7] Checking ELF...
call :log "[5/7] Checking ELF..."

if not exist "%OUT%\%PROJECT%.elf" call :die "ELF missing: %OUT%\%PROJECT%.elf"

where arm-none-eabi-readelf >nul 2>nul
if errorlevel 1 (
    call :log "[WARN] arm-none-eabi-readelf not found; skip ELF summary"
) else (
    REM log summary
    call :log "ELF Summary:"
    for /f "delims=" %%l in ('arm-none-eabi-readelf -h "%OUT%\%PROJECT%.elf" 2^>nul ^| findstr /i "Entry point address"') do call :log "%%l"
    for /f "delims=" %%l in ('arm-none-eabi-readelf -h "%OUT%\%PROJECT%.elf" 2^>nul ^| findstr /i "Number of section headers"') do call :log "%%l"
    call :log ""
)

REM ======================================================
REM [6/7] Objcopy HEX
REM ======================================================
echo [6/7] Objcopy HEX...
call :log "[6/7] Objcopy HEX..."

call :run arm-none-eabi-objcopy -O ihex "%OUT%\%PROJECT%.elf" "%OUT%\%PROJECT%.hex" || call :die "Objcopy HEX failed"
echo [OK] HEX created.
call :log "[OK] HEX created."

REM ======================================================
REM [7/7] Objcopy BIN + Size
REM ======================================================
echo [7/7] Objcopy BIN...
call :log "[7/7] Objcopy BIN..."

call :run arm-none-eabi-objcopy -O binary "%OUT%\%PROJECT%.elf" "%OUT%\%PROJECT%.bin" || call :die "Objcopy BIN failed"
echo [OK] BIN created.
call :log "[OK] BIN created."

echo [7/7] Size...
call :log "[7/7] Size..."
call :run arm-none-eabi-size "%OUT%\%PROJECT%.elf" || call :die "Size failed"

REM ======================================================
REM Final
REM ======================================================
echo ========================================
echo Build DONE ELF/HEX/BIN created in %OUT%
echo Log saved to %LOG%
echo ========================================

call :log "ARTIFACTS:"
call :log "  %OUT%\%PROJECT%.elf"
call :log "  %OUT%\%PROJECT%.hex"
call :log "  %OUT%\%PROJECT%.bin"
call :log "  %OUT%\%PROJECT%.map"
call :log "===== DONE ====="
exit /b 0

REM ======================================================
REM Functions
REM ======================================================
:run
%* >>"%LOG%" 2>&1
exit /b %errorlevel%

:log
echo %~1>>"%LOG%"
exit /b 0

:die
echo [ERROR] %~1
call :log "[ERROR] %~1"
exit /b 1
