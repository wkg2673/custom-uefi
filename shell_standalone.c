/*
 * wkg2673's UEFI Shell - STANDALONE (no gnu-efi required)
 * Builds with plain mingw-w64 / clang without efi.h
 * Same features as shell.c but self-contained.
 *
 * Build (MSYS2 UCRT64/MINGW64):
 *   gcc -ffreestanding -fno-stack-protector -fshort-wchar -mno-red-zone -Wall -c shell_standalone.c -o shell_standalone.o
 *   gcc -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main shell_standalone.o -o shell.efi
 *
 * Or just: ./build_standalone.sh  or  build.bat (auto fallback)
 */

#define GNU_EFI_USE_MS_ABI
#define HAVE_USE_MS_ABI

/* ---- Minimal UEFI types ---- */
#include <stddef.h>
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;
typedef unsigned long long UINTN;
typedef long long INTN;
typedef long long INT64;
typedef int INT32;
typedef short INT16;
typedef signed char INT8;
typedef unsigned short CHAR16;
typedef unsigned char CHAR8;
typedef unsigned char BOOLEAN;
#define TRUE 1
#define FALSE 0
typedef void VOID;
typedef UINTN EFI_STATUS;
typedef VOID* EFI_HANDLE;
typedef VOID* EFI_EVENT;
typedef UINT64 EFI_LBA;
typedef UINT64 EFI_PHYSICAL_ADDRESS;
typedef UINT64 EFI_VIRTUAL_ADDRESS;
#define EFI_SUCCESS 0
#define EFI_ERROR(a) ((a) != EFI_SUCCESS)
#define EFIERR(a) (0x8000000000000000ULL | (a))
#define EFI_LOAD_ERROR EFIERR(1)
#define EFI_INVALID_PARAMETER EFIERR(2)
#define EFI_UNSUPPORTED EFIERR(3)
#define EFI_BAD_BUFFER_SIZE EFIERR(4)
#define EFI_BUFFER_TOO_SMALL EFIERR(5)
#define EFI_NOT_READY EFIERR(6)
#define EFI_DEVICE_ERROR EFIERR(7)
#define EFI_WRITE_PROTECTED EFIERR(8)
#define EFI_OUT_OF_RESOURCES EFIERR(9)
#define EFI_VOLUME_CORRUPTED EFIERR(10)
#define EFI_VOLUME_FULL EFIERR(11)
#define EFI_NO_MEDIA EFIERR(12)
#define EFI_MEDIA_CHANGED EFIERR(13)
#define EFI_NOT_FOUND EFIERR(14)
#define EFI_ACCESS_DENIED EFIERR(15)

#define EFIAPI __attribute__((ms_abi))

typedef struct { UINT32 Data1; UINT16 Data2; UINT16 Data3; UINT8 Data4[8]; } EFI_GUID;

/* GUIDs */
static const EFI_GUID gEfiSimpleFileSystemProtocolGuid = {0x964e5b22,0x6459,0x11d2,{0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}};
static const EFI_GUID gEfiLoadedImageProtocolGuid      = {0x5ab1b31a,0x9562,0x11d2,{0x8e,0x3f,0x00,0xa0,0xc9,0x69,0x72,0x3b}};
static const EFI_GUID gEfiFileInfoGuid                 = {0x09576e92,0x6d3f,0x11d2,{0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}};

#define EFI_FILE_MODE_READ   0x0000000000000001ULL
#define EFI_FILE_MODE_WRITE  0x0000000000000002ULL
#define EFI_FILE_MODE_CREATE 0x8000000000000000ULL
#define EFI_FILE_READ_ONLY   0x0000000000000001ULL
#define EFI_FILE_DIRECTORY   0x0000000000000010ULL
#define EFI_FILE_ARCHIVE     0x0000000000000020ULL

/* Text attributes */
#define EFI_BLACK 0x00
#define EFI_BLUE 0x01
#define EFI_GREEN 0x02
#define EFI_CYAN (EFI_BLUE | EFI_GREEN)
#define EFI_RED 0x04
#define EFI_MAGENTA (EFI_BLUE | EFI_RED)
#define EFI_BROWN (EFI_GREEN | EFI_RED)
#define EFI_LIGHTGRAY (EFI_BLUE | EFI_GREEN | EFI_RED)
#define EFI_BRIGHT 0x08
#define EFI_DARKGRAY (EFI_BLACK | EFI_BRIGHT)
#define EFI_WHITE (EFI_LIGHTGRAY | EFI_BRIGHT)
#define EFI_LIGHTRED (EFI_RED | EFI_BRIGHT)
#define EFI_TEXT_ATTR(f,b) ((f) | ((b)<<4))

/* Chars */
#define CHAR_NULL 0x0000
#define CHAR_BACKSPACE 0x0008
#define CHAR_TAB 0x0009
#define CHAR_LINEFEED 0x000A
#define CHAR_CARRIAGE_RETURN 0x000D
#define SCAN_NULL 0x0000
#define SCAN_ESC 0x0017

/* Reset types */
typedef enum { EfiResetCold, EfiResetWarm, EfiResetShutdown, EfiResetPlatformSpecific } EFI_RESET_TYPE;

/* Time (minimal) */
typedef struct {
    UINT16 Year; UINT8 Month; UINT8 Day; UINT8 Hour; UINT8 Minute; UINT8 Second;
    UINT8 Pad1; UINT32 Nanosecond; INT16 TimeZone; UINT8 Daylight; UINT8 Pad2;
} EFI_TIME;

/* Memory etc */
typedef enum { AllocateAnyPages, AllocateMaxAddress, AllocateAddress, MaxAllocateType } EFI_ALLOCATE_TYPE;
typedef enum { EfiReservedMemoryType, EfiLoaderCode, EfiLoaderData, EfiBootServicesCode, EfiBootServicesData, EfiRuntimeServicesCode, EfiRuntimeServicesData, EfiConventionalMemory, EfiUnusableMemory, EfiACPIReclaimMemory, EfiACPIMemoryNVS, EfiMemoryMappedIO, EfiMemoryMappedIOPortSpace, EfiPalCode, EfiPersistentMemory, EfiMaxMemoryType } EFI_MEMORY_TYPE;
typedef struct { UINT32 Type; EFI_PHYSICAL_ADDRESS PhysicalStart; EFI_VIRTUAL_ADDRESS VirtualStart; UINT64 NumberOfPages; UINT64 Attribute; } EFI_MEMORY_DESCRIPTOR;
typedef UINTN EFI_TPL;

/* Forward decls */
struct _EFI_SIMPLE_TEXT_OUTPUT_INTERFACE;
struct _EFI_SIMPLE_TEXT_INPUT_INTERFACE;
struct _EFI_FILE_PROTOCOL;
struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;
struct _EFI_BOOT_SERVICES;
struct _EFI_RUNTIME_SERVICES;
struct _EFI_SYSTEM_TABLE;

/* File info */
typedef struct {
    UINT64 Size;
    UINT64 FileSize;
    UINT64 PhysicalSize;
    EFI_TIME CreateTime;
    EFI_TIME LastAccessTime;
    EFI_TIME ModificationTime;
    UINT64 Attribute;
    CHAR16 FileName[1];
} EFI_FILE_INFO;

/* Console */
typedef EFI_STATUS (EFIAPI *EFI_TEXT_RESET)(struct _EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *This, BOOLEAN Ext);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_STRING)(struct _EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *This, CHAR16 *String);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_TEST_STRING)(struct _EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *This, CHAR16 *String);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_QUERY_MODE)(struct _EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *This, UINTN Mode, UINTN *Cols, UINTN *Rows);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_MODE)(struct _EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *This, UINTN Mode);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_ATTRIBUTE)(struct _EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *This, UINTN Attr);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_CLEAR_SCREEN)(struct _EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *This);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_CURSOR_POSITION)(struct _EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *This, UINTN Col, UINTN Row);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_ENABLE_CURSOR)(struct _EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *This, BOOLEAN En);
typedef struct { INT32 MaxMode; INT32 Mode; INT32 Attribute; INT32 CursorColumn; INT32 CursorRow; BOOLEAN CursorVisible; } SIMPLE_TEXT_OUTPUT_MODE;
typedef struct _EFI_SIMPLE_TEXT_OUTPUT_INTERFACE {
    EFI_TEXT_RESET Reset;
    EFI_TEXT_STRING OutputString;
    EFI_TEXT_TEST_STRING TestString;
    EFI_TEXT_QUERY_MODE QueryMode;
    EFI_TEXT_SET_MODE SetMode;
    EFI_TEXT_SET_ATTRIBUTE SetAttribute;
    EFI_TEXT_CLEAR_SCREEN ClearScreen;
    EFI_TEXT_SET_CURSOR_POSITION SetCursorPosition;
    EFI_TEXT_ENABLE_CURSOR EnableCursor;
    SIMPLE_TEXT_OUTPUT_MODE *Mode;
} EFI_SIMPLE_TEXT_OUTPUT_INTERFACE;

typedef struct { UINT16 ScanCode; CHAR16 UnicodeChar; } EFI_INPUT_KEY;
typedef EFI_STATUS (EFIAPI *EFI_INPUT_RESET)(struct _EFI_SIMPLE_TEXT_INPUT_INTERFACE *This, BOOLEAN Ext);
typedef EFI_STATUS (EFIAPI *EFI_INPUT_READ_KEY)(struct _EFI_SIMPLE_TEXT_INPUT_INTERFACE *This, EFI_INPUT_KEY *Key);
typedef struct _EFI_SIMPLE_TEXT_INPUT_INTERFACE {
    EFI_INPUT_RESET Reset;
    EFI_INPUT_READ_KEY ReadKeyStroke;
    EFI_EVENT WaitForKey;
} EFI_SIMPLE_TEXT_INPUT_INTERFACE;

/* File */
typedef struct _EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;
typedef EFI_STATUS (EFIAPI *EFI_FILE_OPEN)(EFI_FILE_PROTOCOL *This, EFI_FILE_PROTOCOL **NewHandle, CHAR16 *FileName, UINT64 OpenMode, UINT64 Attributes);
typedef EFI_STATUS (EFIAPI *EFI_FILE_CLOSE)(EFI_FILE_PROTOCOL *This);
typedef EFI_STATUS (EFIAPI *EFI_FILE_DELETE)(EFI_FILE_PROTOCOL *This);
typedef EFI_STATUS (EFIAPI *EFI_FILE_READ)(EFI_FILE_PROTOCOL *This, UINTN *BufferSize, VOID *Buffer);
typedef EFI_STATUS (EFIAPI *EFI_FILE_WRITE)(EFI_FILE_PROTOCOL *This, UINTN *BufferSize, VOID *Buffer);
typedef EFI_STATUS (EFIAPI *EFI_FILE_GET_POSITION)(EFI_FILE_PROTOCOL *This, UINT64 *Position);
typedef EFI_STATUS (EFIAPI *EFI_FILE_SET_POSITION)(EFI_FILE_PROTOCOL *This, UINT64 Position);
typedef EFI_STATUS (EFIAPI *EFI_FILE_GET_INFO)(EFI_FILE_PROTOCOL *This, EFI_GUID *Type, UINTN *BufferSize, VOID *Buffer);
typedef EFI_STATUS (EFIAPI *EFI_FILE_SET_INFO)(EFI_FILE_PROTOCOL *This, EFI_GUID *Type, UINTN BufferSize, VOID *Buffer);
typedef EFI_STATUS (EFIAPI *EFI_FILE_FLUSH)(EFI_FILE_PROTOCOL *This);
struct _EFI_FILE_PROTOCOL {
    UINT64 Revision;
    EFI_FILE_OPEN Open;
    EFI_FILE_CLOSE Close;
    EFI_FILE_DELETE Delete;
    EFI_FILE_READ Read;
    EFI_FILE_WRITE Write;
    EFI_FILE_GET_POSITION GetPosition;
    EFI_FILE_SET_POSITION SetPosition;
    EFI_FILE_GET_INFO GetInfo;
    EFI_FILE_SET_INFO SetInfo;
    EFI_FILE_FLUSH Flush;
};

