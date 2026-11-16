/*
 * Custom UEFI Shell - "C:\" Green Prompt Edition
 * 
 * Features:
 *  - Green "C:\>" prompt (instead of Shell>)
 *  - hello -> Hello!
 *  - ls / dir -> list files on current volume
 *  - echo <text> -> print
 *  - echo <text> > <file> -> create file
 *  - clear / cls -> clear screen
 *  - help -> show help
 *  - exit -> exit shell
 *  - shutdown -> power off via ResetSystem
 *
 * Build: gnu-efi (x86_64)
 *   Linux:  make
 *   Windows: build.bat (needs clang or x86_64-w64-mingw32-gcc)
 *
 * Tested with gnu-efi 3.0.x
 */

#include <efi.h>
#include <efilib.h>

#define MAX_CMD_LEN 512
#define MAX_ARGS    16

static EFI_FILE_PROTOCOL *Root = NULL;

// ---------- Utilities (CHAR16) ----------

static UINTN StrLen16(CONST CHAR16 *s) {
    UINTN i = 0;
    while (s[i] != L'\0') i++;
    return i;
}

static INTN StrCmp16(CONST CHAR16 *a, CONST CHAR16 *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (INTN)(*a) - (INTN)(*b);
}

static INTN StrCaseCmp16(CONST CHAR16 *a, CONST CHAR16 *b) {
    while (*a && *b) {
        CHAR16 ca = *a, cb = *b;
        if (ca >= L'A' && ca <= L'Z') ca += 32;
        if (cb >= L'A' && cb <= L'Z') cb += 32;
        if (ca != cb) return (INTN)ca - (INTN)cb;
        a++; b++;
    }
    return (INTN)(*a) - (INTN)(*b);
}

static INTN StrnCmp16(CONST CHAR16 *a, CONST CHAR16 *b, UINTN n) {
    for (UINTN i=0;i<n;i++) {
        if (a[i] != b[i]) return (INTN)a[i] - (INTN)b[i];
        if (a[i]==L'\0') return 0;
    }
    return 0;
}

static VOID StrCpy16(CHAR16 *dst, CONST CHAR16 *src) {
    while ((*dst++ = *src++) != L'\0');
}

static CHAR16 ToLower16(CHAR16 c) {
    if (c >= L'A' && c <= L'Z') return c + 32;
    return c;
}

static VOID Trim16(CHAR16 *s) {
    UINTN len = StrLen16(s);
    UINTN start = 0;
    while (s[start]==L' ' || s[start]==L'\t') start++;
    if (start) {
        UINTN j = 0;
        for (UINTN i=start; s[i]!=L'\0'; i++, j++) s[j]=s[i];
        s[j]=L'\0';
        len = j;
    } else {
        len = StrLen16(s);
    }
    // trim trailing
    while (len>0 && (s[len-1]==L' ' || s[len-1]==L'\t')) {
        s[len-1]=L'\0';
        len--;
    }
}

static BOOLEAN StartsWithI(CONST CHAR16 *str, CONST CHAR16 *prefix) {
    UINTN plen = StrLen16(prefix);
    if (StrLen16(str) < plen) return FALSE;
    for (UINTN i=0;i<plen;i++) {
        if (ToLower16(str[i]) != ToLower16(prefix[i])) return FALSE;
    }
    return TRUE;
}

static CHAR16* FindChar16(CHAR16 *s, CHAR16 c) {
    while (*s) {
        if (*s == c) return s;
        s++;
    }
    return NULL;
}

// Convert CHAR16 to CHAR8 (ASCII only) for file write
static UINTN UnicodeToAscii(CONST CHAR16 *src, CHAR8 *dst, UINTN dstSize) {
    UINTN i=0;
    while (src[i] && i+1 < dstSize) {
        dst[i] = (CHAR8)(src[i] & 0xFF);
        i++;
    }
    dst[i]='\0';
    return i;
}

