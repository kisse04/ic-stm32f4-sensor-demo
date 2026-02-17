#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#Ian 260217, guide
'''
0. prepare code, run CMD at project folder (ex. nucleo_f446_uart_test01)
1. Clean build
2. build using this script
3. flash using ST tool

build3.py --clean
build3.py
set PATH=%PATH%;C:\ST\STM32CubeIDE_2.0.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.300.202508131133\tools\bin
STM32_Programmer_CLI.exe -c port=SWD -w out_gcc\nucleo_f446_uart_test01.hex -v -rst


'''

#Putty connect

'''
Putty
Serial line → COM3
Speed	115200
Data bits	8
Stop bits	1
Parity	None
Flow control	None

'''


import argparse
import glob
import os
import shutil
import stat
import subprocess
import time
from datetime import datetime
from pathlib import Path
from typing import Optional


def write_log(log_path: Path, msg: str) -> None:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    with log_path.open("a", encoding="utf-8", errors="replace") as f:
        f.write(msg + "\n")


def die(log_path: Path, msg: str, code: int = 1) -> None:
    print(f"[ERROR] {msg}")
    write_log(log_path, f"[ERROR] {msg}")
    raise SystemExit(code)


def run_cmd(log_path: Path, cmd, env=None, cwd=None) -> int:
    """Run a command, append stdout/stderr to log. No shell."""
    cmd_str = " ".join(f'"{c}"' if " " in str(c) else str(c) for c in cmd)
    write_log(log_path, cmd_str)

    p = subprocess.run(
        cmd,
        cwd=str(cwd) if cwd else None,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    if p.stdout:
        write_log(log_path, p.stdout.rstrip("\n"))
    return p.returncode


def capture_first_matching_line(cmd, contains: str, env, cwd=None) -> Optional[str]:
    """Run cmd, return first line containing substring (case-insensitive)."""
    try:
        p = subprocess.run(
            cmd,
            cwd=str(cwd) if cwd else None,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
    except FileNotFoundError:
        return None

    target = contains.lower()
    for line in (p.stdout or "").splitlines():
        if target in line.lower():
            return line.strip()
    return None


def obj_path(out_dir: Path, src_path: Path) -> Path:
    """
    Put object file under out_dir preserving relative directory structure.
    Example:
      Core/Src/main.c -> out_gcc/Core/Src/main.o
      Core/Startup/startup_xxx.s -> out_gcc/Core/Startup/startup_xxx.o
    """
    rel = src_path.with_suffix(".o")
    obj = out_dir / rel
    obj.parent.mkdir(parents=True, exist_ok=True)
    return obj


def is_up_to_date(src: Path, obj: Path) -> bool:
    """Return True if obj exists and is newer than src."""
    if not obj.exists():
        return False
    try:
        return obj.stat().st_mtime >= src.stat().st_mtime
    except FileNotFoundError:
        return False


def _rm_readonly(func, path, exc_info):
    """
    For Windows: clear readonly bit then retry.
    """
    try:
        os.chmod(path, stat.S_IWRITE)
        func(path)
    except Exception:
        # Let shutil raise the original error if still failing
        raise


def rmtree_retry(path: Path, retries: int = 10, delay_s: float = 0.2) -> None:
    """
    Robust rmtree for Windows where antivirus/IDE may keep handles briefly.
    """
    last_exc = None
    for _ in range(retries):
        try:
            shutil.rmtree(path, onerror=_rm_readonly)
            return
        except PermissionError as e:
            last_exc = e
            time.sleep(delay_s)
    if last_exc:
        raise last_exc


def main() -> int:
    parser = argparse.ArgumentParser(description="GCC build script (ST-makefile compatible)")
    parser.add_argument("--project", default="nucleo_f446_uart_test01")
    parser.add_argument("--out", default="out_gcc")
    parser.add_argument("--log", default="log.txt")
    parser.add_argument(
        "--gcc-bin",
        default=r"C:\ST\STM32CubeIDE_2.0.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.100.202509120712\tools\bin",
        help="Path to arm-none-eabi toolchain bin folder",
    )
    parser.add_argument("--ldscript", default="STM32F446RETX_FLASH.ld")

    # Enhancements
    parser.add_argument("--clean", action="store_true", help="Remove out dir and log then exit")
    parser.add_argument("--rebuild", action="store_true", help="Force rebuild (ignore incremental)")
    parser.add_argument("--no-objdump", action="store_true", help="Skip generating .list by objdump")
    args = parser.parse_args()

    # Match bat: cd /d "%~dp0"
    script_dir = Path(__file__).resolve().parent
    os.chdir(script_dir)

    project = args.project
    out_dir = Path(args.out)
    log_path = Path(args.log)
    gcc_bin = Path(args.gcc_bin)
    ldscript = Path(args.ldscript)

    # Clean mode
    if args.clean:
        if out_dir.exists():
            try:
                rmtree_retry(out_dir)
            except PermissionError as e:
                print(f"[ERROR] Clean failed (folder busy): {out_dir}")
                print(f"        {e}")
                print("        Tip: close STM32CubeIDE, stop debug session, and close any file explorer in that folder.")
                return 1
        try:
            log_path.unlink()
        except FileNotFoundError:
            pass
        print("[OK] Clean done.")
        return 0

    # Initialize log
    try:
        log_path.unlink()
    except FileNotFoundError:
        pass

    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    write_log(log_path, f"===== Build Log - {now} =====")
    write_log(log_path, f"PWD={Path.cwd()}")
    write_log(log_path, f"PROJECT={project}")
    write_log(log_path, f"OUT={out_dir}")
    write_log(log_path, f"GCC_BIN={gcc_bin}")
    write_log(log_path, f"REBUILD={args.rebuild}")

    # Env: set PATH=%GCC_BIN%;%PATH%
    env = os.environ.copy()
    env["PATH"] = str(gcc_bin) + os.pathsep + env.get("PATH", "")

    # Absolute tool paths (avoid PATH lookup issues on Windows)
    gcc_exe = (gcc_bin / "arm-none-eabi-gcc.exe").resolve()
    objcopy_exe = (gcc_bin / "arm-none-eabi-objcopy.exe").resolve()
    readelf_exe = (gcc_bin / "arm-none-eabi-readelf.exe").resolve()
    size_exe = (gcc_bin / "arm-none-eabi-size.exe").resolve()
    objdump_exe = (gcc_bin / "arm-none-eabi-objdump.exe").resolve()

    write_log(log_path, f"gcc_exe={gcc_exe}")
    write_log(log_path, f"objcopy_exe={objcopy_exe}")
    write_log(log_path, f"readelf_exe={readelf_exe}")
    write_log(log_path, f"size_exe={size_exe}")
    write_log(log_path, f"objdump_exe={objdump_exe}")

    if not gcc_exe.exists():
        die(log_path, f"arm-none-eabi-gcc.exe not found: {gcc_exe}")

    gcc_ver = capture_first_matching_line([str(gcc_exe), "--version"], "gcc", env, cwd=Path.cwd())
    if gcc_ver:
        write_log(log_path, f"GCC_VERSION={gcc_ver}")

    write_log(log_path, "")

    print("========================================")
    print(f"Building {project} using GCC...")
    print("========================================")

    out_dir.mkdir(parents=True, exist_ok=True)

    # Include paths (same as your bat)
    inc_flags = [
        r"-ICore\Inc",
        r"-ICore\app\Inc",  # NEW: your app headers
        r"-IDrivers\CMSIS\Include",
        r"-IDrivers\CMSIS\Device\ST\STM32F4xx\Include",
        r"-IDrivers\STM32F4xx_HAL_Driver\Inc",
    ]

    # CFLAGS: align with ST's hard-float ABI (and keep your debug settings)
    cflags = [
        "-mcpu=cortex-m4",
        "-mthumb",
        "-mfpu=fpv4-sp-d16",
        "-mfloat-abi=hard",
        "-O0",
        "-g",
        "-Wall",
        "-ffunction-sections",
        "-fdata-sections",
        "-DSTM32F446xx",
        "-DUSE_HAL_DRIVER",
    ]

    # LDFLAGS: mirror the ST makefile line you pasted
    # NOTE: ST uses absolute -T"...\STM32F446RETX_FLASH.ld"
    # We'll resolve to absolute path too.
    ldscript_abs = ldscript.resolve()
    ldflags = [
        "-mcpu=cortex-m4",
        "-mthumb",
        "-mfpu=fpv4-sp-d16",
        "-mfloat-abi=hard",
        f"-T{str(ldscript_abs)}",
        "--specs=nosys.specs",
        "-Wl,--gc-sections",
        f"-Wl,-Map={out_dir / (project + '.map')}",
        "-static",
        "--specs=nano.specs",
        "-Wl,--start-group",
        "-lc",
        "-lm",
        "-Wl,--end-group",
    ]

    # [1/10] Compile Core/Src
    print("[1/10] Compiling Core/Src...")
    write_log(log_path, "[1/10] Compiling Core/Src...")

    core_sources = sorted(glob.glob(r"Core\Src\*.c"))
    for src in core_sources:
        src_path = Path(src)
        obj = obj_path(out_dir, src_path)

        if not args.rebuild and is_up_to_date(src_path, obj):
            print(f"  [SKIP] {src_path}")
            write_log(log_path, f"[SKIP] {src_path} -> {obj}")
            continue

        print(f"  Compiling {src_path}")
        write_log(log_path, f"---- Compiling {src_path} ----")
        cmd = [str(gcc_exe), *cflags, *inc_flags, "-c", str(src_path), "-o", str(obj)]
        rc = run_cmd(log_path, cmd, env=env)
        if rc != 0:
            die(log_path, f"Compile failed: {src_path}")

    # [2/10] Compile Core/app/Src
    print("[2/10] Compiling Core/app/Src...")
    write_log(log_path, "[2/10] Compiling Core/app/Src...")

    app_sources = sorted(glob.glob(r"Core\app\Src\*.c"))
    for src in app_sources:
        src_path = Path(src)
        obj = obj_path(out_dir, src_path)
        if not args.rebuild and is_up_to_date(src_path, obj):
            print(f"  [SKIP] {src_path}")
            write_log(log_path, f"[SKIP] {src_path} -> {obj}")
            continue

        print(f"  Compiling {src_path}")
        write_log(log_path, f"---- Compiling {src_path} ----")
        cmd = [str(gcc_exe), *cflags, *inc_flags, "-c", str(src_path), "-o", str(obj)]
        rc = run_cmd(log_path, cmd, env=env)
        if rc != 0:
            die(log_path, f"Compile failed: {src_path}")


    # [3/10] Compile HAL Drivers
    print("[3/10] Compiling HAL Drivers...")
    write_log(log_path, "[3/10] Compiling HAL Drivers...")

    hal_sources = sorted(glob.glob(r"Drivers\STM32F4xx_HAL_Driver\Src\*.c"))
    for src in hal_sources:
        src_path = Path(src)
        obj = obj_path(out_dir, src_path)

        if not args.rebuild and is_up_to_date(src_path, obj):
            print(f"  [SKIP] {src_path}")
            write_log(log_path, f"[SKIP] {src_path} -> {obj}")
            continue

        print(f"  Compiling {src_path}")
        write_log(log_path, f"---- Compiling {src_path} ----")
        cmd = [str(gcc_exe), *cflags, *inc_flags, "-c", str(src_path), "-o", str(obj)]
        rc = run_cmd(log_path, cmd, env=env)
        if rc != 0:
            die(log_path, f"Compile failed: {src_path}")

    # [4/10] Compile Startup
    print("[4/10] Compiling Startup...")
    write_log(log_path, "[4/10] Compiling Startup...")

    startup_s = Path(r"Core\Startup\startup_stm32f446retx.s")
    startup_o = obj_path(out_dir, startup_s)

    if not startup_s.exists():
        die(log_path, f"Startup file missing: {startup_s}")

    if not args.rebuild and is_up_to_date(startup_s, startup_o):
        print(f"  [SKIP] {startup_s}")
        write_log(log_path, f"[SKIP] {startup_s} -> {startup_o}")
    else:
        write_log(log_path, "---- Compiling startup ----")
        cmd = [
            str(gcc_exe),
            *cflags,
            "-x",
            "assembler-with-cpp",
            "-c",
            str(startup_s),
            "-o",
            str(startup_o),
        ]
        rc = run_cmd(log_path, cmd, env=env)
        if rc != 0:
            die(log_path, "Startup compile failed")

    # [5/10] Link (objects.list like ST)
    print("[5/10] Linking...")
    write_log(log_path, "[5/10] Linking...")

    obj_list = out_dir / "objects.list"
    if obj_list.exists():
        obj_list.unlink()

    objects = sorted(out_dir.rglob("*.o"))
    if not objects:
        die(log_path, "No object files found under out_dir; nothing to link")

    # Write POSIX relative paths (stable, and matches typical CubeIDE style)
    with obj_list.open("w", encoding="utf-8", newline="\n") as f:
        for obj in objects:
            rel = obj.resolve().relative_to(Path.cwd().resolve())
            f.write(rel.as_posix() + "\n")

    if obj_list.stat().st_size == 0:
        die(log_path, "objects.list is empty")

    elf = out_dir / f"{project}.elf"

    # Mirror ST ordering: gcc -o elf @"objects.list" ...ldflags...
    cmd = [
        str(gcc_exe),
        "-o",
        str(elf),
        f"@{str(obj_list)}",
        *ldflags,
    ]

    write_log(log_path, "---- Link command ----")
    write_log(log_path, " ".join(str(x) for x in cmd))

    rc = run_cmd(log_path, cmd, env=env)
    if rc != 0:
        die(log_path, "Linking failed")

    print("[OK] Linking finished.")
    write_log(log_path, "[OK] Linking finished.")

    # [6/10] Check ELF (summary only)
    print("[6/10] Checking ELF...")
    write_log(log_path, "[6/10] Checking ELF...")

    if not elf.exists():
        die(log_path, f"ELF missing: {elf}")

    if readelf_exe.exists():
        write_log(log_path, "ELF Summary:")
        line1 = capture_first_matching_line([str(readelf_exe), "-h", str(elf)], "Entry point address", env)
        if line1:
            write_log(log_path, line1)
        line2 = capture_first_matching_line([str(readelf_exe), "-h", str(elf)], "Flags:", env)
        if line2:
            write_log(log_path, line2)
        write_log(log_path, "")
    else:
        write_log(log_path, "[WARN] arm-none-eabi-readelf.exe not found; skip ELF summary")

    # (Optional) Generate .list like ST: objdump -h -S elf > project.list
    if not args.no_objdump and objdump_exe.exists():
        print("[7/10] ObjDump LIST...")
        write_log(log_path, "[7/10] ObjDump LIST...")

        list_file = out_dir / f"{project}.list"
        # We'll capture stdout into file directly (like ST makefile)
        cmd = [str(objdump_exe), "-h", "-S", str(elf)]
        write_log(log_path, " ".join(str(x) for x in cmd))
        p = subprocess.run(
            cmd,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=False,
        )
        if p.returncode != 0:
            # log error output (best effort decode)
            try:
                out_text = (p.stdout or b"").decode("utf-8", errors="replace")
            except Exception:
                out_text = "<objdump output decode failed>"
            write_log(log_path, out_text)
            die(log_path, "Objdump list failed")
        list_file.write_bytes(p.stdout or b"")
        write_log(log_path, f"[OK] LIST created: {list_file}")
    else:
        list_file = None

    # [8/10] Objcopy HEX
    print("[8/10] Objcopy HEX...")
    write_log(log_path, "[8/10] Objcopy HEX...")

    hex_file = out_dir / f"{project}.hex"
    cmd = [str(objcopy_exe), "-O", "ihex", str(elf), str(hex_file)]
    rc = run_cmd(log_path, cmd, env=env)
    if rc != 0:
        die(log_path, "Objcopy HEX failed")

    print("[OK] HEX created.")
    write_log(log_path, "[OK] HEX created.")

    # [9/10] Objcopy BIN + Size
    print("[9/10] Objcopy BIN...")
    write_log(log_path, "[9/10] Objcopy BIN...")

    bin_file = out_dir / f"{project}.bin"
    cmd = [str(objcopy_exe), "-O", "binary", str(elf), str(bin_file)]
    rc = run_cmd(log_path, cmd, env=env)
    if rc != 0:
        die(log_path, "Objcopy BIN failed")

    print("[OK] BIN created.")
    write_log(log_path, "[OK] BIN created.")

    print("[10/10] Size...")
    write_log(log_path, "[10/10] Size...")

    cmd = [str(size_exe), str(elf)]
    rc = run_cmd(log_path, cmd, env=env)
    if rc != 0:
        die(log_path, "Size failed")

    print("========================================")
    print(f"Build DONE ELF/HEX/BIN created in {out_dir}")
    print(f"Log saved to {log_path}")
    print("========================================")

    write_log(log_path, "ARTIFACTS:")
    write_log(log_path, f"  {elf}")
    if list_file:
        write_log(log_path, f"  {list_file}")
    write_log(log_path, f"  {hex_file}")
    write_log(log_path, f"  {bin_file}")
    write_log(log_path, f"  {out_dir / (project + '.map')}")
    write_log(log_path, f"  {obj_list}")
    write_log(log_path, "===== DONE =====")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        print("\n[ERROR] Interrupted by user")
        raise SystemExit(130)
