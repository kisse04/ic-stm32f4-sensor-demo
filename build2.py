#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import glob
import os
import shutil
import subprocess
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


def windows_path_to_posix(p: Path) -> str:
    return str(p).replace("\\", "/")


def obj_path(out_dir: Path, src_path: Path) -> Path:
    """
    Put object file under out_dir preserving relative directory structure.
    Example:
      Core/Src/main.c -> out_gcc/Core/Src/main.o
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


def main() -> int:
    parser = argparse.ArgumentParser(description="GCC build script (ported from .bat) - enhanced")
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
    args = parser.parse_args()

    # Match bat: cd /d "%~dp0"
    script_dir = Path(__file__).resolve().parent
    os.chdir(script_dir)

    project = args.project
    out_dir = Path(args.out)
    log_path = Path(args.log)
    gcc_bin = Path(args.gcc_bin)
    ldscript = args.ldscript

    # Clean mode
    if args.clean:
        if out_dir.exists():
            shutil.rmtree(out_dir)
        try:
            log_path.unlink()
        except FileNotFoundError:
            pass
        print("[OK] Clean done.")
        return 0

    # Initialize log (del /q)
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

    # Env: set PATH=%GCC_BIN%;%PATH% (keep it, but we will use absolute exe paths)
    env = os.environ.copy()
    env["PATH"] = str(gcc_bin) + os.pathsep + env.get("PATH", "")

    # Absolute tool paths (avoid PATH lookup issues on Windows)
    gcc_exe = (gcc_bin / "arm-none-eabi-gcc.exe").resolve()
    objcopy_exe = (gcc_bin / "arm-none-eabi-objcopy.exe").resolve()
    readelf_exe = (gcc_bin / "arm-none-eabi-readelf.exe").resolve()
    size_exe = (gcc_bin / "arm-none-eabi-size.exe").resolve()

    write_log(log_path, f"gcc_exe={gcc_exe}")
    write_log(log_path, f"objcopy_exe={objcopy_exe}")
    write_log(log_path, f"readelf_exe={readelf_exe}")
    write_log(log_path, f"size_exe={size_exe}")

    if not gcc_exe.exists():
        die(log_path, f"arm-none-eabi-gcc.exe not found: {gcc_exe}")

    gcc_ver = capture_first_matching_line([str(gcc_exe), "--version"], "gcc", env, cwd=Path.cwd())
    if gcc_ver:
        write_log(log_path, f"GCC_VERSION={gcc_ver}")

    objcopy_ver = capture_first_matching_line([str(objcopy_exe), "--version"], "objcopy", env, cwd=Path.cwd())
    if objcopy_ver:
        write_log(log_path, f"OBJCOPY_VERSION={objcopy_ver}")

    write_log(log_path, "")

    print("========================================")
    print(f"Building {project} using GCC...")
    print("========================================")

    out_dir.mkdir(parents=True, exist_ok=True)

    # Include paths / flags (same as bat)
    inc_flags = [
        r"-ICore\Inc",
        r"-IDrivers\CMSIS\Include",
        r"-IDrivers\CMSIS\Device\ST\STM32F4xx\Include",
        r"-IDrivers\STM32F4xx_HAL_Driver\Inc",
    ]
    cflags = [
        "-mcpu=cortex-m4",
        "-mthumb",
        "-O0",
        "-g",
        "-Wall",
        "-ffunction-sections",
        "-fdata-sections",
        "-DSTM32F446xx",
        "-DUSE_HAL_DRIVER",
    ]
    ldflags = [
        "-T",
        ldscript,
        "-Wl,--gc-sections",
        f"-Wl,-Map={out_dir / (project + '.map')}",
        "-Wl,-e,Reset_Handler",
    ]

    # [1/7] Compile Core/Src
    print("[1/7] Compiling Core/Src...")
    write_log(log_path, "[1/7] Compiling Core/Src...")

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

    # [2/7] Compile HAL Drivers
    print("[2/7] Compiling HAL Drivers...")
    write_log(log_path, "[2/7] Compiling HAL Drivers...")

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

    # [3/7] Compile Startup
    print("[3/7] Compiling Startup...")
    write_log(log_path, "[3/7] Compiling Startup...")

    startup_s = Path(r"Core\Startup\startup_stm32f446retx.s")
    startup_o = obj_path(out_dir, startup_s)  # preserve path too

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

    # [4/7] Link (RSP)
    print("[4/7] Linking...")
    write_log(log_path, "[4/7] Linking...")

    rsp = out_dir / "objects.rsp"
    if rsp.exists():
        rsp.unlink()

    objects = sorted(out_dir.rglob("*.o"))
    with rsp.open("w", encoding="utf-8", newline="\n") as f:
        for obj in objects:
            # Use POSIX-style absolute paths; arm-none-eabi-gcc/ld accept this well on Windows.
            p = obj.resolve().as_posix()
            if " " in p:
                # Response-file quoting is inconsistent across toolchains;
                # fail fast with a clear error instead of generating a broken rsp.
                die(log_path, f"Object path contains spaces; not supported in rsp: {p}")
            f.write(p + "\n")



    rsp_size = rsp.stat().st_size if rsp.exists() else 0
    if rsp_size == 0:
        die(log_path, "Rsp file empty")

    obj_count = sum(1 for _ in rsp.open("r", encoding="utf-8", errors="replace"))
    write_log(log_path, f"RSP={rsp} (size={rsp_size} bytes)")
    write_log(log_path, f"OBJ_COUNT={obj_count}")
    write_log(log_path, "")

    elf = out_dir / f"{project}.elf"

    write_log(log_path, "---- Link command ----")
    write_log(log_path, f"{gcc_exe} {' '.join(cflags)} @{rsp} {' '.join(ldflags)} -o {elf}")

    cmd = [str(gcc_exe), *cflags, f"@{str(rsp.resolve())}", *ldflags, "-o", str(elf)]

    
    rc = run_cmd(log_path, cmd, env=env)
    if rc != 0:
        die(log_path, "Linking failed")

    print("[OK] Linking finished.")
    write_log(log_path, "[OK] Linking finished.")

    # [5/7] Check ELF (summary only)
    print("[5/7] Checking ELF...")
    write_log(log_path, "[5/7] Checking ELF...")

    if not elf.exists():
        die(log_path, f"ELF missing: {elf}")

    if not readelf_exe.exists():
        write_log(log_path, "[WARN] arm-none-eabi-readelf.exe not found; skip ELF summary")
    else:
        write_log(log_path, "ELF Summary:")
        line1 = capture_first_matching_line([str(readelf_exe), "-h", str(elf)], "Entry point address", env)
        if line1:
            write_log(log_path, line1)
        line2 = capture_first_matching_line([str(readelf_exe), "-h", str(elf)], "Number of section headers", env)
        if line2:
            write_log(log_path, line2)
        write_log(log_path, "")

    # [6/7] Objcopy HEX
    print("[6/7] Objcopy HEX...")
    write_log(log_path, "[6/7] Objcopy HEX...")

    hex_file = out_dir / f"{project}.hex"
    cmd = [str(objcopy_exe), "-O", "ihex", str(elf), str(hex_file)]
    rc = run_cmd(log_path, cmd, env=env)
    if rc != 0:
        die(log_path, "Objcopy HEX failed")

    print("[OK] HEX created.")
    write_log(log_path, "[OK] HEX created.")

    # [7/7] Objcopy BIN + Size
    print("[7/7] Objcopy BIN...")
    write_log(log_path, "[7/7] Objcopy BIN...")

    bin_file = out_dir / f"{project}.bin"
    cmd = [str(objcopy_exe), "-O", "binary", str(elf), str(bin_file)]
    rc = run_cmd(log_path, cmd, env=env)
    if rc != 0:
        die(log_path, "Objcopy BIN failed")

    print("[OK] BIN created.")
    write_log(log_path, "[OK] BIN created.")

    print("[7/7] Size...")
    write_log(log_path, "[7/7] Size...")

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
    write_log(log_path, f"  {hex_file}")
    write_log(log_path, f"  {bin_file}")
    write_log(log_path, f"  {out_dir / (project + '.map')}")
    write_log(log_path, "===== DONE =====")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        print("\n[ERROR] Interrupted by user")
        raise SystemExit(130)
