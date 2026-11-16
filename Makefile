# Makefile for Custom UEFI Shell - supports Linux and MSYS2
# Auto-fallback to standalone if gnu-efi not found

ARCH            = x86_64

# Detect MSYS2
UNAME_S := $(shell uname -s 2>/dev/null)
ifeq ($(findstring MSYS,$(UNAME_S)),MSYS)
  EFI_INC := /ucrt64/include/efi
  ifeq ($(wildcard $(EFI_INC)/efi.h),)
    EFI_INC := /mingw64/include/efi
  endif
  # MSYS2 standalone needs PATH
  export PATH := /ucrt64/bin:/mingw64/bin:$(PATH)
else
  EFI_INC := /usr/include/efi
endif

EFI_LIB         = $(shell dirname $(EFI_INC))/../lib
EFI_CRT         = $(EFI_LIB)/crt0-efi-$(ARCH).o
EFI_LDS         = $(EFI_LIB)/elf_$(ARCH)_efi.lds

# Check if gnu-efi exists
HAS_GNUEFI := $(wildcard $(EFI_INC)/efi.h)

ifeq ($(HAS_GNUEFI),)
  # No gnu-efi -> standalone target
  TARGET   = shell.efi
  SRC      = shell_standalone.c
  OBJ      = shell_standalone.o
  all: $(TARGET)

  $(OBJ): $(SRC)
	gcc -ffreestanding -fno-stack-protector -fshort-wchar -mno-red-zone -Wall -c $< -o $@

  $(TARGET): $(OBJ)
	gcc -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main $< -o $@

  clean:
	rm -f $(OBJ) $(TARGET) shell.so shell.o
else
  # gnu-efi -> original shell.c
  TARGET          = shell.efi
  OBJS            = shell.o
  SO              = shell.so
  CC              = gcc
  LD              = ld
  CFLAGS          = -I$(EFI_INC) -I$(EFI_INC)/$(ARCH) -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -Wall -Wextra -DEFI_FUNCTION_WRAPPER -DGNU_EFI_USE_MS_ABI
  LDFLAGS         = -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic $(EFI_CRT)

  all: $(TARGET)

  shell.o: shell.c
	$(CC) $(CFLAGS) -c shell.c -o shell.o

  $(SO): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $(SO) -L $(EFI_LIB) -lgnuefi -lefi

  $(TARGET): $(SO)
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel* --target=efi-app-$(ARCH) $(SO) $(TARGET)

  clean:
	rm -f $(OBJS) $(SO) $(TARGET) shell_standalone.o

endif

# For QEMU test:
# qemu-system-x86_64 -bios OVMF.fd -drive file=fat:rw:.,format=raw
