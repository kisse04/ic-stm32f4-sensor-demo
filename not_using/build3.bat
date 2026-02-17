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

REM Ensure output folder exists early
if not exist "%OUT%" mkdir "%OUT%"

REM =========================================
REM  Log file (collect everything)
REM =========================================
set "LOG=%OUT%\log.txt"
> "%LOG%" echo ===== Build Log - %DATE% %TIME% =====
>>"%LOG%" echo PROJECT=%PROJECT%
>>"%LOG%" echo OUT=%OUT%
>>"%LOG%" echo GCC_BIN=%GCC_BIN%
>>"%LOG%" echo PATH=%PATH%
>>"%LOG%" echo.

REM Check compiler exist
where arm-none-eabi-gcc >>"%LOG%" 2>&1
if errorlevel 1 (
  echo [ERROR] arm-none-eabi-gcc not found in PATH
  echo [ERROR] arm-none-eabi-gcc not found in PATH >>"%LOG%"
  exit /b 1
)

echo ========================================
echo Building %PROJECT% using GCC...
echo Log: %LOG%
echo ========================================

>>"%LOG%" echo ========================================
>>"%LOG%" echo Building %PROJECT% using GCC...
>>"%LOG%" echo ========================================

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
>>"%LOG%" echo [1/3] Compiling Core/Src...

for %%f in (Core\Src\*.c) do (
  echo Compiling %%f
  >>"%LOG%" echo.
  >>"%LOG%" echo ---- Compiling %%f ----
  arm-none-eabi-gcc %CFLAGS% %INC% -c "%%f" -o "%OUT%\%%~nf.o" >>"%LOG%" 2>&1
  if errorlevel 1 (
    echo [ERROR] Compile failed: %%f
    >>"%LOG%" echo [ERROR] Compile failed: %%f
    exit /b 1
  )
)

REM =========================================
REM  Compile HAL drivers
REM =========================================
echo [2/3] Compiling HAL Drivers...
>>"%LOG%" echo [2/3] Compiling HAL Drivers...

for %%f in (Drivers\STM32F4xx_HAL_Driver\Src\*.c) do (
  echo Compiling %%f
  >>"%LOG%" echo.
  >>"%LOG%" echo ---- Compiling %%f ----
  arm-none-eabi-gcc %CFLAGS% %INC% -c "%%f" -o "%OUT%\%%~nf.o" >>"%LOG%" 2>&1
  if errorlevel 1 (
    echo [ERROR] Compile failed: %%f
    >>"%LOG%" echo [ERROR] Compile failed: %%f
    exit /b 1
  )
)

REM =========================================
REM  Link
REM =========================================
echo [3/3] Linking...
>>"%LOG%" echo.
>>"%LOG%" echo [3/3] Linking...
>>"%LOG%" echo ---- Link command ----
>>"%LOG%" echo arm-none-eabi-gcc %CFLAGS% %OUT%\*.o %LDFLAGS% -o %OUT%\%PROJECT%.elf

arm-none-eabi-gcc %CFLAGS% "%OUT%\*.o" %LDFLAGS% -o "%OUT%\%PROJECT%.elf" >>"%LOG%" 2>&1
if errorlevel 1 (
  echo [ERROR] Linking failed
  >>"%LOG%" echo [ERROR] Linking failed
  exit /b 1
)

REM =========================================
REM Create hex & bin
REM =========================================
>>"%LOG%" echo.
>>"%LOG%" echo Creating HEX/BIN...
arm-none-eabi-objcopy -O ihex "%OUT%\%PROJECT%.elf" "%OUT%\%PROJECT%.hex" >>"%LOG%" 2>&1
if errorlevel 1 (
  echo [ERROR] objcopy (hex) failed
  >>"%LOG%" echo [ERROR] objcopy (hex) failed
  exit /b 1
)

arm-none-eabi-objcopy -O binary "%OUT%\%PROJECT%.elf" "%OUT%\%PROJECT%.bin" >>"%LOG%" 2>&1
if errorlevel 1 (
  echo [ERROR] objcopy (bin) failed
  >>"%LOG%" echo [ERROR] objcopy (bin) failed
  exit /b 1
)

REM Program size
>>"%LOG%" echo.
>>"%LOG%" echo Program size:
arm-none-eabi-size "%OUT%\%PROJECT%.elf" >>"%LOG%" 2>&1

echo ========================================
echo Build DONE! ELF/HEX/BIN created in %OUT%
echo Log saved to %LOG%
echo ========================================

>>"%LOG%" echo.
>>"%LOG%" echo ========================================
>>"%LOG%" echo Build DONE! ELF/HEX/BIN created in %OUT%
>>"%LOG%" echo ========================================
exit /b 0
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