/* Simple FS */
typedef struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
    UINT64 Revision;
    EFI_STATUS (EFIAPI *OpenVolume)(struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This, EFI_FILE_PROTOCOL **Root);
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

/* Loaded Image */
typedef struct {
    UINT32 Revision;
    EFI_HANDLE ParentHandle;
    struct _EFI_SYSTEM_TABLE *SystemTable;
    EFI_HANDLE DeviceHandle;
    VOID *FilePath; /* EFI_DEVICE_PATH* */
    VOID *Reserved;
    UINT32 LoadOptionsSize;
    VOID *LoadOptions;
    VOID *ImageBase;
    UINT64 ImageSize;
    EFI_MEMORY_TYPE ImageCodeType;
    EFI_MEMORY_TYPE ImageDataType;
    EFI_STATUS (EFIAPI *Unload)(EFI_HANDLE ImageHandle);
} EFI_LOADED_IMAGE_PROTOCOL;

/* Device Path stub */
typedef struct _EFI_DEVICE_PATH_PROTOCOL { UINT8 Type; UINT8 SubType; UINT8 Length[2]; } EFI_DEVICE_PATH_PROTOCOL;

/* Boot Services - full correct order! */
typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_PAGES)(EFI_ALLOCATE_TYPE Type, EFI_MEMORY_TYPE MemType, UINTN Pages, EFI_PHYSICAL_ADDRESS *Memory);
typedef EFI_STATUS (EFIAPI *EFI_FREE_PAGES)(EFI_PHYSICAL_ADDRESS Memory, UINTN Pages);
typedef EFI_STATUS (EFIAPI *EFI_GET_MEMORY_MAP)(UINTN *MemoryMapSize, EFI_MEMORY_DESCRIPTOR *MemoryMap, UINTN *MapKey, UINTN *DescriptorSize, UINT32 *DescriptorVersion);
typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_POOL)(EFI_MEMORY_TYPE PoolType, UINTN Size, VOID **Buffer);
typedef EFI_STATUS (EFIAPI *EFI_FREE_POOL)(VOID *Buffer);
typedef VOID (EFIAPI *EFI_COPY_MEM)(VOID *Dest, VOID *Src, UINTN Len);
typedef VOID (EFIAPI *EFI_SET_MEM)(VOID *Buffer, UINTN Size, UINT8 Value);
typedef EFI_STATUS (EFIAPI *EFI_CREATE_EVENT)(UINT32 Type, EFI_TPL NotifyTpl, VOID *NotifyFunc, VOID *NotifyContext, EFI_EVENT *Event);
typedef EFI_STATUS (EFIAPI *EFI_SET_TIMER)(EFI_EVENT Event, UINT32 Type, UINT64 TriggerTime);
typedef EFI_STATUS (EFIAPI *EFI_WAIT_FOR_EVENT)(UINTN NumberOfEvents, EFI_EVENT *Event, UINTN *Index);
typedef EFI_STATUS (EFIAPI *EFI_SIGNAL_EVENT)(EFI_EVENT Event);
typedef EFI_STATUS (EFIAPI *EFI_CLOSE_EVENT)(EFI_EVENT Event);
typedef EFI_STATUS (EFIAPI *EFI_CHECK_EVENT)(EFI_EVENT Event);
typedef EFI_STATUS (EFIAPI *EFI_INSTALL_PROTOCOL_INTERFACE)(EFI_HANDLE *Handle, EFI_GUID *Protocol, UINT32 InterfaceType, VOID *Interface);
typedef EFI_STATUS (EFIAPI *EFI_REINSTALL_PROTOCOL_INTERFACE)(EFI_HANDLE Handle, EFI_GUID *Protocol, VOID *OldInterface, VOID *NewInterface);
typedef EFI_STATUS (EFIAPI *EFI_UNINSTALL_PROTOCOL_INTERFACE)(EFI_HANDLE Handle, EFI_GUID *Protocol, VOID *Interface);
typedef EFI_STATUS (EFIAPI *EFI_HANDLE_PROTOCOL)(EFI_HANDLE Handle, EFI_GUID *Protocol, VOID **Interface);
typedef EFI_STATUS (EFIAPI *EFI_REGISTER_PROTOCOL_NOTIFY)(EFI_GUID *Protocol, EFI_EVENT Event, VOID **Registration);
typedef EFI_STATUS (EFIAPI *EFI_LOCATE_HANDLE)(UINT32 SearchType, EFI_GUID *Protocol, VOID *SearchKey, UINTN *BufferSize, EFI_HANDLE *Buffer);
typedef EFI_STATUS (EFIAPI *EFI_LOCATE_DEVICE_PATH)(EFI_GUID *Protocol, EFI_DEVICE_PATH_PROTOCOL **DevicePath, EFI_HANDLE *Device);
typedef EFI_STATUS (EFIAPI *EFI_INSTALL_CONFIGURATION_TABLE)(EFI_GUID *Guid, VOID *Table);
typedef EFI_STATUS (EFIAPI *EFI_IMAGE_LOAD)(BOOLEAN BootPolicy, EFI_HANDLE ParentImageHandle, EFI_DEVICE_PATH_PROTOCOL *FilePath, VOID *SourceBuffer, UINTN SourceSize, EFI_HANDLE *ImageHandle);
typedef EFI_STATUS (EFIAPI *EFI_IMAGE_START)(EFI_HANDLE ImageHandle, UINTN *ExitDataSize, CHAR16 **ExitData);
typedef EFI_STATUS (EFIAPI *EFI_EXIT)(EFI_HANDLE ImageHandle, EFI_STATUS ExitStatus, UINTN ExitDataSize, CHAR16 *ExitData);
typedef EFI_STATUS (EFIAPI *EFI_IMAGE_UNLOAD)(EFI_HANDLE ImageHandle);
typedef EFI_STATUS (EFIAPI *EFI_EXIT_BOOT_SERVICES)(EFI_HANDLE ImageHandle, UINTN MapKey);
typedef EFI_STATUS (EFIAPI *EFI_GET_NEXT_MONOTONIC_COUNT)(UINT64 *Count);
typedef EFI_STATUS (EFIAPI *EFI_STALL)(UINTN Microseconds);
typedef EFI_STATUS (EFIAPI *EFI_SET_WATCHDOG_TIMER)(UINTN Timeout, UINT64 WatchdogCode, UINTN DataSize, CHAR16 *WatchdogData);
typedef EFI_STATUS (EFIAPI *EFI_CONNECT_CONTROLLER)(EFI_HANDLE ControllerHandle, EFI_HANDLE *DriverImageHandle, EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath, BOOLEAN Recursive);
typedef EFI_STATUS (EFIAPI *EFI_DISCONNECT_CONTROLLER)(EFI_HANDLE ControllerHandle, EFI_HANDLE DriverImageHandle, EFI_HANDLE ChildHandle);
typedef EFI_STATUS (EFIAPI *EFI_OPEN_PROTOCOL)(EFI_HANDLE Handle, EFI_GUID *Protocol, VOID **Interface, EFI_HANDLE AgentHandle, EFI_HANDLE ControllerHandle, UINT32 Attributes);
typedef EFI_STATUS (EFIAPI *EFI_CLOSE_PROTOCOL)(EFI_HANDLE Handle, EFI_GUID *Protocol, EFI_HANDLE AgentHandle, EFI_HANDLE ControllerHandle);
typedef EFI_STATUS (EFIAPI *EFI_OPEN_PROTOCOL_INFORMATION)(EFI_HANDLE Handle, EFI_GUID *Protocol, VOID **EntryBuffer, UINTN *EntryCount);
typedef EFI_STATUS (EFIAPI *EFI_PROTOCOLS_PER_HANDLE)(EFI_HANDLE Handle, EFI_GUID ***ProtocolBuffer, UINTN *ProtocolBufferCount);
typedef EFI_STATUS (EFIAPI *EFI_LOCATE_HANDLE_BUFFER)(UINT32 SearchType, EFI_GUID *Protocol, VOID *SearchKey, UINTN *NoHandles, EFI_HANDLE **Buffer);
typedef EFI_STATUS (EFIAPI *EFI_LOCATE_PROTOCOL)(EFI_GUID *Protocol, VOID *Registration, VOID **Interface);
typedef EFI_STATUS (EFIAPI *EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES)(EFI_HANDLE *Handle, ...);
typedef EFI_STATUS (EFIAPI *EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES)(EFI_HANDLE Handle, ...);
typedef EFI_STATUS (EFIAPI *EFI_CALCULATE_CRC32)(VOID *Data, UINTN DataSize, UINT32 *Crc32);
typedef EFI_STATUS (EFIAPI *EFI_CREATE_EVENT_EX)(UINT32 Type, EFI_TPL NotifyTpl, VOID *NotifyFunction, VOID *NotifyContext, EFI_GUID *EventGroup, EFI_EVENT *Event);

typedef struct _EFI_BOOT_SERVICES {
    struct { UINT64 Signature; UINT32 Revision; UINT32 HeaderSize; UINT32 CRC32; UINT32 Reserved; } Hdr;
    EFI_STATUS (EFIAPI *RaiseTPL)(EFI_TPL NewTpl);
    VOID (EFIAPI *RestoreTPL)(EFI_TPL OldTpl);
    EFI_ALLOCATE_PAGES AllocatePages;
    EFI_FREE_PAGES FreePages;
    EFI_GET_MEMORY_MAP GetMemoryMap;
    EFI_ALLOCATE_POOL AllocatePool;
    EFI_FREE_POOL FreePool;
    EFI_CREATE_EVENT CreateEvent;
    EFI_SET_TIMER SetTimer;
    EFI_WAIT_FOR_EVENT WaitForEvent;
    EFI_SIGNAL_EVENT SignalEvent;
    EFI_CLOSE_EVENT CloseEvent;
    EFI_CHECK_EVENT CheckEvent;
    EFI_INSTALL_PROTOCOL_INTERFACE InstallProtocolInterface;
    EFI_REINSTALL_PROTOCOL_INTERFACE ReinstallProtocolInterface;
    EFI_UNINSTALL_PROTOCOL_INTERFACE UninstallProtocolInterface;
    EFI_HANDLE_PROTOCOL HandleProtocol;
    VOID *Reserved;
    EFI_REGISTER_PROTOCOL_NOTIFY RegisterProtocolNotify;
    EFI_LOCATE_HANDLE LocateHandle;
    EFI_LOCATE_DEVICE_PATH LocateDevicePath;
    EFI_INSTALL_CONFIGURATION_TABLE InstallConfigurationTable;
    EFI_IMAGE_LOAD LoadImage;
    EFI_IMAGE_START StartImage;
    EFI_EXIT Exit;
    EFI_IMAGE_UNLOAD UnloadImage;
    EFI_EXIT_BOOT_SERVICES ExitBootServices;
    EFI_GET_NEXT_MONOTONIC_COUNT GetNextMonotonicCount;
    EFI_STALL Stall;
    EFI_SET_WATCHDOG_TIMER SetWatchdogTimer;
    EFI_CONNECT_CONTROLLER ConnectController;
    EFI_DISCONNECT_CONTROLLER DisconnectController;
    EFI_OPEN_PROTOCOL OpenProtocol;
    EFI_CLOSE_PROTOCOL CloseProtocol;
    EFI_OPEN_PROTOCOL_INFORMATION OpenProtocolInformation;
    EFI_PROTOCOLS_PER_HANDLE ProtocolsPerHandle;
    EFI_LOCATE_HANDLE_BUFFER LocateHandleBuffer;
    EFI_LOCATE_PROTOCOL LocateProtocol;
    EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES InstallMultipleProtocolInterfaces;
    EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES UninstallMultipleProtocolInterfaces;
    EFI_CALCULATE_CRC32 CalculateCrc32;
    EFI_COPY_MEM CopyMem;
    EFI_SET_MEM SetMem;
    EFI_CREATE_EVENT_EX CreateEventEx;
} EFI_BOOT_SERVICES;