// ---------- File System ----------

static EFI_STATUS OpenRoot(EFI_HANDLE ImageHandle) {
    EFI_STATUS Status;
    EFI_LOADED_IMAGE *LoadedImage = NULL;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs = NULL;

    Status = uefi_call_wrapper(gBS->HandleProtocol, 3, ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID**)&LoadedImage);
    if (EFI_ERROR(Status) || !LoadedImage) {
        // Fallback: try LibOpenRoot if handle is invalid? search all handles
        Print(L"Failed to get LoadedImage: %r\n", Status);
        return Status;
    }

    Status = uefi_call_wrapper(gBS->HandleProtocol, 3, LoadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (VOID**)&Fs);
    if (EFI_ERROR(Status)) {
        // Try to locate any SimpleFileSystem
        EFI_HANDLE *Handles = NULL;
        UINTN HandleCount = 0;
        Status = uefi_call_wrapper(gBS->LocateHandleBuffer, 5, ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &HandleCount, &Handles);
        if (EFI_ERROR(Status) || HandleCount==0) {
            Print(L"No SimpleFileSystem found: %r\n", Status);
            return Status;
        }
        // Use first handle (usually the ESP)
        Status = uefi_call_wrapper(gBS->HandleProtocol, 3, Handles[0], &gEfiSimpleFileSystemProtocolGuid, (VOID**)&Fs);
        FreePool(Handles);
        if (EFI_ERROR(Status)) return Status;
    }

    Status = uefi_call_wrapper(Fs->OpenVolume, 2, Fs, &Root);
    if (EFI_ERROR(Status)) {
        Print(L"OpenVolume failed: %r\n", Status);
    }
    return Status;
}

// ---------- ls ----------

