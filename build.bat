@echo off
setlocal enabledelayedexpansion
echo === Custom UEFI Shell Build (Windows) ===
echo.

REM Add MSYS2 UCRT64 to PATH if present
if exist "D:\msys64\ucrt64\bin\gcc.exe" set "PATH=D:\msys64\ucrt64\bin;D:\msys64\usr\bin;%PATH%"
if exist "C:\msys64\ucrt64\bin\gcc.exe" set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"

REM Check for gnu-efi
set EFI_INC=
if exist "D:\msys64\ucrt64\include\efi\efi.h" set EFI_INC=D:\msys64\ucrt64\include\efi
if exist "C:\msys64\ucrt64\include\efi\efi.h" set EFI_INC=C:\msys64\ucrt64\include\efi
if exist "D:\msys64\mingw64\include\efi\efi.h" set EFI_INC=D:\msys64\mingw64\include\efi
if exist "/usr/include/efi/efi.h" set EFI_INC=/usr/include/efi

if defined EFI_INC (
    echo Found gnu-efi at %EFI_INC%
    echo Building shell.c with gnu-efi...
    goto build_gnuefi
) else (
    echo gnu-efi NOT found - efi.h missing.
    echo Building STANDALONE version - no gnu-efi needed - shell_standalone.c
    goto build_standalone
)

:build_gnuefi
where gcc >nul 2>&1
if errorlevel 1 (
    echo gcc not found in PATH. Install MSYS2 UCRT64 gcc or add to PATH.
    goto no_toolchain
)
set CRT=
if exist "%EFI_INC%\..\lib\crt0-efi-x86_64.o" set CRT=%EFI_INC%\..\lib\crt0-efi-x86_64.o
if exist "D:\msys64\ucrt64\lib\crt0-efi-x86_64.o" set CRT=D:\msys64\ucrt64\lib\crt0-efi-x86_64.o
if exist "C:\msys64\ucrt64\lib\crt0-efi-x86_64.o" set CRT=C:\msys64\ucrt64\lib\crt0-efi-x86_64.o

if not defined CRT (
    echo crt0-efi-x86_64.o not found, falling back to standalone...
    goto build_standalone
)

echo CRT: %CRT%
gcc -I"%EFI_INC%" -I"%EFI_INC%\x86_64" -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -Wall -DEFI_FUNCTION_WRAPPER -DGNU_EFI_USE_MS_ABI -c shell.c -o shell.o
if errorlevel 1 (
    echo gnu-efi compile failed, trying standalone...
    goto build_standalone
)
ld -nostdlib -znocombreloc -T "%EFI_INC%\..\lib\elf_x86_64_efi.lds" -shared -Bsymbolic "%CRT%" shell.o -o shell.so -L "%EFI_INC%\..\lib" -lgnuefi -lefi
if errorlevel 1 (
    echo ld failed, trying standalone with gcc link...
    gcc -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main shell.o -o shell.efi
    if not errorlevel 1 goto done
    goto build_standalone
)
objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel* --target=efi-app-x86_64 shell.so shell.efi
if errorlevel 1 goto build_standalone
goto done

:build_standalone
echo.
echo --- Standalone build (shell_standalone.c) ---
echo This version needs ONLY mingw-w64 gcc, no gnu-efi.
where gcc >nul 2>&1
if not errorlevel 1 goto do_gcc_standalone
where clang >nul 2>&1
if not errorlevel 1 goto clang_standalone
goto no_toolchain

:do_gcc_standalone
echo Compiling shell_standalone.c...
gcc -ffreestanding -fno-stack-protector -fshort-wchar -mno-red-zone -Wall -c shell_standalone.c -o shell_standalone.o
if errorlevel 1 (
    echo compile failed
    goto no_toolchain
)
echo Linking shell.efi...
gcc -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main shell_standalone.o -o shell.efi
if errorlevel 1 (
    echo link failed
    goto no_toolchain
)
del shell_standalone.o 2>nul
goto done

:clang_standalone
echo Found clang, building standalone with clang...
clang --target=x86_64-unknown-windows -ffreestanding -fno-stack-protector -fshort-wchar -mno-red-zone -c shell_standalone.c -o shell_standalone.o
if errorlevel 1 goto no_toolchain
where lld-link >nul 2>&1
if not errorlevel 1 (
    lld-link /subsystem:efi_application /entry:efi_main shell_standalone.o /out:shell.efi
    if not errorlevel 1 goto done
)
clang -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main shell_standalone.o -o shell.efi
if errorlevel 1 goto no_toolchain
goto done

:done
echo.
echo Build succeeded: %cd%\shell.efi
for %%A in (shell.efi) do echo Size: %%~zA bytes
echo.
echo Copy to USB ESP: \EFI\BOOT\BOOTX64.EFI  or run from UEFI Shell via fs0:shell.efi
echo To test in QEMU: qemu-system-x86_64 -bios OVMF.fd -drive file=fat:rw:.,format=raw
echo.
pause
exit /b 0

:no_toolchain
echo.
echo ERROR: No suitable toolchain found or build failed.
echo.
echo FIX for your error efi.h not found:
echo   You have 2 options:
echo.
echo   OPTION A (quick, recommended): Just build standalone version
echo     - Standalone needs ONLY gcc, no gnu-efi.
echo     - In MSYS2 UCRT64 terminal, run:
echo         export PATH=/ucrt64/bin:$PATH
echo         gcc -ffreestanding -fno-stack-protector -fshort-wchar -mno-red-zone -Wall -c shell_standalone.c -o shell_standalone.o
echo         gcc -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main shell_standalone.o -o shell.efi
echo       OR simply run:  ./build.sh   (auto does this)
echo       OR run build.bat again (now auto-falls back to standalone)
echo.
echo   OPTION B: Install gnu-efi to build original shell.c
echo     In MSYS2 UCRT64 terminal, run:
echo         pacman -S --noconfirm mingw-w64-ucrt-x86_64-gnu-efi
echo       For MINGW64: pacman -S mingw-w64-x86_64-gnu-efi
echo       Then re-run: make  or  build.bat
echo.
echo   Other options:
echo     - WSL/Linux: sudo apt install gnu-efi build-essential and make
echo     - Docker: docker run --rm -v "%%cd%%:/work" -w /work fedora bash -c "dnf install -y gnu-efi-devel binutils gcc make and make"
echo.
pause
exit /b 1