/* Runtime */
typedef EFI_STATUS (EFIAPI *EFI_GET_TIME)(EFI_TIME *Time, VOID *Capabilities);
typedef EFI_STATUS (EFIAPI *EFI_SET_TIME)(EFI_TIME *Time);
typedef EFI_STATUS (EFIAPI *EFI_GET_WAKEUP_TIME)(BOOLEAN *Enabled, BOOLEAN *Pending, EFI_TIME *Time);
typedef EFI_STATUS (EFIAPI *EFI_SET_WAKEUP_TIME)(BOOLEAN Enable, EFI_TIME *Time);
typedef EFI_STATUS (EFIAPI *EFI_SET_VIRTUAL_ADDRESS_MAP)(UINTN MemoryMapSize, UINTN DescriptorSize, UINT32 DescriptorVersion, EFI_MEMORY_DESCRIPTOR *VirtualMap);
typedef EFI_STATUS (EFIAPI *EFI_CONVERT_POINTER)(UINTN DebugDisposition, VOID **Address);
typedef EFI_STATUS (EFIAPI *EFI_GET_VARIABLE)(CHAR16 *VariableName, EFI_GUID *VendorGuid, UINT32 *Attributes, UINTN *DataSize, VOID *Data);
typedef EFI_STATUS (EFIAPI *EFI_GET_NEXT_VARIABLE_NAME)(UINTN *VariableNameSize, CHAR16 *VariableName, EFI_GUID *VendorGuid);
typedef EFI_STATUS (EFIAPI *EFI_SET_VARIABLE)(CHAR16 *VariableName, EFI_GUID *VendorGuid, UINT32 Attributes, UINTN DataSize, VOID *Data);
typedef EFI_STATUS (EFIAPI *EFI_GET_NEXT_HIGH_MONO_COUNT)(UINT32 *HighCount);
typedef EFI_STATUS (EFIAPI *EFI_RESET_SYSTEM)(EFI_RESET_TYPE ResetType, EFI_STATUS ResetStatus, UINTN DataSize, VOID *ResetData);
typedef struct {
    struct { UINT64 Signature; UINT32 Revision; UINT32 HeaderSize; UINT32 CRC32; UINT32 Reserved; } Hdr;
    EFI_GET_TIME GetTime;
    EFI_SET_TIME SetTime;
    EFI_GET_WAKEUP_TIME GetWakeupTime;
    EFI_SET_WAKEUP_TIME SetWakeupTime;
    EFI_SET_VIRTUAL_ADDRESS_MAP SetVirtualAddressMap;
    EFI_CONVERT_POINTER ConvertPointer;
    EFI_GET_VARIABLE GetVariable;
    EFI_GET_NEXT_VARIABLE_NAME GetNextVariableName;
    EFI_SET_VARIABLE SetVariable;
    EFI_GET_NEXT_HIGH_MONO_COUNT GetNextHighMonotonicCount;
    EFI_RESET_SYSTEM ResetSystem;
    VOID *UpdateCapsule;
    VOID *QueryCapsuleCapabilities;
    VOID *QueryVariableInfo;
} EFI_RUNTIME_SERVICES;

/* System Table */
typedef struct _EFI_SYSTEM_TABLE {
    struct { UINT64 Signature; UINT32 Revision; UINT32 HeaderSize; UINT32 CRC32; UINT32 Reserved; } Hdr;
    CHAR16 *FirmwareVendor;
    UINT32 FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_INTERFACE *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *ConOut;
    EFI_HANDLE StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_INTERFACE *StdErr;
    EFI_RUNTIME_SERVICES *RuntimeServices;
    EFI_BOOT_SERVICES *BootServices;
    UINTN NumberOfTableEntries;
    VOID *ConfigurationTable;
} EFI_SYSTEM_TABLE;

/* Locate search types */
#define AllHandles 0
#define ByProtocol 2

/* ---- Globals ---- */
static EFI_SYSTEM_TABLE *gST;
static EFI_BOOT_SERVICES *gBS;
static EFI_RUNTIME_SERVICES *gRT;
static EFI_FILE_PROTOCOL *Root = NULL;
static BOOLEAN UseGermanLayout = TRUE; // German QWERTZ is now standard (US via 'layout us')

#define MAX_CMD_LEN 512