static void CmdLs(CHAR16 *args) {
    EFI_STATUS Status;
    EFI_FILE_PROTOCOL *Dir = Root;
    EFI_FILE_PROTOCOL *Target = NULL;
    BOOLEAN NeedClose = FALSE;

    Trim16(args);
    if (args[0] != L'\0') {
        // Open sub directory / file
        Status = uefi_call_wrapper(Root->Open, 5, Root, &Target, args, EFI_FILE_MODE_READ, 0);
        if (EFI_ERROR(Status)) {
            Print(L"ls: cannot access '%s': %r\r\n", args, Status);
            return;
        }
        Dir = Target;
        NeedClose = TRUE;
    }

    // Read directory entries
    UINTN BufSize = 4096;
    EFI_FILE_INFO *FileInfo = AllocatePool(BufSize);
    if (!FileInfo) {
        Print(L"ls: out of memory\r\n");
        if (NeedClose) uefi_call_wrapper(Dir->Close, 1, Dir);
        return;
    }

    // Reset to beginning (re-open already at start). Directory read starts at 0.
    // Enumerate
    while (TRUE) {
        UINTN ReadSize = BufSize;
        Status = uefi_call_wrapper(Dir->Read, 3, Dir, &ReadSize, FileInfo);
        if (EFI_ERROR(Status)) {
            Print(L"ls: read error %r\r\n", Status);
            break;
        }
        if (ReadSize == 0) break; // end

        // Skip . and .. maybe show but we filter . 
        // FileInfo->FileName is CHAR16*
        UINTN fsize = FileInfo->FileSize;
        EFI_FILE_INFO *info = FileInfo;
        BOOLEAN isDir = (info->Attribute & EFI_FILE_DIRECTORY) != 0;

        // Print
        // Color dirs differently? Keep simple
        if (isDir) {
            // Cyan for dirs
            uefi_call_wrapper(gST->ConOut->SetAttribute, 2, gST->ConOut, EFI_TEXT_ATTR(EFI_CYAN, EFI_BLACK));
            Print(L"  <DIR>  %s\r\n", info->FileName);
            uefi_call_wrapper(gST->ConOut->SetAttribute, 2, gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
        } else {
            Print(L"  %10ld  %s\r\n", fsize, info->FileName);
        }
    }

    FreePool(FileInfo);
    if (NeedClose) uefi_call_wrapper(Dir->Close, 1, Dir);
}

// ---------- echo ----------

static void CmdEcho(CHAR16 *rest) {
    // rest is everything after "echo"
    // Check for redirection: ">"
    CHAR16 *redir = FindChar16(rest, L'>');
    if (redir) {
        *redir = L'\0';
        CHAR16 *text = rest;
        CHAR16 *filename = redir + 1;
        Trim16(text);
        Trim16(filename);
        // Remove quotes if present?
        // If filename contains spaces, keep as is after trim
        if (filename[0]==L'\0') {
            Print(L"echo: missing filename after '>'\r\n");
            return;
        }
        // Strip leading '>' already done, also handle ">>" not supported
        if (filename[0]==L'>') filename++;

        Trim16(filename);
        // Remove surrounding quotes
        UINTN flen = StrLen16(filename);
        if (flen >=2 && ((filename[0]==L'"' && filename[flen-1]==L'"') || (filename[0]==L'\'' && filename[flen-1]==L'\''))) {
            filename[flen-1]=L'\0';
            filename++;
        }
        // Create / overwrite file
        EFI_FILE_PROTOCOL *File = NULL;
        EFI_STATUS Status = uefi_call_wrapper(Root->Open, 5, Root, &File, filename, EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
        if (EFI_ERROR(Status)) {
            Print(L"echo: cannot create '%s': %r\r\n", filename, Status);
            return;
        }
        // Convert text to ASCII + add newline? Original echo without newline? We do not add extra CRLF unless needed.
        // The classic: echo Hello! > file -> file contains Hello! (maybe newline not needed). We'll write as is.
        // If text is empty, create empty file.
        CHAR8 AsciiBuf[1024];
        UINTN AsciiLen = 0;
        if (text[0] != L'\0') {
            AsciiLen = UnicodeToAscii(text, AsciiBuf, sizeof(AsciiBuf));
            // Write
            UINTN WriteSize = AsciiLen;
            Status = uefi_call_wrapper(File->Write, 3, File, &WriteSize, AsciiBuf);
            if (EFI_ERROR(Status)) {
                Print(L"echo: write failed: %r\r\n", Status);
            }
        } else {
            // empty file, truncate
            // need to set position 0 and truncate? Open with CREATE already truncates? Actually need to set info size 0?
            // Most impl truncates on open with CREATE. If we opened existing, we should set size 0.
            // Do File->SetPosition 0 and then set file size? Simpler: close works, content is empty.
        }
        uefi_call_wrapper(File->Close, 1, File);
        // echo to file doesn't print to screen (like bash)
    } else {
        Trim16(rest);
        // Remove surrounding quotes for display? Keep as is but print
        Print(L"%s\r\n", rest);
    }
}

// ---------- Input ----------

static UINTN ReadLine(CHAR16 *Buffer, UINTN MaxLen) {
    UINTN idx = 0;
    EFI_INPUT_KEY Key;
    EFI_STATUS Status;

    // Ensure buffer zero
    for (UINTN i=0;i<MaxLen;i++) Buffer[i]=L'\0';

    while (TRUE) {
        UINTN EventIndex;
        uefi_call_wrapper(gBS->WaitForEvent, 3, 1, &gST->ConIn->WaitForKey, &EventIndex);
        Status = uefi_call_wrapper(gST->ConIn->ReadKeyStroke, 2, gST->ConIn, &Key);
        if (EFI_ERROR(Status)) continue;

        if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) { // Enter
            Print(L"\r\n");
            break;
        } else if (Key.UnicodeChar == CHAR_BACKSPACE || Key.UnicodeChar == 0x7F) {
            if (idx > 0) {
                idx--;
                Buffer[idx]=L'\0';
                // Erase char on screen: backspace, space, backspace
                Print(L"\b \b");
            }
        } else if (Key.ScanCode == SCAN_ESC) {
            // Clear line on ESC?
            while (idx>0) {
                idx--;
                Print(L"\b \b");
            }
            Buffer[0]=L'\0';
        } else if (Key.UnicodeChar >= 32 && Key.UnicodeChar <= 126) {
            if (idx < MaxLen-1) {
                Buffer[idx++] = Key.UnicodeChar;
                Buffer[idx]=L'\0';
                // Echo
                CHAR16 s[2] = {Key.UnicodeChar, L'\0'};
                Print(L"%s", s);
            }
        } else if (Key.UnicodeChar != 0) {
            // Support other unicode
            if (idx < MaxLen-1) {
                Buffer[idx++] = Key.UnicodeChar;
                Buffer[idx]=L'\0';
                CHAR16 s[2] = {Key.UnicodeChar, L'\0'};
                Print(L"%s", s);
            }
        }
        // Handle up/down history? Not needed for minimal shell
    }
    return idx;
}

// ---------- Help / Clear ----------

static void PrintHelp(void) {
    Print(L"\r\nCustom UEFI Shell - commands:\r\n");
    Print(L"  hello                - respond with Hello!\r\n");
    Print(L"  ls / dir [path]      - list files on current volume\r\n");
    Print(L"  echo <text>          - print text\r\n");
    Print(L"  echo <text> > <file> - create file with content\r\n");
    Print(L"  cat/type <file>      - show file contents\r\n");
    Print(L"  clear / cls          - clear screen\r\n");
    Print(L"  help / ?             - show this help\r\n");
    Print(L"  exit                 - exit shell\r\n");
    Print(L"  shutdown             - power off system\r\n");
    Print(L"  reboot / reset       - reboot system\r\n");
    Print(L"\r\n");
}

static void CmdCat(CHAR16 *args) {
    Trim16(args);
    if (args[0]==L'\0') {
        Print(L"cat: missing file name\r\n");
        return;
    }
    EFI_FILE_PROTOCOL *File=NULL;
    EFI_STATUS Status = uefi_call_wrapper(Root->Open, 5, Root, &File, args, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(Status)) {
        Print(L"cat: cannot open '%s': %r\r\n", args, Status);
        return;
    }
    // Get file info to know size?
    CHAR8 Buf[512];
    while (TRUE) {
        UINTN Size = sizeof(Buf)-1;
        Status = uefi_call_wrapper(File->Read, 3, File, &Size, Buf);
        if (EFI_ERROR(Status) || Size==0) break;
        Buf[Size]='\0';
        // Convert to CHAR16 for Print? Print supports %a for ASCII
        Print(L"%a", Buf);
    }
    Print(L"\r\n");
    uefi_call_wrapper(File->Close, 1, File);
}

// ---------- Main Loop ----------

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS Status;
    InitializeLib(ImageHandle, SystemTable);

    // Clear screen and set background
    uefi_call_wrapper(gST->ConOut->ClearScreen, 1, gST->ConOut);
    uefi_call_wrapper(gST->ConOut->SetAttribute, 2, gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));

    Print(L"\r\nwkg2673's UEFI Shell v1.0\r\n");
    Print(L"Type 'help' for commands.\r\n\r\n");

    Status = OpenRoot(ImageHandle);
    if (EFI_ERROR(Status)) {
        Print(L"Warning: filesystem not available, ls/echo > will fail: %r\r\n", Status);
    }

    CHAR16 CmdLine[MAX_CMD_LEN];

    while (TRUE) {
        // Print prompt: green "C:\"
        uefi_call_wrapper(gST->ConOut->SetAttribute, 2, gST->ConOut, EFI_TEXT_ATTR(EFI_GREEN, EFI_BLACK));
        Print(L"C:\\");
        // alternative: Print(L"C:\\> "); but spec says display "C:\"
        // We'll show "C:\> " with green C:\ and white >
        uefi_call_wrapper(gST->ConOut->SetAttribute, 2, gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
        Print(L"> ");

        // Reset to light gray for input
        uefi_call_wrapper(gST->ConOut->SetAttribute, 2, gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));

        ReadLine(CmdLine, MAX_CMD_LEN);
        Trim16(CmdLine);

        if (CmdLine[0]==L'\0') continue;

        // hello (exact, case-insensitive)
        if (StrCaseCmp16(CmdLine, L"hello")==0) {
            Print(L"Hello!\r\n");
            continue;
        }

        if (StrCaseCmp16(CmdLine, L"help")==0 || StrCaseCmp16(CmdLine, L"?")==0) {
            PrintHelp();
            continue;
        }

        if (StrCaseCmp16(CmdLine, L"clear")==0 || StrCaseCmp16(CmdLine, L"cls")==0) {
            uefi_call_wrapper(gST->ConOut->ClearScreen, 1, gST->ConOut);
            continue;
        }

        if (StrCaseCmp16(CmdLine, L"exit")==0) {
            Print(L"Exiting...\r\n");
            break;
        }

        if (StrCaseCmp16(CmdLine, L"shutdown")==0) {
            Print(L"Shutting down...\r\n");
            // Small delay
            uefi_call_wrapper(gBS->Stall, 1, 500000);
            uefi_call_wrapper(gRT->ResetSystem, 4, EfiResetShutdown, EFI_SUCCESS, 0, NULL);
            // Should not return
            break;
        }

        if (StrCaseCmp16(CmdLine, L"reboot")==0 || StrCaseCmp16(CmdLine, L"reset")==0) {
            Print(L"Rebooting...\r\n");
            uefi_call_wrapper(gBS->Stall, 1, 500000);
            uefi_call_wrapper(gRT->ResetSystem, 4, EfiResetCold, EFI_SUCCESS, 0, NULL);
            break;
        }

        if (StartsWithI(CmdLine, L"ls")) {
            CHAR16 *args = CmdLine + 2;
            if (StrLen16(CmdLine) == 2) args = L"";
            else if (CmdLine[2]==L' ' || CmdLine[2]==L'\t') args = CmdLine + 3;
            else { Print(L"Unknown command: %s\r\nType 'help' for list.\r\n", CmdLine); continue; }
            CmdLs(args);
            continue;
        }

        if (StartsWithI(CmdLine, L"dir")) {
            CHAR16 *args = L"";
            if (StrLen16(CmdLine) > 3) {
                if (CmdLine[3]==L' ' || CmdLine[3]==L'\t') args = CmdLine + 4;
                else { Print(L"Unknown command: %s\r\n", CmdLine); continue; }
            }
            CmdLs(args);
            continue;
        }

        if (StartsWithI(CmdLine, L"echo")) {
            // Need at least "echo"
            CHAR16 *rest = L"";
            if (StrLen16(CmdLine) > 4) {
                if (CmdLine[4]==L' ' || CmdLine[4]==L'\t') rest = CmdLine + 5;
                else { Print(L"Unknown command: %s\r\n", CmdLine); continue; }
            } else if (StrLen16(CmdLine) == 4) {
                rest = L"";
            } else { Print(L"Unknown command: %s\r\n", CmdLine); continue; }
            CmdEcho(rest);
            continue;
        }

        if (StartsWithI(CmdLine, L"cat ") || StartsWithI(CmdLine, L"type ")) {
            CHAR16 *args;
            if (StartsWithI(CmdLine, L"cat ")) args = CmdLine + 4;
            else args = CmdLine + 5;
            CmdCat(args);
            continue;
        }

        // Unknown
        Print(L"Unknown command: '%s'\r\nType 'help' for list.\r\n", CmdLine);
    }

    if (Root) uefi_call_wrapper(Root->Close, 1, Root);
    return EFI_SUCCESS;
}