set CFLAGS= -mcpu=cortex-m4 -mthumb -O0 -g -Wall -ffunction-sections -fdata-sections ^
  -DSTM32F446xx -DUSE_HAL_DRIVER



REM Ensure output folder exists early
if not exist "%OUT%" mkdir "%OUT%"

REM =========================================
REM  Log file (collect everything)
REM =========================================
set "LOG=%OUT%\log.txt"
> "%LOG%" echo ===== Build Log - %DATE% %TIME% =====
>>"%LOG%" echo PROJECT=%PROJECT%
>>"%LOG%" echo OUT=%OUT%
>>"%LOG%" echo GCC_BIN=%GCC_BIN%
>>"%LOG%" echo PATH=%PATH%
>>"%LOG%" echo.

REM Check compiler exist
where arm-none-eabi-gcc >>"%LOG%" 2>&1
if errorlevel 1 (
  echo [ERROR] arm-none-eabi-gcc not found in PATH
  echo [ERROR] arm-none-eabi-gcc not found in PATH >>"%LOG%"
  exit /b 1
)

echo ========================================
echo Building %PROJECT% using GCC...
echo Log: %LOG%
echo ========================================

>>"%LOG%" echo ========================================
>>"%LOG%" echo Building %PROJECT% using GCC...
>>"%LOG%" echo ========================================

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
>>"%LOG%" echo [1/3] Compiling Core/Src...

for %%f in (Core\Src\*.c) do (
  echo Compiling %%f
  >>"%LOG%" echo.
  >>"%LOG%" echo ---- Compiling %%f ----
  arm-none-eabi-gcc %CFLAGS% %INC% -c "%%f" -o "%OUT%\%%~nf.o" >>"%LOG%" 2>&1
  if errorlevel 1 (
    echo [ERROR] Compile failed: %%f
    >>"%LOG%" echo [ERROR] Compile failed: %%f
    exit /b 1
  )
)

REM =========================================
REM  Compile HAL drivers
REM =========================================
echo [2/3] Compiling HAL Drivers...
>>"%LOG%" echo [2/3] Compiling HAL Drivers...

for %%f in (Drivers\STM32F4xx_HAL_Driver\Src\*.c) do (
  echo Compiling %%f
  >>"%LOG%" echo.
  >>"%LOG%" echo ---- Compiling %%f ----
  arm-none-eabi-gcc %CFLAGS% %INC% -c "%%f" -o "%OUT%\%%~nf.o" >>"%LOG%" 2>&1
  if errorlevel 1 (
    echo [ERROR] Compile failed: %%f
    >>"%LOG%" echo [ERROR] Compile failed: %%f
    exit /b 1
  )
)

REM =========================================
REM  Link
REM =========================================
echo [3/3] Linking...
>>"%LOG%" echo.
>>"%LOG%" echo [3/3] Linking...
>>"%LOG%" echo ---- Link command ----
>>"%LOG%" echo arm-none-eabi-gcc %CFLAGS% %OUT%\*.o %LDFLAGS% -o %OUT%\%PROJECT%.elf

arm-none-eabi-gcc %CFLAGS% "%OUT%\*.o" %LDFLAGS% -o "%OUT%\%PROJECT%.elf" >>"%LOG%" 2>&1
if errorlevel 1 (
  echo [ERROR] Linking failed
  >>"%LOG%" echo [ERROR] Linking failed
  exit /b 1
)

REM =========================================
REM Create hex & bin
REM =========================================
>>"%LOG%" echo.
>>"%LOG%" echo Creating HEX/BIN...
arm-none-eabi-objcopy -O ihex "%OUT%\%PROJECT%.elf" "%OUT%\%PROJECT%.hex" >>"%LOG%" 2>&1
if errorlevel 1 (
  echo [ERROR] objcopy (hex) failed
  >>"%LOG%" echo [ERROR] objcopy (hex) failed
  exit /b 1
)

arm-none-eabi-objcopy -O binary "%OUT%\%PROJECT%.elf" "%OUT%\%PROJECT%.bin" >>"%LOG%" 2>&1
if errorlevel 1 (
  echo [ERROR] objcopy (bin) failed
  >>"%LOG%" echo [ERROR] objcopy (bin) failed
  exit /b 1
)

REM Program size
>>"%LOG%" echo.
>>"%LOG%" echo Program size:
arm-none-eabi-size "%OUT%\%PROJECT%.elf" >>"%LOG%" 2>&1

echo ========================================
echo Build DONE! ELF/HEX/BIN created in %OUT%
echo Log saved to %LOG%
echo ========================================

>>"%LOG%" echo.
>>"%LOG%" echo ========================================
>>"%LOG%" echo Build DONE! ELF/HEX/BIN created in %OUT%
>>"%LOG%" echo ========================================
exit /b 0