/* Global variable GUID for GetVariable */
static const EFI_GUID gEfiDevicePathProtocolGuid = {0x09576E91, 0x6D3F, 0x11D2, {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};
static const EFI_GUID gEfiGlobalVariableGuid = {0x8BE4DF61, 0x93CA, 0x11D2, {0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B, 0x8C}};

/* Ex protocol GUID and structs for Ctrl+Plus/Minus resize */
static const EFI_GUID gEfiSimpleTextInputExProtocolGuid = {0xDD9E7534, 0x7762, 0x4698, {0x8C, 0x14, 0xF5, 0x85, 0x17, 0xA6, 0x25, 0xAA}};
typedef struct { UINT32 KeyShiftState; UINT8 KeyToggleState; } EFI_KEY_STATE_TMP;
#define EFI_SHIFT_STATE_VALID       0x80000000
#define EFI_LEFT_CONTROL_PRESSED    0x00000008
#define EFI_RIGHT_CONTROL_PRESSED   0x00000004
#define EFI_LEFT_ALT_PRESSED        0x00000020
#define SCAN_UP 0x0001
#define SCAN_DOWN 0x0002
#define SCAN_F11 0x0015
#define SCAN_F12 0x0016
typedef struct { EFI_INPUT_KEY Key; EFI_KEY_STATE_TMP KeyState; } EFI_KEY_DATA_TMP;
typedef struct _EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_TMP {
    EFI_STATUS (EFIAPI *Reset)(struct _EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_TMP *This, BOOLEAN Ext);
    EFI_STATUS (EFIAPI *ReadKeyStrokeEx)(struct _EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_TMP *This, EFI_KEY_DATA_TMP *KeyData);
    EFI_EVENT WaitForKeyEx;
    EFI_STATUS (EFIAPI *SetState)(struct _EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_TMP *This, UINT8 *ToggleState);
    EFI_STATUS (EFIAPI *RegisterKeyNotify)(struct _EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_TMP *This, EFI_KEY_DATA_TMP *KeyData, VOID *NotifyFunc, VOID **Handle);
    EFI_STATUS (EFIAPI *UnregisterKeyNotify)(struct _EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_TMP *This, VOID *Handle);
} EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_TMP;
static EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_TMP *ConInEx = NULL;

/* Forward for resize */
static void ResizeConsole(BOOLEAN enlarge);

/* ---- Keyboard layout - forward decls for utilities ---- */
static UINTN StrLen16(const CHAR16 *s);
static CHAR16 ToLower16(CHAR16 c);
static void PrintStr(const CHAR16 *s);
static void ConWrite(const CHAR16 *s);

static BOOLEAN StrContainsDe(const CHAR16 *s) {
    UINTN len = StrLen16(s);
    for (UINTN i = 0; i + 1 < len; i++) {
        if (ToLower16(s[i]) == L'd' && ToLower16(s[i+1]) == L'e') return TRUE;
    }
    return FALSE;
}
static BOOLEAN StrContainsEn(const CHAR16 *s) {
    UINTN len = StrLen16(s);
    for (UINTN i = 0; i + 1 < len; i++) {
        if (ToLower16(s[i]) == L'e' && ToLower16(s[i+1]) == L'n') return TRUE;
    }
    return FALSE;
}

static CHAR16 TranslateGermanChar(CHAR16 c) {
    // Full US -> German QWERTZ mapping (when firmware is US but physical keyboard is German)
    // Covers y/z, umlauts, ß, and Shift+. -> : etc.
    switch (c) {
        // unshifted row
        case L'`': return L'^';
        case L'-': return 0x00DF; // ß
        case L'=': return 0x00B4; // ´
        case L'q': return L'q'; case L'Q': return L'Q';
        case L'w': return L'w'; case L'W': return L'W';
        case L'e': return L'e'; case L'E': return L'E';
        case L'r': return L'r'; case L'R': return L'R';
        case L't': return L't'; case L'T': return L'T';
        case L'y': return L'z'; case L'Y': return L'Z';
        case L'u': return L'u'; case L'U': return L'U';
        case L'i': return L'i'; case L'I': return L'I';
        case L'o': return L'o'; case L'O': return L'O';
        case L'p': return L'p'; case L'P': return L'P';
        case L'[': return 0x00FC; // ü
        case L'{': return 0x00DC; // Ü
        case L']': return L'+';
        case L'}': return L'*';
        case L'\\': return L'#';
        case L'|': return L'\'';
        case L'a': return L'a'; case L'A': return L'A';
        case L's': return L's'; case L'S': return L'S';
        case L'd': return L'd'; case L'D': return L'D';
        case L'f': return L'f'; case L'F': return L'F';
        case L'g': return L'g'; case L'G': return L'G';
        case L'h': return L'h'; case L'H': return L'H';
        case L'j': return L'j'; case L'J': return L'J';
        case L'k': return L'k'; case L'K': return L'K';
        case L'l': return L'l'; case L'L': return L'L';
        case L';': return 0x00F6; // ö
        case L':': return 0x00D6; // Ö
        case L'\'': return 0x00E4; // ä
        case L'"': return 0x00C4; // Ä
        case L'z': return L'y'; case L'Z': return L'Y';
        case L'x': return L'x'; case L'X': return L'X';
        case L'c': return L'c'; case L'C': return L'C';
        case L'v': return L'v'; case L'V': return L'V';
        case L'b': return L'b'; case L'B': return L'B';
        case L'n': return L'n'; case L'N': return L'N';
        case L'm': return L'm'; case L'M': return L'M';
        case L',': return L',';
        case L'<': return L';';
        case L'.': return L'.';
        case L'>': return L':';
        case L'/': return L'-';
        case L'?': return L'_';
        // shifted row
        case L'~': return 0x00B0; // °
        case L'!': return L'!';
        case L'@': return L'"';
        case L'#': return 0x00A7; // §
        case L'$': return L'$';
        case L'%': return L'%';
        case L'^': return L'&';
        case L'&': return L'/';
        case L'*': return L'(';
        case L'(': return L')';
        case L')': return L'=';
        case L'_': return L'?';
        case L'+': return L'`';
        default: return c;
    }
}

static void DetectKeyboardLayout(void) {
    // German is now standard (UseGermanLayout = TRUE by default)
    // Only switch to US if firmware explicitly reports en
    CHAR16 buf[64];
    UINTN size = sizeof(buf);
    UINT32 attr = 0;
    EFI_STATUS st;

    size = sizeof(buf);
    st = gRT->GetVariable(L"PlatformLang", (EFI_GUID*)&gEfiGlobalVariableGuid, &attr, &size, buf);
    if (!EFI_ERROR(st) && size >= 2) {
        buf[(size/2) < 63 ? (size/2) : 63] = 0;
        if (StrContainsDe(buf)) { UseGermanLayout = TRUE; return; }
        if (StrContainsEn(buf)) { UseGermanLayout = FALSE; return; }
        // unknown lang, keep German standard
        return;
    }
    size = sizeof(buf);
    st = gRT->GetVariable(L"Lang", (EFI_GUID*)&gEfiGlobalVariableGuid, &attr, &size, buf);
    if (!EFI_ERROR(st) && size >= 2) {
        if (StrContainsDe(buf)) { UseGermanLayout = TRUE; return; }
        if (StrContainsEn(buf)) { UseGermanLayout = FALSE; return; }
    }
    // No variable or unknown -> keep German standard
}

static void PrintKeyboardInfo(void) {
    if (UseGermanLayout) {
        gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
        PrintStr(L"Layout: German QWERTZ (standard)\r\n");
    } else {
        PrintStr(L"Layout: US QWERTY (use 'layout de' for German)\r\n");
    }
}

static void ResizeConsole(BOOLEAN enlarge) {
    if (!gST || !gST->ConOut || !gST->ConOut->Mode) return;
    INTN cur = gST->ConOut->Mode->Mode;
    INTN max = gST->ConOut->Mode->MaxMode;
    if (max <= 1) return;
    INTN next = cur + (enlarge ? 1 : -1);
    if (next < 0) next = max - 1;
    if (next >= max) next = 0;
    EFI_STATUS st = gST->ConOut->SetMode(gST->ConOut, next);
    if (!EFI_ERROR(st)) {
        // SetMode clears screen, re-apply attribute
        gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
    }
}

static BOOLEAN IsCtrlPressed(EFI_KEY_STATE_TMP *State) {
    if (!State) return FALSE;
    // Some firmware does not set EFI_SHIFT_STATE_VALID, so check Ctrl bits directly
    // Also handle case where State is zero but Ctrl still pressed (fallback)
    return (State->KeyShiftState & (EFI_LEFT_CONTROL_PRESSED | EFI_RIGHT_CONTROL_PRESSED)) != 0;
}

/* ---- Utilities ---- */
static UINTN StrLen16(const CHAR16 *s){ UINTN i=0; while(s[i]) i++; return i; }
static void StrCpy16(CHAR16 *d, const CHAR16 *s){ while((*d++=*s++)); }
static CHAR16 ToLower16(CHAR16 c){ if(c>=L'A'&&c<=L'Z') return c+32; return c; }
static INTN StrCmp16(const CHAR16 *a, const CHAR16 *b){ while(*a && *b && *a==*b){a++;b++;} return (INTN)*a-(INTN)*b; }
static int StrCaseCmp16(const CHAR16 *a, const CHAR16 *b){
    while(*a && *b){ CHAR16 ca=ToLower16(*a), cb=ToLower16(*b); if(ca!=cb) return (int)ca-(int)cb; a++; b++; }
    return (int)*a-(int)*b;
}
static void Trim16(CHAR16 *s){
    UINTN len=StrLen16(s), start=0;
    while(s[start]==L' '||s[start]==L'\t') start++;
    if(start){ UINTN j=0; for(UINTN i=start; s[i]; i++,j++) s[j]=s[i]; s[j]=0; len=j; } else len=StrLen16(s);
    while(len>0 && (s[len-1]==L' '||s[len-1]==L'\t')) s[--len]=0;
}
static BOOLEAN StartsWithI(const CHAR16 *str, const CHAR16 *pre){
    UINTN pl=StrLen16(pre); if(StrLen16(str)<pl) return FALSE;
    for(UINTN i=0;i<pl;i++) if(ToLower16(str[i])!=ToLower16(pre[i])) return FALSE;
    return TRUE;
}
static CHAR16* FindChar16(CHAR16 *s, CHAR16 c){ while(*s){ if(*s==c) return s; s++; } return 0; }
static UINTN UnicodeToAscii(const CHAR16 *src, CHAR8 *dst, UINTN dstSize){
    UINTN i=0; while(src[i] && i+1<dstSize){ dst[i]=(CHAR8)(src[i]&0xFF); i++; } dst[i]=0; return i;
}

/* Output */
static void ConWrite(const CHAR16 *s){
    gST->ConOut->OutputString(gST->ConOut, (CHAR16*)s);
}
static void PrintChar(CHAR16 c){
    CHAR16 s[2]={c,0}; ConWrite(s);
}
static void PrintStr(const CHAR16 *s){ ConWrite(s); }
static void PrintUint(UINT64 v){
    CHAR16 buf[32]; int i=0;
    if(v==0){ PrintChar(L'0'); return; }
    CHAR16 tmp[32]; int pos=0;
    while(v>0){ tmp[pos++] = (CHAR16)(L'0' + (v%10)); v/=10; }
    for(int j=pos-1;j>=0;j--) PrintChar(tmp[j]);
}
static void PrintHex(UINT64 v){
    CHAR16 *hex=L"0123456789ABCDEF";
    CHAR16 buf[17]; buf[16]=0;
    for(int i=15;i>=0;i--){ buf[i]=hex[v&0xF]; v>>=4; }
    // trim leading zeros
    int s=0; while(s<15 && buf[s]==L'0') s++;
    PrintStr(buf+s);
}
static void PrintStatus(EFI_STATUS s){
    PrintStr(L"0x"); PrintHex(s);
}

/* File System helpers */
static CHAR16 *StatusToStr(EFI_STATUS s){
    if(s==EFI_SUCCESS) return L"Success";
    return L"Error";
}

static EFI_STATUS OpenRoot(EFI_HANDLE ImageHandle){
    EFI_STATUS Status;
    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage=0;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs=0;
    Status = gBS->HandleProtocol(ImageHandle, (EFI_GUID*)&gEfiLoadedImageProtocolGuid, (VOID**)&LoadedImage);
    if(!EFI_ERROR(Status) && LoadedImage){
        Status = gBS->HandleProtocol(LoadedImage->DeviceHandle, (EFI_GUID*)&gEfiSimpleFileSystemProtocolGuid, (VOID**)&Fs);
        if(!EFI_ERROR(Status)){
            Status = Fs->OpenVolume(Fs, &Root);
            if(!EFI_ERROR(Status)) return EFI_SUCCESS;
        }
    }
    // fallback: locate any FS
    EFI_HANDLE *Handles=0; UINTN Count=0;
    Status = gBS->LocateHandleBuffer(ByProtocol, (EFI_GUID*)&gEfiSimpleFileSystemProtocolGuid, 0, &Count, &Handles);
    if(EFI_ERROR(Status) || Count==0) return Status;
    Status = gBS->HandleProtocol(Handles[0], (EFI_GUID*)&gEfiSimpleFileSystemProtocolGuid, (VOID**)&Fs);
    if(!EFI_ERROR(Status)){
        Status = Fs->OpenVolume(Fs, &Root);
    }
    gBS->FreePool(Handles);
    return Status;
}

/* Commands */
static void CmdLs(CHAR16 *args){
    EFI_STATUS Status;
    EFI_FILE_PROTOCOL *Dir = Root;
    EFI_FILE_PROTOCOL *Target=0;
    BOOLEAN needClose=FALSE;
    Trim16(args);
    if(args[0]!=0){
        Status = Root->Open(Root, &Target, args, EFI_FILE_MODE_READ, 0);
        if(EFI_ERROR(Status)){
            PrintStr(L"ls: cannot access '"); PrintStr(args); PrintStr(L"': "); PrintStatus(Status); PrintStr(L"\r\n");
            return;
        }
        Dir=Target; needClose=TRUE;
    }
    UINTN BufSize=4096;
    VOID *Buf=0;
    Status = gBS->AllocatePool(EfiLoaderData, BufSize, &Buf);
    if(EFI_ERROR(Status) || !Buf){ PrintStr(L"ls: out of memory\r\n"); if(needClose) Dir->Close(Dir); return; }
    while(1){
        UINTN ReadSize=BufSize;
        Status = Dir->Read(Dir, &ReadSize, Buf);
        if(EFI_ERROR(Status)){ PrintStr(L"ls: read error "); PrintStatus(Status); PrintStr(L"\r\n"); break; }
        if(ReadSize==0) break;
        EFI_FILE_INFO *info=(EFI_FILE_INFO*)Buf;
        BOOLEAN isDir = (info->Attribute & EFI_FILE_DIRECTORY)!=0;
        if(isDir){
            gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_CYAN, EFI_BLACK));
            PrintStr(L"  <DIR>  "); PrintStr(info->FileName); PrintStr(L"\r\n");
            gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
        } else {
            PrintStr(L"  "); PrintUint(info->FileSize); PrintStr(L"  "); PrintStr(info->FileName); PrintStr(L"\r\n");
        }
    }
    gBS->FreePool(Buf);
    if(needClose) Dir->Close(Dir);
}

static void CmdEcho(CHAR16 *rest){
    CHAR16 *redir = FindChar16(rest, L'>');
    if(redir){
        *redir=0;
        CHAR16 *text=rest;
        CHAR16 *filename=redir+1;
        Trim16(text); Trim16(filename);
        if(filename[0]==0){ PrintStr(L"echo: missing filename after '>'\r\n"); return; }
        if(filename[0]==L'>') filename++;
        Trim16(filename);
        UINTN flen=StrLen16(filename);
        if(flen>=2 && ((filename[0]==L'"'&&filename[flen-1]==L'"')||(filename[0]==L'\''&&filename[flen-1]==L'\''))){
            filename[flen-1]=0; filename++;
        }
        EFI_FILE_PROTOCOL *File=0;
        EFI_STATUS Status = Root->Open(Root, &File, filename, EFI_FILE_MODE_CREATE|EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE, 0);
        if(EFI_ERROR(Status)){ PrintStr(L"echo: cannot create '"); PrintStr(filename); PrintStr(L"': "); PrintStatus(Status); PrintStr(L"\r\n"); return; }
        CHAR8 AsciiBuf[1024];
        if(text[0]!=0){
            UINTN AsciiLen = UnicodeToAscii(text, AsciiBuf, sizeof(AsciiBuf));
            UINTN WriteSize=AsciiLen;
            File->Write(File, &WriteSize, AsciiBuf);
        }
        File->Close(File);
    } else {
        Trim16(rest);
        PrintStr(rest); PrintStr(L"\r\n");
    }
}

