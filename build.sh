#!/bin/bash
set -e

# Custom UEFI Shell build.sh - works on Linux and MSYS2 (UCRT64/MINGW64)
# Auto-detects gnu-efi, falls back to standalone (no deps)

echo "=== Custom UEFI Shell Build ==="

# MSYS2 needs /ucrt64/bin in PATH for cc1 DLLs
if [ -d "/ucrt64/bin" ]; then
    export PATH="/ucrt64/bin:$PATH"
fi
if [ -d "/mingw64/bin" ]; then
    export PATH="/mingw64/bin:$PATH"
fi

EFI_INC=""
for p in "/ucrt64/include/efi" "/mingw64/include/efi" "/usr/include/efi" "/usr/local/include/efi"; do
    if [ -f "$p/efi.h" ]; then EFI_INC="$p"; break; fi
done

if [ -n "$EFI_INC" ] && [ -f "$EFI_INC/efi.h" ]; then
    echo "Found gnu-efi at $EFI_INC -> building shell.c"
    if command -v gcc >/dev/null 2>&1; then
        echo "Compiling shell.c..."
        gcc -I"$EFI_INC" -I"$EFI_INC/x86_64" -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -Wall -DEFI_FUNCTION_WRAPPER -DGNU_EFI_USE_MS_ABI -c shell.c -o shell.o
        # Find crt0 and lds
        CRT=""
        for c in "$EFI_INC/../lib/crt0-efi-x86_64.o" "/usr/lib/crt0-efi-x86_64.o" "/usr/lib64/gnuefi/crt0-efi-x86_64.o"; do
            if [ -f "$c" ]; then CRT="$c"; break; fi
        done
        LDS=""
        for l in "$EFI_INC/../lib/elf_x86_64_efi.lds" "/usr/lib/elf_x86_64_efi.lds" "/usr/lib64/gnuefi/elf_x86_64_efi.lds"; do
            if [ -f "$l" ]; then LDS="$l"; break; fi
        done
        if [ -n "$CRT" ] && [ -n "$LDS" ]; then
            echo "Linking with gnu-efi (crt $CRT)..."
            ld -nostdlib -znocombreloc -T "$LDS" -shared -Bsymbolic "$CRT" shell.o -o shell.so -L"$(dirname $CRT)" -lgnuefi -lefi || {
                echo "ld failed, trying gcc link fallback..."
                gcc -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main shell.o -o shell.efi
                echo "Build OK: shell.efi (gcc link fallback)"
                ls -lh shell.efi
                exit 0
            }
            objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel* --target=efi-app-x86_64 shell.so shell.efi
            echo "Build OK: shell.efi (gnu-efi)"
            ls -lh shell.efi
            rm -f shell.so shell.o
            exit 0
        else
            echo "crt0/lds not found, falling back to standalone..."
        fi
    fi
fi

echo "gnu-efi not found or not usable -> building STANDALONE (shell_standalone.c)"
echo "This needs only gcc, no efi.h"

if ! command -v gcc >/dev/null 2>&1; then
    echo "ERROR: gcc not found!"
    echo "MSYS2: pacman -S mingw-w64-ucrt-x86_64-gcc  (UCRT64) or mingw-w64-x86_64-gcc (MINGW64)"
    echo "Linux: sudo apt install build-essential"
    exit 1
fi

echo "Compiling shell_standalone.c..."
gcc -ffreestanding -fno-stack-protector -fshort-wchar -mno-red-zone -Wall -c shell_standalone.c -o shell_standalone.o

echo "Linking shell.efi..."
gcc -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main shell_standalone.o -o shell.efi

rm -f shell_standalone.o
echo ""
echo "Build OK: shell.efi (standalone)"
ls -lh shell.efi
echo ""
echo "To test with QEMU:"
echo "  qemu-system-x86_64 -bios /usr/share/OVMF/OVMF_CODE.fd -drive file=fat:rw:.,format=raw"
echo "Or copy to ESP: cp shell.efi /boot/efi/EFI/BOOT/BOOTX64.EFI"
echo ""
echo "Fixed your MSYS2 error:"
echo "  Your error was 'efi.h: No such file or directory' because gnu-efi not installed."
echo "  Standalone build fixes it. Or install gnu-efi: pacman -S mingw-w64-ucrt-x86_64-gnu-efi"
