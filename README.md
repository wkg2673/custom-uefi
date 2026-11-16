<div align="center">

# Custom UEFI Shell — Green `C:\` Edition

A minimal UEFI Shell-like `.efi` application with a green `C:\>` prompt.

<img width="1259" height="1599" alt="wkg2673-uefi-shell" src="https://github.com/user-attachments/assets/ef5160cd-98c0-45c0-9893-cc30935bdfe8" />


</div>

## Features

| Command | Description |
|---------|-------------|
| `C:\>` prompt | Prompt is **green** (`EFI_GREEN` on black), like DOS. |
| `hello` | Replies `Hello!` (case-insensitive) |
| `ls` / `dir` | Lists files on ESP/volume via `EFI_SIMPLE_FILE_SYSTEM_PROTOCOL` |
| `ls <path>` | List sub-directory |
| `echo <text>` | Prints text `shell.c:162` |
| `echo <text> > <file>` | Creates file (e.g. `echo Hello! > hello.txt`) |
| `cat` / `type <file>` | Shows file contents (bonus) |
| `clear` / `cls` | Clears screen |
| `help` / `?` | Shows help |
| `exit` | Exits program (returns from `efi_main`) |
| `shutdown` | powers off PC |
| `reboot` / `reset` | Cold reboot (bonus) |

Supports backspace editing, `Trim16()`, case-insensitive parsing, and ASCII file write via `CHAR16->CHAR8` conversion.

## Files

```

  shell.c            - main UEFI application (gnu-efi)
  shell_standalone.c - Standalone UEFI (doesn't require gnu-efi)
  Makefile           - Linux make (gnu-efi)
  build.bat          - Windows build (clang or mingw-w64)
  build.sh           - Linux helper script
```

## Build

### Option 1: Linux / WSL (recommended, easiest)

```bash
sudo apt update && sudo apt install gnu-efi build-essential binutils
make
# -> shell.efi
```

Or:

```bash
chmod +x build.sh
./build.sh
```

### Option 2: Windows + clang

1. Install LLVM: https://github.com/llvm/llvm-project/releases (or `winget install LLVM.LLVM`)
2. Ensure `clang` and `lld-link` in PATH
3. `build.bat` double-click or `cmd> build.bat`

### Option 3: Windows + mingw-w64

```powershell
winget install --id=MSYS2.MSYS2
# In MSYS2 UCRT64:
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-binutils
# then from project dir:
x86_64-w64-mingw32-gcc -ffreestanding -fno-stack-protector -fshort-wchar -mno-red-zone -c shell.c -o shell.o
x86_64-w64-mingw32-gcc -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main shell.o -o shell.efi
```

### Option 4: Docker (no local install)

```powershell
docker run --rm -v "${pwd}:/work" -w /work fedora bash -c "dnf install -y gnu-efi-devel binutils gcc make && make"
```

## Install / Run

1. Format USB as FAT32 / GPT
2. Copy `shell.efi` to `EFI\BOOT\BOOTX64.EFI` for auto-boot, or to `shell.efi` in root.
3. Boot PC in UEFI mode (Secure Boot OFF or signed).
4. Or test in QEMU:

```bash
qemu-system-x86_64 -bios /usr/share/OVMF/OVMF_CODE.fd -drive file=fat:rw:.,format=raw
# inside QEMU UEFI shell:
# fs0:
# shell.efi
```

## Implementation Details

- **Green prompt**: `gST->ConOut->SetAttribute(EFI_GREEN)` before `Print(L"C:\\")` then reset to `EFI_LIGHTGRAY` `shell.c:293-298`
- **Filesystem**: Opens volume via `LoadedImage->DeviceHandle` + `gEfiSimpleFileSystemProtocolGuid`, fallback via `LocateHandleBuffer` `shell.c:52`
- **ls**: Enumerates with `EFI_FILE_INFO` buffer `4096` + `File->Read` `shell.c:105`
- **echo >**: Finds `>` in command line, splits `text` / `filename`, converts to ASCII, `Open(...CREATE)`, `Write()` `shell.c:164-214`
- **shutdown**: `gRT->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL)` `shell.c:249`
- **exit**: breaks main `while(TRUE)` loop and `Close(Root)` then `return EFI_SUCCESS` `shell.c:244`
- **Input**: `WaitForEvent(ConIn->WaitForKey)` + `ReadKeyStroke` with backspace handling `shell.c:217`

## Notes

- Secure Boot must be disabled unless you sign `shell.efi`.
- File writes go to the ESP/volume the app was launched from.
- Colors use UEFI `SetAttribute`; green is `0x02` (`EFI_GREEN`).

## License

[MIT](LICENSE) - do what you want.