static void CmdCat(CHAR16 *args){
    Trim16(args);
    if(args[0]==0){ PrintStr(L"cat: missing file name\r\n"); return; }
    EFI_FILE_PROTOCOL *File=0;
    EFI_STATUS Status = Root->Open(Root, &File, args, EFI_FILE_MODE_READ, 0);
    if(EFI_ERROR(Status)){ PrintStr(L"cat: cannot open '"); PrintStr(args); PrintStr(L"': "); PrintStatus(Status); PrintStr(L"\r\n"); return; }
    CHAR8 Buf[512];
    while(1){
        UINTN Size=sizeof(Buf)-1;
        Status = File->Read(File, &Size, Buf);
        if(EFI_ERROR(Status) || Size==0) break;
        Buf[Size]=0;
        // convert ascii to CHAR16 for OutputString
        CHAR16 WBuf[513];
        for(UINTN i=0;i<Size && i<512;i++){ WBuf[i]=(CHAR16)(Buf[i] & 0xFF); } WBuf[Size]=0;
        // handle \n -> \r\n ? just print raw; UEFI expects \r\n but file with \n will show
        ConWrite(WBuf);
    }
    PrintStr(L"\r\n");
    File->Close(File);
}

static void DoTree(EFI_FILE_PROTOCOL *Dir, int depth, BOOLEAN *lastStack) {
    // Collect all entries first to know which is last (for └── vs ├──)
    UINTN bufSize = 4096;
    VOID *buf = NULL;
    if (EFI_ERROR(gBS->AllocatePool(EfiLoaderData, bufSize, &buf)) || !buf) return;
    Dir->SetPosition(Dir, 0);

    EFI_FILE_INFO **infos = NULL;
    UINTN count = 0, cap = 32;
    if (EFI_ERROR(gBS->AllocatePool(EfiLoaderData, cap * sizeof(EFI_FILE_INFO*), (VOID**)&infos)) || !infos) {
        gBS->FreePool(buf);
        return;
    }
    while (1) {
        UINTN readSize = bufSize;
        EFI_STATUS st = Dir->Read(Dir, &readSize, buf);
        if (EFI_ERROR(st) || readSize == 0) break;
        EFI_FILE_INFO *fi = (EFI_FILE_INFO*)buf;
        if (StrCmp16(fi->FileName, L".") == 0 || StrCmp16(fi->FileName, L"..") == 0) continue;
        EFI_FILE_INFO *copy = NULL;
        if (EFI_ERROR(gBS->AllocatePool(EfiLoaderData, readSize, (VOID**)&copy)) || !copy) continue;
        gBS->CopyMem(copy, buf, readSize);
        if (count >= cap) {
            UINTN newCap = cap * 2;
            EFI_FILE_INFO **newInfos = NULL;
            if (EFI_ERROR(gBS->AllocatePool(EfiLoaderData, newCap * sizeof(EFI_FILE_INFO*), (VOID**)&newInfos)) || !newInfos) {
                gBS->FreePool(copy);
                break;
            }
            gBS->CopyMem(newInfos, infos, count * sizeof(EFI_FILE_INFO*));
            gBS->FreePool(infos);
            infos = newInfos;
            cap = newCap;
        }
        infos[count++] = copy;
    }
    gBS->FreePool(buf);

    for (UINTN i = 0; i < count; i++) {
        EFI_FILE_INFO *info = infos[i];
        BOOLEAN isLast = (i == count - 1);
        for (int d = 0; d < depth; d++) {
            if (lastStack[d]) PrintStr(L"    ");
            else PrintStr(L"\x2502   ");
        }
        if (isLast) PrintStr(L"\x2514\x2500\x2500 ");
        else PrintStr(L"\x251C\x2500\x2500 ");
        PrintStr(info->FileName);
        if (info->Attribute & EFI_FILE_DIRECTORY) {
            PrintStr(L"\r\n");
            EFI_FILE_PROTOCOL *Sub = NULL;
            if (!EFI_ERROR(Dir->Open(Dir, &Sub, info->FileName, EFI_FILE_MODE_READ, 0)) && Sub) {
                BOOLEAN newStack[32];
                for (int d = 0; d < depth; d++) newStack[d] = lastStack[d];
                newStack[depth] = isLast;
                if (depth < 32) {
                    Sub->SetPosition(Sub, 0);
                    DoTree(Sub, depth + 1, newStack);
                }
                Sub->Close(Sub);
            }
        } else {
            PrintStr(L"\r\n");
        }
        gBS->FreePool(info);
    }
    gBS->FreePool(infos);
    Dir->SetPosition(Dir, 0);
}

static void CmdTree(CHAR16 *args) {
    Trim16(args);
    EFI_FILE_PROTOCOL *Start = Root;
    BOOLEAN needClose = FALSE;
    CHAR16 *display = L"C:\\";
    if (args[0] != 0) {
        EFI_STATUS st = Root->Open(Root, &Start, args, EFI_FILE_MODE_READ, 0);
        if (EFI_ERROR(st)) {
            PrintStr(L"tree: cannot access '"); PrintStr(args); PrintStr(L"': "); PrintStatus(st); PrintStr(L"\r\n");
            return;
        }
        // If it's a file, just print it
        UINTN s = 4096; VOID *b = NULL;
        if (!EFI_ERROR(gBS->AllocatePool(EfiLoaderData, s, &b)) && b) {
            EFI_STATUS s2 = Start->GetInfo(Start, (EFI_GUID*)&gEfiFileInfoGuid, &s, b);
            if (!EFI_ERROR(s2)) {
                EFI_FILE_INFO *fi = (EFI_FILE_INFO*)b;
                if (!(fi->Attribute & EFI_FILE_DIRECTORY)) {
                    PrintStr(args); PrintStr(L"\r\n");
                    gBS->FreePool(b);
                    Start->Close(Start);
                    return;
                }
            }
            gBS->FreePool(b);
        }
        display = args;
        needClose = TRUE;
    }
    PrintStr(display); PrintStr(L"\r\n");
    BOOLEAN stack[32] = {0};
    DoTree(Start, 0, stack);
    if (needClose) Start->Close(Start);
    else Start->SetPosition(Start, 0);
}

// ---------- Boot Menu ----------
#define BOOT_ENTRY_MAX 32
#define BOOT_DESC_MAX 128
typedef struct {
    CHAR16 Desc[BOOT_DESC_MAX];
    CHAR16 FilePath[260];
    EFI_DEVICE_PATH_PROTOCOL *DevPath;
    EFI_HANDLE DevHandle;
    BOOLEAN IsBootOption;
    UINT16 BootNum;
} BOOT_ENTRY;

static EFI_DEVICE_PATH_PROTOCOL* DupDevicePath(EFI_DEVICE_PATH_PROTOCOL *Path) {
    if (!Path) return NULL;
    UINTN size = 0;
    EFI_DEVICE_PATH_PROTOCOL *p = Path;
    while (p) {
        UINTN len = p->Length[0] | (p->Length[1] << 8);
        size += len;
        if (p->Type == 0x7F && p->SubType == 0xFF) break;
        p = (EFI_DEVICE_PATH_PROTOCOL*)((UINT8*)p + len);
    }
    VOID *copy = NULL;
    if (EFI_ERROR(gBS->AllocatePool(EfiLoaderData, size, &copy)) || !copy) return NULL;
    gBS->CopyMem(copy, Path, size);
    return (EFI_DEVICE_PATH_PROTOCOL*)copy;
}

static EFI_DEVICE_PATH_PROTOCOL* CreateFileDevicePath(EFI_DEVICE_PATH_PROTOCOL *Base, CHAR16 *FilePath) {
    if (!Base || !FilePath) return NULL;
    UINTN baseSize = 0;
    EFI_DEVICE_PATH_PROTOCOL *p = Base;
    while (p) {
        UINTN len = p->Length[0] | (p->Length[1] << 8);
        if (p->Type == 0x7F && p->SubType == 0xFF) break;
        baseSize += len;
        p = (EFI_DEVICE_PATH_PROTOCOL*)((UINT8*)p + len);
    }
    UINTN fileLen = (StrLen16(FilePath) + 1) * 2 + 4;
    UINTN total = baseSize + fileLen + 4;
    VOID *newPath = NULL;
    if (EFI_ERROR(gBS->AllocatePool(EfiLoaderData, total, &newPath)) || !newPath) return NULL;
    gBS->CopyMem(newPath, Base, baseSize);
    EFI_DEVICE_PATH_PROTOCOL *fp = (EFI_DEVICE_PATH_PROTOCOL*)((UINT8*)newPath + baseSize);
    fp->Type = 0x04;
    fp->SubType = 0x04;
    fp->Length[0] = (UINT8)(fileLen & 0xFF);
    fp->Length[1] = (UINT8)((fileLen >> 8) & 0xFF);
    CHAR16 *dst = (CHAR16*)((UINT8*)fp + 4);
    StrCpy16(dst, FilePath);
    EFI_DEVICE_PATH_PROTOCOL *end = (EFI_DEVICE_PATH_PROTOCOL*)((UINT8*)fp + fileLen);
    end->Type = 0x7F;
    end->SubType = 0xFF;
    end->Length[0] = 4;
    end->Length[1] = 0;
    return (EFI_DEVICE_PATH_PROTOCOL*)newPath;
}

static CHAR16* GetFilePathFromDevicePath(EFI_DEVICE_PATH_PROTOCOL *dp) {
    if (!dp) return NULL;
    EFI_DEVICE_PATH_PROTOCOL *p = dp;
    CHAR16 *last = NULL;
    while (p) {
        UINTN len = p->Length[0] | (p->Length[1] << 8);
        if (p->Type == 0x7F && p->SubType == 0xFF) break;
        if (p->Type == 0x04 && p->SubType == 0x04) {
            last = (CHAR16*)((UINT8*)p + 4);
        }
        p = (EFI_DEVICE_PATH_PROTOCOL*)((UINT8*)p + len);
    }
    return last;
}

static EFI_STATUS LoadImageFromFileFallback(EFI_HANDLE ParentImage, EFI_HANDLE DevHandle, CHAR16 *FilePath, EFI_DEVICE_PATH_PROTOCOL *Dp, EFI_HANDLE *NewImage) {
    CHAR16 *fp = FilePath;
    EFI_HANDLE h = DevHandle;
    if ((!fp || !fp[0]) && Dp) fp = GetFilePathFromDevicePath(Dp);
    if (!fp || !fp[0]) return EFI_NOT_FOUND;
    // Try DevHandle first, then fallback to scanning all FS handles (fixes Boot#### on some firmwares where LocateDevicePath fails)
    EFI_STATUS st2;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs=NULL;
    EFI_FILE_PROTOCOL *vol=NULL;
    EFI_FILE_PROTOCOL *file=NULL;
    // 1. Try specific DevHandle if provided
    if (h) {
        if (!EFI_ERROR(gBS->HandleProtocol(h, (EFI_GUID*)&gEfiSimpleFileSystemProtocolGuid, (VOID**)&fs)) && fs) {
            if (!EFI_ERROR(fs->OpenVolume(fs, &vol)) && vol) {
                EFI_STATUS st = vol->Open(vol, &file, fp, EFI_FILE_MODE_READ, 0);
                if (!EFI_ERROR(st) && file) goto found;
                if (vol) { vol->Close(vol); vol=NULL; }
            }
        }
        fs=NULL; vol=NULL; file=NULL;
    }
    // 2. Try LocateDevicePath
    if (Dp) {
        EFI_DEVICE_PATH_PROTOCOL *tmp = Dp;
        EFI_HANDLE h2=NULL;
        if (!EFI_ERROR(gBS->LocateDevicePath((EFI_GUID*)&gEfiSimpleFileSystemProtocolGuid, &tmp, &h2)) && h2) {
            h = h2;
            if (!EFI_ERROR(gBS->HandleProtocol(h, (EFI_GUID*)&gEfiSimpleFileSystemProtocolGuid, (VOID**)&fs)) && fs) {
                if (!EFI_ERROR(fs->OpenVolume(fs, &vol)) && vol) {
                    EFI_STATUS st = vol->Open(vol, &file, fp, EFI_FILE_MODE_READ, 0);
                    if (!EFI_ERROR(st) && file) goto found;
                    if (vol) { vol->Close(vol); vol=NULL; }
                }
            }
            fs=NULL; vol=NULL; file=NULL;
        }
    }
    // 3. Brute force: try every SimpleFileSystem handle with this FilePath (fixes Windows Boot Manager/HackBGRT)
    {
        EFI_HANDLE *handles=NULL; UINTN hcnt=0;
        if (!EFI_ERROR(gBS->LocateHandleBuffer(ByProtocol, (EFI_GUID*)&gEfiSimpleFileSystemProtocolGuid, NULL, &hcnt, &handles)) && handles) {
            for (UINTN i=0;i<hcnt;i++) {
                EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs2=NULL;
                if (EFI_ERROR(gBS->HandleProtocol(handles[i], (EFI_GUID*)&gEfiSimpleFileSystemProtocolGuid, (VOID**)&fs2))||!fs2) continue;
                EFI_FILE_PROTOCOL *vol2=NULL;
                if (EFI_ERROR(fs2->OpenVolume(fs2, &vol2))||!vol2) continue;
                EFI_FILE_PROTOCOL *f2=NULL;
                if (!EFI_ERROR(vol2->Open(vol2, &f2, fp, EFI_FILE_MODE_READ, 0)) && f2) {
                    // Found
                    fs = fs2; vol = vol2; file = f2;
                    // Need to keep vol open for later? We'll use this one
                    // Close other vols we opened? We already closed previous attempts. Keep this.
                    // Need to free handles array but keep vol/file
                    gBS->FreePool(handles);
                    goto found;
                }
                vol2->Close(vol2);
            }
            gBS->FreePool(handles);
        }
        return EFI_NOT_FOUND;
    }
found:;
    // Now vol and file are open, proceed to read
    EFI_STATUS st;
    // Get file size
    UINTN infoSize = sizeof(EFI_FILE_INFO) + 512;
    VOID *infoBuf=NULL;
    if (EFI_ERROR(gBS->AllocatePool(EfiLoaderData, infoSize, &infoBuf)) || !infoBuf) { file->Close(file); vol->Close(vol); return EFI_OUT_OF_RESOURCES; }
    EFI_FILE_INFO *info = (EFI_FILE_INFO*)infoBuf;
    infoSize = 512 + sizeof(EFI_FILE_INFO);
    // Use GetInfo
    st = file->GetInfo(file, (EFI_GUID*)&gEfiFileInfoGuid, &infoSize, info);
    if (EFI_ERROR(st)) {
        // Try with larger buffer
        gBS->FreePool(infoBuf);
        infoSize = 4096;
        if (EFI_ERROR(gBS->AllocatePool(EfiLoaderData, infoSize, &infoBuf)) || !infoBuf) { file->Close(file); vol->Close(vol); return st; }
        st = file->GetInfo(file, (EFI_GUID*)&gEfiFileInfoGuid, &infoSize, infoBuf);
        if (EFI_ERROR(st)) { gBS->FreePool(infoBuf); file->Close(file); vol->Close(vol); return st; }
        info = (EFI_FILE_INFO*)infoBuf;
    }
    UINTN fileSize = (UINTN)info->FileSize;
    gBS->FreePool(infoBuf);
    if (fileSize==0 || fileSize> (64*1024*1024)) { file->Close(file); vol->Close(vol); return EFI_LOAD_ERROR; }
    VOID *fileBuf=NULL;
    if (EFI_ERROR(gBS->AllocatePool(EfiLoaderData, fileSize, &fileBuf)) || !fileBuf) { file->Close(file); vol->Close(vol); return EFI_OUT_OF_RESOURCES; }
    file->SetPosition(file, 0);
    UINTN readSize = fileSize;
    st = file->Read(file, &readSize, fileBuf);
    file->Close(file);
    vol->Close(vol);
    if (EFI_ERROR(st) || readSize != fileSize) { gBS->FreePool(fileBuf); return EFI_DEVICE_ERROR; }
    // LoadImage via SourceBuffer
    st = gBS->LoadImage(FALSE, ParentImage, NULL, fileBuf, fileSize, NewImage);
    gBS->FreePool(fileBuf);
    return st;
}

static void GetDriveDescription(EFI_HANDLE h, UINTN idx, CHAR16 *out, UINTN outSize) {
    out[0]=0;
    // Try to get file system volume label
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs=NULL;
    if (!EFI_ERROR(gBS->HandleProtocol(h, (EFI_GUID*)&gEfiSimpleFileSystemProtocolGuid, (VOID**)&fs)) && fs) {
        EFI_FILE_PROTOCOL *vol=NULL;
        if (!EFI_ERROR(fs->OpenVolume(fs, &vol)) && vol) {
            // Try FileSystemVolumeLabelInfo - GUID DB871A2B-...? Use FileSystemInfo
            // Use EFI_FILE_SYSTEM_INFO GUID 09576E93-...
            EFI_GUID fsInfoGuid = {0x09576E93, 0x6D3F, 0x11D2, {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};
            UINTN sz = 1024;
            VOID *buf=NULL;
            if (!EFI_ERROR(gBS->AllocatePool(EfiLoaderData, sz, &buf)) && buf) {
                EFI_STATUS st = vol->GetInfo(vol, &fsInfoGuid, &sz, buf);
                if (!EFI_ERROR(st) && sz >= 40) {
                    // EFI_FILE_SYSTEM_INFO: VolumeLabel is at offset 36+?
                    // Structure: Size, ReadOnly, VolumeSize, FreeSpace, BlockSize, VolumeLabel[1]
                    // VolumeLabel is CHAR16* at offset 36 (after 4+1+8+8+4)
                    CHAR16 *label = (CHAR16*)((UINT8*)buf + 36);
                    if (label[0]) {
                        // Use label
                        StrCpy16(out, label);
                        // Append drive index
                        UINTN l = StrLen16(out);
                        if (l+10 < outSize) {
                            out[l]=L' '; out[l+1]=L'(' ; out[l+2]=L'F'; out[l+3]=L'S'; 
                            out[l+4]=L'0'+ (CHAR16)(idx%10); out[l+5]=L')'; out[l+6]=0;
                        }
                        gBS->FreePool(buf);
                        vol->Close(vol);
                        return;
                    }
                }
                gBS->FreePool(buf);
            }
            vol->Close(vol);
        }
    }
    // Fallback: try DevicePathToText if available
    EFI_DEVICE_PATH_PROTOCOL *dp=NULL;
    if (!EFI_ERROR(gBS->HandleProtocol(h, (EFI_GUID*)&gEfiDevicePathProtocolGuid, (VOID**)&dp)) && dp) {
        // Try to find HD node and use its partition info
        EFI_DEVICE_PATH_PROTOCOL *p = dp;
        while (p) {
            UINTN len = p->Length[0] | (p->Length[1] << 8);
            if (p->Type == 0x04 && p->SubType == 0x01) { // MEDIA_HARDDRIVE_DP
                // Hard drive node: contains partition info, but we can just use "HD"
                StrCpy16(out, L"HD");
                break;
            }
            if (p->Type == 0x7F && p->SubType == 0xFF) break;
            p = (EFI_DEVICE_PATH_PROTOCOL*)((UINT8*)p + len);
        }
        if (out[0]==0) StrCpy16(out, L"Drive");
        UINTN l = StrLen16(out);
        if (l+10 < outSize) {
            out[l]=L' '; out[l+1]=L'0'+(CHAR16)(idx%10); out[l+2]=0;
        }
        return;
    }
    // Ultimate fallback: Drive N
    StrCpy16(out, L"Drive ");
    out[6]=L'0'+(CHAR16)(idx%10); out[7]=0;
}

static void FreeBootEntries(BOOT_ENTRY *entries, UINTN count) {
    for (UINTN i=0;i<count;i++) {
        if (entries[i].DevPath) gBS->FreePool(entries[i].DevPath);
    }
}

static UINTN CollectBootEntries(BOOT_ENTRY *entries, UINTN max) {
    UINTN cnt = 0;
    UINTN orderSize = 0;
    EFI_STATUS st = gRT->GetVariable(L"BootOrder", (EFI_GUID*)&gEfiGlobalVariableGuid, NULL, &orderSize, NULL);
    if (st == EFIERR(5) || st == 0x8000000000000005ULL) {
        VOID *orderBuf = NULL;
        if (!EFI_ERROR(gBS->AllocatePool(EfiLoaderData, orderSize, &orderBuf)) && orderBuf) {
            UINT32 attr=0;
            if (!EFI_ERROR(gRT->GetVariable(L"BootOrder", (EFI_GUID*)&gEfiGlobalVariableGuid, &attr, &orderSize, orderBuf))) {
                UINT16 *order = (UINT16*)orderBuf;
                UINTN num = orderSize / 2;
                for (UINTN i=0;i<num && cnt<max;i++) {
                    CHAR16 name[16];
                    const CHAR16 *hex = L"0123456789ABCDEF";
                    name[0]=L'B'; name[1]=L'o'; name[2]=L'o'; name[3]=L't';
                    name[4]=hex[(order[i]>>12)&0xF]; name[5]=hex[(order[i]>>8)&0xF]; name[6]=hex[(order[i]>>4)&0xF]; name[7]=hex[order[i]&0xF]; name[8]=0;
                    UINTN varSize=0;
                    gRT->GetVariable(name, (EFI_GUID*)&gEfiGlobalVariableGuid, NULL, &varSize, NULL);
                    if (varSize==0 || varSize>4096) continue;
                    VOID *varBuf=NULL;
                    if (EFI_ERROR(gBS->AllocatePool(EfiLoaderData, varSize, &varBuf))||!varBuf) continue;
                    if (EFI_ERROR(gRT->GetVariable(name, (EFI_GUID*)&gEfiGlobalVariableGuid, NULL, &varSize, varBuf))) { gBS->FreePool(varBuf); continue; }
                    if (varSize < 6) { gBS->FreePool(varBuf); continue; }
                    CHAR16 *desc = (CHAR16*)((UINT8*)varBuf+6);
                    UINTN descLen = StrLen16(desc);
                    EFI_DEVICE_PATH_PROTOCOL *dp = (EFI_DEVICE_PATH_PROTOCOL*)((UINT8*)desc + (descLen+1)*2);
                    if ((UINT8*)dp >= (UINT8*)varBuf+varSize) { gBS->FreePool(varBuf); continue; }
                    StrCpy16(entries[cnt].Desc, desc);
                    entries[cnt].DevPath = DupDevicePath(dp);
                    // Store file path and handle for fallback
                    {
                        CHAR16 *fp = GetFilePathFromDevicePath(dp);
                        if (fp) StrCpy16(entries[cnt].FilePath, fp);
                        else entries[cnt].FilePath[0]=0;
                        EFI_DEVICE_PATH_PROTOCOL *tmp2 = dp;
                        EFI_HANDLE h2=NULL;
                        if (!EFI_ERROR(gBS->LocateDevicePath((EFI_GUID*)&gEfiSimpleFileSystemProtocolGuid, &tmp2, &h2))) entries[cnt].DevHandle = h2;
                        else entries[cnt].DevHandle = NULL;
                    }
                    entries[cnt].IsBootOption = TRUE;
                    entries[cnt].BootNum = order[i];
                    cnt++;
                    gBS->FreePool(varBuf);
                    if (cnt>=max) break;
                }
            }
            gBS->FreePool(orderBuf);
        }
    }
    EFI_HANDLE *handles=NULL; UINTN hcnt=0;
    if (!EFI_ERROR(gBS->LocateHandleBuffer(ByProtocol, (EFI_GUID*)&gEfiSimpleFileSystemProtocolGuid, NULL, &hcnt, &handles)) && handles) {
        const CHAR16 *candidates[] = { L"\\EFI\\Boot\\BOOTX64.EFI", L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi", L"\\EFI\\ubuntu\\grubx64.efi", L"\\EFI\\fedora\\grubx64.efi", L"\\EFI\\Boot\\grubx64.efi", NULL };
        for (UINTN h=0; h<hcnt && cnt<max; h++) {
            EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs=NULL;
            if (EFI_ERROR(gBS->HandleProtocol(handles[h], (EFI_GUID*)&gEfiSimpleFileSystemProtocolGuid, (VOID**)&fs))||!fs) continue;
            EFI_FILE_PROTOCOL *vol=NULL;
            if (EFI_ERROR(fs->OpenVolume(fs, &vol))||!vol) continue;
            EFI_DEVICE_PATH_PROTOCOL *devPath=NULL;
            gBS->HandleProtocol(handles[h], (EFI_GUID*)&gEfiDevicePathProtocolGuid, (VOID**)&devPath);
            for (int c=0; candidates[c] && cnt<max; c++) {
                EFI_FILE_PROTOCOL *f=NULL;
                if (!EFI_ERROR(vol->Open(vol, &f, (CHAR16*)candidates[c], EFI_FILE_MODE_READ, 0)) && f) {
                    f->Close(f);
                    {
                        CHAR16 drv[64]; GetDriveDescription(handles[h], h, drv, 64);
                        StrCpy16(entries[cnt].Desc, drv);
                        UINTN dl = StrLen16(entries[cnt].Desc);
                        if (dl+2 < BOOT_DESC_MAX) { entries[cnt].Desc[dl]=L':'; entries[cnt].Desc[dl+1]=L' '; entries[cnt].Desc[dl+2]=0; dl+=2; }
                        // Append candidate file path without leading
                        const CHAR16 *fp = candidates[c]+1;
                        // Ensure fits
                        UINTN fl = StrLen16(fp);
                        if (dl+fl < BOOT_DESC_MAX-1) StrCpy16(entries[cnt].Desc+dl, fp);
                    }
                    entries[cnt].DevPath = devPath ? CreateFileDevicePath(devPath, (CHAR16*)candidates[c]) : NULL;
                    entries[cnt].DevHandle = handles[h];
                    StrCpy16(entries[cnt].FilePath, (CHAR16*)candidates[c]);
                    entries[cnt].IsBootOption = FALSE;
                    entries[cnt].BootNum = 0xFFFF;
                    cnt++;
                }
            }
            vol->Close(vol);
        }
        gBS->FreePool(handles);
    }
    return cnt;
}

static void DrawBootMenu(BOOT_ENTRY *entries, UINTN count, UINTN selected) {
    gST->ConOut->ClearScreen(gST->ConOut);
    gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLUE));
    PrintStr(L"  Boot Menu - Use Up/Down, Enter to boot, ESC to exit                              \r\n");
    gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
    PrintStr(L"  Found "); PrintUint(count); PrintStr(L" boot entries\r\n\r\n");
    for (UINTN i=0;i<count;i++) {
        if (i==selected) {
            gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLUE));
            PrintStr(L"> "); PrintStr(entries[i].Desc);
            PrintStr(L"                              \r\n");
            gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
        } else {
            PrintStr(L"  "); PrintStr(entries[i].Desc); PrintStr(L"\r\n");
        }
    }
    PrintStr(L"\r\n  F11/F12 or Ctrl+/- resize | Enter=boot\r\n");
}

static void CmdBoot(EFI_HANDLE ImageHandle) {
    BOOT_ENTRY *entries = NULL;
    if (EFI_ERROR(gBS->AllocatePool(EfiLoaderData, BOOT_ENTRY_MAX * sizeof(BOOT_ENTRY), (VOID**)&entries)) || !entries) {
        PrintStr(L"No memory for boot menu\r\n");
        return;
    }
    // Zero
    for (UINTN _i=0; _i<BOOT_ENTRY_MAX; _i++) entries[_i].DevPath=NULL;
    UINTN count = CollectBootEntries(entries, BOOT_ENTRY_MAX);
    if (count==0) {
        PrintStr(L"No bootable entries found.\r\n");
        PrintStr(L"Checked BootOrder and \\EFI\\Boot\\BOOTX64.EFI etc. on all FS handles.\r\n");
        return;
    }
    UINTN sel = 0;
    DrawBootMenu(entries, count, sel);
    while (1) {
        UINTN idx;
        EFI_EVENT ev = ConInEx ? ConInEx->WaitForKeyEx : gST->ConIn->WaitForKey;
        gBS->WaitForEvent(1, &ev, &idx);
        EFI_INPUT_KEY Key;
        EFI_KEY_STATE_TMP ks; BOOLEAN hasEx=FALSE;
        if (ConInEx) {
            EFI_KEY_DATA_TMP kd;
            if (EFI_ERROR(ConInEx->ReadKeyStrokeEx(ConInEx, &kd))) continue;
            Key = kd.Key; ks = kd.KeyState; hasEx=TRUE;
        } else {
            if (EFI_ERROR(gST->ConIn->ReadKeyStroke(gST->ConIn, &Key))) continue;
        }
        if (Key.ScanCode == SCAN_UP) {
            sel = (sel==0? count-1 : sel-1);
            DrawBootMenu(entries, count, sel);
        } else if (Key.ScanCode == SCAN_DOWN) {
            sel = (sel+1)%count;
            DrawBootMenu(entries, count, sel);
        } else if (Key.ScanCode == SCAN_ESC) {
            gST->ConOut->ClearScreen(gST->ConOut);
            break;
        } else if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
            gST->ConOut->ClearScreen(gST->ConOut);
            PrintStr(L"Booting: "); PrintStr(entries[sel].Desc); PrintStr(L"\r\n");
            EFI_DEVICE_PATH_PROTOCOL *dp = entries[sel].DevPath;
            if (!dp) {
                PrintStr(L"No device path.\r\n");
                gBS->Stall(2000000);
                DrawBootMenu(entries, count, sel);
                continue;
            }
            EFI_HANDLE newImg=NULL;
            EFI_STATUS st = gBS->LoadImage(FALSE, ImageHandle, dp, NULL, 0, &newImg);
            if (EFI_ERROR(st) && st == EFI_NOT_FOUND) {
                // Fallback: try file read + LoadImage via SourceBuffer (fixes 0xE on some firmwares)
                PrintStr(L"LoadImage via DP failed, trying file fallback...\r\n");
                st = LoadImageFromFileFallback(ImageHandle, entries[sel].DevHandle, entries[sel].FilePath, dp, &newImg);
            }
            if (EFI_ERROR(st)) {
                PrintStr(L"LoadImage failed: "); PrintStatus(st); PrintStr(L" (0x"); PrintHex(st); PrintStr(L")\r\n");
                if (st == EFI_NOT_FOUND) PrintStr(L"File not found - check EFI path and ESP\r\n");
                gBS->Stall(3000000);
                DrawBootMenu(entries, count, sel);
                continue;
            }
            st = gBS->StartImage(newImg, NULL, NULL);
            PrintStr(L"StartImage returned: "); PrintStatus(st); PrintStr(L"\r\n");
            gBS->Stall(3000000);
            DrawBootMenu(entries, count, sel);
        } else if (Key.ScanCode == SCAN_F11) { ResizeConsole(FALSE); DrawBootMenu(entries, count, sel); }
        else if (Key.ScanCode == SCAN_F12) { ResizeConsole(TRUE); DrawBootMenu(entries, count, sel); }
        else if (hasEx && IsCtrlPressed(&ks) && (Key.UnicodeChar==L'+'||Key.UnicodeChar==L'='||Key.UnicodeChar==L'*')) { ResizeConsole(TRUE); DrawBootMenu(entries, count, sel); }
    }
    FreeBootEntries(entries, count);
    gBS->FreePool(entries);
    gST->ConOut->ClearScreen(gST->ConOut);
}

static void PrintHelp(void){
    gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_GREEN, EFI_BLACK));
    PrintStr(L"\r\nwkg2673's UEFI Shell - commands:\r\n");
    PrintStr(L"  hello                - respond with Hello!\r\n");
    PrintStr(L"  ls / dir [path]      - list files\r\n");
    PrintStr(L"  tree [path]          - display file tree (like Windows tree)\r\n");
    PrintStr(L"  boot                   - boot menu (all drives, Up/Down Enter)\r\n");
    PrintStr(L"  echo <text>          - print text\r\n");
    PrintStr(L"  echo <text> > <file> - create file\r\n");
    PrintStr(L"  cat/type <file>      - show file\r\n");
    PrintStr(L"  clear / cls          - clear screen\r\n");
    PrintStr(L"  help / ?             - show help\r\n");
    PrintStr(L"  layout [de/us/auto] - keyboard layout (German QWERTZ vs US QWERTY)\r\n");
    PrintStr(L"  exit                 - exit shell\r\n");
    PrintStr(L"  shutdown/shtdown/off - power off\r\n");
    PrintStr(L"  reboot/reset/restart - reboot\r\n");
    PrintStr(L"\r\nwkg2673's UEFI Shell - controls:\r\n");
    PrintStr(L"  Ctrl +               - cycle console sizes\r\n\r\n");
}

static UINTN ReadLine(CHAR16 *Buffer, UINTN MaxLen){
    UINTN idx=0;
    for(UINTN i=0;i<MaxLen;i++) Buffer[i]=0;
    // Try to locate Ex protocol for Ctrl detection (once)
    if (!ConInEx) {
        // ConsoleInHandle is gST->ConsoleInHandle
        EFI_STATUS st2 = gBS->HandleProtocol(gST->ConsoleInHandle, (EFI_GUID*)&gEfiSimpleTextInputExProtocolGuid, (VOID**)&ConInEx);
        if (EFI_ERROR(st2)) ConInEx = NULL;
    }
    while(1){
        UINTN Index;
        EFI_INPUT_KEY Key;
        EFI_KEY_STATE_TMP KeyStateTmp;
        BOOLEAN hasEx = FALSE;
        EFI_STATUS Status;
        if (ConInEx) {
            // Use Ex for better Ctrl detection
            gBS->WaitForEvent(1, &ConInEx->WaitForKeyEx, &Index);
            EFI_KEY_DATA_TMP KeyData;
            Status = ConInEx->ReadKeyStrokeEx(ConInEx, &KeyData);
            if(EFI_ERROR(Status)) continue;
            Key = KeyData.Key;
            KeyStateTmp = KeyData.KeyState;
            hasEx = TRUE;
            // Translate after getting raw US char
            if (UseGermanLayout && Key.UnicodeChar != 0) {
                Key.UnicodeChar = TranslateGermanChar(Key.UnicodeChar);
            }
            // Check for Ctrl+Plus/Minus or F11/F12 resize (like Windows CMD Ctrl+ +/-)
            if (hasEx && IsCtrlPressed(&KeyStateTmp)) {
                if (Key.UnicodeChar == L'+' || Key.UnicodeChar == L'=' || Key.UnicodeChar == L'*') {
                    ResizeConsole(TRUE);
                    // Reprint prompt+buffer after mode change (screen cleared)
                    gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_GREEN, EFI_BLACK));
                    PrintStr(L"C:\\");
                    gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
                    PrintStr(L"> ");
                    if (idx>0) {
                        CHAR16 tmp[MAX_CMD_LEN];
                        for(UINTN i=0;i<idx && i<MAX_CMD_LEN-1;i++) tmp[i]=Buffer[i];
                        tmp[idx]=0;
                        PrintStr(tmp);
                    }
                    continue;
                }
            }
            // F11/F12 always resize even without Ctrl (fallback for simple)
            if (Key.ScanCode == SCAN_F11) { ResizeConsole(FALSE); continue; }
            if (Key.ScanCode == SCAN_F12) { ResizeConsole(TRUE); continue; }
            // Also handle +/- without Ctrl when at empty prompt as fallback (if Ex not available, Ctrl detection fails)
            // For Ex, only handle Ctrl+Plus, not plain +/- (so user can type + normally)
        } else {
            gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &Index);
            Status = gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
            if(EFI_ERROR(Status)) continue;
            if (UseGermanLayout && Key.UnicodeChar != 0) {
                Key.UnicodeChar = TranslateGermanChar(Key.UnicodeChar);
            }
            // Simple fallback: F11/F12 resize, and +/- at empty line resizes (so + can still be typed mid-command)
            if (Key.ScanCode == SCAN_F11) { ResizeConsole(FALSE); 
                gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_GREEN, EFI_BLACK));
                PrintStr(L"C:\\"); gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK)); PrintStr(L"> ");
                if(idx>0){ CHAR16 tmp[MAX_CMD_LEN]; for(UINTN i=0;i<idx;i++) tmp[i]=Buffer[i]; tmp[idx]=0; PrintStr(tmp); }
                continue; }
            if (Key.ScanCode == SCAN_F12) { ResizeConsole(TRUE);
                gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_GREEN, EFI_BLACK));
                PrintStr(L"C:\\"); gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK)); PrintStr(L"> ");
                if(idx>0){ CHAR16 tmp[MAX_CMD_LEN]; for(UINTN i=0;i<idx;i++) tmp[i]=Buffer[i]; tmp[idx]=0; PrintStr(tmp); }
                continue; }
            if (idx==0 && (Key.UnicodeChar==L'+' || Key.UnicodeChar==L'-')) {
                // At empty prompt, +/- resizes (otherwise insert normally so echo a+b works)
                if (Key.UnicodeChar==L'+') ResizeConsole(TRUE); else ResizeConsole(FALSE);
                // After resize, reprint prompt
                gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_GREEN, EFI_BLACK));
                PrintStr(L"C:\\"); gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK)); PrintStr(L"> ");
                continue;
            }
        }
        if(Key.UnicodeChar==CHAR_CARRIAGE_RETURN){
            PrintStr(L"\r\n");
            break;
        } else if(Key.UnicodeChar==CHAR_BACKSPACE || Key.UnicodeChar==0x7F){
            if(idx>0){
                idx--; Buffer[idx]=0;
                PrintStr(L"\b \b");
            }
        } else if(Key.ScanCode==SCAN_ESC){
            while(idx>0){ idx--; PrintStr(L"\b \b"); }
            Buffer[0]=0;
        } else if(Key.UnicodeChar>=32){
            if(idx < MaxLen-1){
                Buffer[idx++]=Key.UnicodeChar;
                Buffer[idx]=0;
                CHAR16 s[2]={Key.UnicodeChar,0};
                PrintStr(s);
            }
        }
    }
    return idx;
}

/* Entry */
EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable){
    gST=SystemTable;
    gBS=SystemTable->BootServices;
    gRT=SystemTable->RuntimeServices;

    gST->ConOut->ClearScreen(gST->ConOut);
    gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_GREEN, EFI_BLACK));
    PrintStr(L"██╗    ██╗██╗  ██╗ ██████╗ ██████╗  ██████╗███████╗██████╗ \r\n");
    PrintStr(L"██║    ██║██║ ██╔╝██╔════╝ ╚════██╗██╔════╝╚════██║╚════██╗\r\n");
    PrintStr(L"██║ █╗ ██║█████╔╝ ██║  ███╗ █████╔╝███████╗    ██╔╝ █████╔╝\r\n");
    PrintStr(L"██║███╗██║██╔═██╗ ██║   ██║██╔═══╝ ██╔═══██╗  ██╔╝  ╚═══██╗\r\n");
    PrintStr(L"╚███╔███╔╝██║  ██╗╚██████╔╝███████╗╚██████╔╝  ██║  ██████╔╝\r\n");
    PrintStr(L" ╚══╝╚══╝ ╚═╝  ╚═╝ ╚═════╝ ╚══════╝ ╚═════╝   ╚═╝  ╚═════╝\r\n");
    PrintStr(L"\r\nwkg2673's UEFI Shell v1.0\r\n");
    gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
    PrintHelp();
    DetectKeyboardLayout();
    PrintKeyboardInfo();
    PrintStr(L"\r\n");

    EFI_STATUS Status = OpenRoot(ImageHandle);
    if(EFI_ERROR(Status)){
        PrintStr(L"Warning: filesystem not available, ls/echo > will fail: "); PrintStatus(Status); PrintStr(L"\r\n");
    }

    CHAR16 CmdLine[MAX_CMD_LEN];
    while(1){
        gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_GREEN, EFI_BLACK));
        PrintStr(L"C:\\");
        gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
        PrintStr(L"> ");
        ReadLine(CmdLine, MAX_CMD_LEN);
        Trim16(CmdLine);
        if(CmdLine[0]==0) continue;

        if(StrCaseCmp16(CmdLine, L"hello")==0){ PrintStr(L"Hello!\r\n"); continue; }
        gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_GREEN, EFI_BLACK));
        if(StrCaseCmp16(CmdLine, L"to8")==0){ PrintStr(L"Yes, thats me!\r\n"); continue; }
        gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_GREEN, EFI_BLACK));
        if(StrCaseCmp16(CmdLine, L"about")==0){ PrintStr(L"This is v1 of wkg2673's UEFI Shell! - @wkg2673\r\n"); continue; }
        gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
        if(StrCaseCmp16(CmdLine, L"help")==0 || StrCaseCmp16(CmdLine, L"?")==0){ PrintHelp(); continue; }
        if(StrCaseCmp16(CmdLine, L"clear")==0 || StrCaseCmp16(CmdLine, L"cls")==0){ gST->ConOut->ClearScreen(gST->ConOut); continue; }
        if(StrCaseCmp16(CmdLine, L"exit")==0){ PrintStr(L"Exiting...\r\n"); break; }
        if(StrCaseCmp16(CmdLine, L"off")==0 || StrCaseCmp16(CmdLine, L"shtdown")==0 || StrCaseCmp16(CmdLine, L"shutdown")==0){
            PrintStr(L"Shutting down...\r\n");
            gBS->Stall(500000);
            gRT->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, 0);
            break;
        }
        if(StrCaseCmp16(CmdLine, L"reboot")==0 || StrCaseCmp16(CmdLine, L"reset")==0 || StrCaseCmp16(CmdLine, L"restart")==0){
            PrintStr(L"Rebooting...\r\n");
            gBS->Stall(500000);
            gRT->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, 0);
            break;
        }
        if(StartsWithI(CmdLine, L"ls")){
            CHAR16 *args=L"";
            if(StrLen16(CmdLine)==2) args=L"";
            else if(CmdLine[2]==L' '||CmdLine[2]==L'\t') args=CmdLine+3;
            else { PrintStr(L"Unknown command: "); PrintStr(CmdLine); PrintStr(L"\r\n"); continue; }
            CmdLs(args); continue;
        }
        if(StartsWithI(CmdLine, L"dir")){
            CHAR16 *args=L"";
            if(StrLen16(CmdLine)>3){
                if(CmdLine[3]==L' '||CmdLine[3]==L'\t') args=CmdLine+4;
                else { PrintStr(L"Unknown: "); PrintStr(CmdLine); PrintStr(L"\r\n"); continue; }
            }
            CmdLs(args); continue;
        }
        if(StartsWithI(CmdLine, L"echo")){
            CHAR16 *rest=L"";
            if(StrLen16(CmdLine)>4){
                if(CmdLine[4]==L' '||CmdLine[4]==L'\t') rest=CmdLine+5;
                else { PrintStr(L"Unknown: "); PrintStr(CmdLine); PrintStr(L"\r\n"); continue; }
            } else if(StrLen16(CmdLine)==4) rest=L"";
            else { PrintStr(L"Unknown: "); PrintStr(CmdLine); PrintStr(L"\r\n"); continue; }
            CmdEcho(rest); continue;
        }
        if(StartsWithI(CmdLine, L"cat ") || StartsWithI(CmdLine, L"type ")){
            CHAR16 *args = StartsWithI(CmdLine, L"cat ") ? CmdLine+4 : CmdLine+5;
            CmdCat(args); continue;
        }
        if(StrCaseCmp16(CmdLine, L"boot")==0) { CmdBoot(ImageHandle); continue; }
        if(StartsWithI(CmdLine, L"boot ")) { CmdBoot(ImageHandle); continue; }
        if(StrCaseCmp16(CmdLine, L"tree")==0) { CmdTree(L""); continue; }
        if(StartsWithI(CmdLine, L"tree ")) { CHAR16 *a = CmdLine+5; Trim16(a); CmdTree(a); continue; }
        if(StrCaseCmp16(CmdLine, L"layout")==0 || StrCaseCmp16(CmdLine, L"keymap")==0){
            PrintKeyboardInfo();
            if (UseGermanLayout) PrintStr(L"Current: de (QWERTZ)\r\n");
            else PrintStr(L"Current: us (QWERTY)\r\n");
            PrintStr(L"Usage: layout de | layout us | layout auto\r\n");
            continue;
        }
        if(StartsWithI(CmdLine, L"layout ") || StartsWithI(CmdLine, L"keymap ")){
            CHAR16 *arg = FindChar16(CmdLine, L' ');
            if (arg) { arg++; Trim16(arg); }
            else arg = L"";
            if(StrCaseCmp16(arg, L"de")==0 || StrCaseCmp16(arg, L"german")==0 || StrCaseCmp16(arg, L"deutsch")==0 || StrCaseCmp16(arg, L"qwertz")==0){
                UseGermanLayout = TRUE;
                PrintStr(L"Switched to German QWERTZ (y/z swapped, ae/oe/ue via ; ' [ )\r\n");
            } else if(StrCaseCmp16(arg, L"us")==0 || StrCaseCmp16(arg, L"en")==0 || StrCaseCmp16(arg, L"english")==0 || StrCaseCmp16(arg, L"qwerty")==0){
                UseGermanLayout = FALSE;
                PrintStr(L"Switched to US QWERTY\r\n");
            } else if(StrCaseCmp16(arg, L"auto")==0){
                DetectKeyboardLayout();
                PrintKeyboardInfo();
            } else {
                PrintStr(L"Usage: layout [de/us/auto]\r\n");
            }
            continue;
        }
        PrintStr(L"Unknown command: '"); PrintStr(CmdLine); PrintStr(L"'\r\nType 'help' for list.\r\n");
    }
    if(Root) Root->Close(Root);
    return EFI_SUCCESS;
}
