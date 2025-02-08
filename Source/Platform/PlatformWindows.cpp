#include "Platform.h"

#include <stdlib.h>
#include <stdio.h>

#include "../ThirdParty/stb_sprintf.h"

struct WindowsState {
    u64 us_resolution;
    b8 large_pages_enabled;
};

global WindowsState platform_state;

internal void InitPlatform() {
    LARGE_INTEGER li = {};

    if (!QueryPerformanceFrequency(&li)) {
        PrintLiteral("Failed QueryPerformanceFrequency\n");
        Exit(1);
    }

    platform_state.us_resolution = li.QuadPart;
    platform_state.large_pages_enabled = b8(EnableLargePages());
}

b32 EnableLargePages() {
    b32 result = 0;

    HANDLE token;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        TOKEN_PRIVILEGES privileges;
        privileges.PrivilegeCount = 1;

        if (LookupPrivilegeValueA(0, SE_LOCK_MEMORY_NAME, &privileges.Privileges[0].Luid)) {
            privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            if (AdjustTokenPrivileges(token, 0, &privileges, sizeof(privileges), 0, 0)) {
                result = 1;
            }
        }

        CloseHandle(token);
    }

    return result;
}

u64 GetPageSize() {
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);

    return u64(system_info.dwPageSize);
}

u64 GetLargePageSize() {
    SIZE_T size = GetLargePageMinimum();

    return u64(size);
}

void *ReserveMemory(u64 size) {
    return VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
}

b32 CommitMemory(void *ptr, u64 size) {
    return VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) != 0;
}

void *ReserveMemoryLarge(u64 size) {
    return VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);
}

void *ReserveMemoryLargeIfPossible(u64 size) {
    void *result = 0;

    /*
    if (PlatformState.LargePagesEnabled) {
        Result = ReserveMemoryLarge(Size);
    } else {*/
        result = ReserveMemory(size);
        CommitMemory(result, size);
    // }

    return result;
}

void DecommitMemory(void *ptr, u64 size) {
    VirtualFree(ptr, size, MEM_DECOMMIT);
}

void ReleaseMemory(void *ptr, u64 size) {
    VirtualFree(ptr, 0, MEM_RELEASE);
}

void *HeapAlloc(u64 size) {
    return malloc(size);
}

void HeapFree(void *ptr) {
    return free(ptr);
}

internal HANDLE GetStdOutHandle() {
    HANDLE result = GetStdHandle(STD_OUTPUT_HANDLE);

    if (result == INVALID_HANDLE_VALUE || result == 0) {
        if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
            AllocConsole();
        }

        result = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    return result;
}

void Print(const char *fmt, ...) {
    if (fmt) {
        HANDLE console = GetStdOutHandle();

        va_list args;
        va_start(args, fmt);
        int needed = stbsp_vsnprintf(0, 0, fmt, args);
        va_end(args);

        if (needed <= 0) {
            return;
        }

        char *buffer = (char *) HeapAlloc(needed + 1);

        va_start(args, fmt);
        stbsp_vsnprintf(buffer, needed + 1, fmt, args);
        va_end(args);

        WriteConsoleA(console, buffer, needed, 0, 0);

        HeapFree(buffer);
    }
}

void PrintLiteral(const char *literal) {
    if (literal) {
        HANDLE console = GetStdOutHandle();

        WriteConsoleA(console, literal, lstrlenA(literal), 0, 0);
    }
}

void Exit(int code) {
    ExitProcess(u32(code));
}

OS_Handle OpenFile(String path, OS_Flags flags) {
    char *cpath = (char *) HeapAlloc(path.len + 1);
    CopyMemory(cpath, path.ptr, path.len);
    cpath[path.len] = 0;

    DWORD access = 0;
    if (flags & OS_READ)
        access |= GENERIC_READ;
    if (flags & OS_WRITE)
        access |= GENERIC_WRITE;

    DWORD shared = 0;
    if (flags & OS_SHARED)
        shared = FILE_SHARE_READ;

    SECURITY_ATTRIBUTES security_attributes = {sizeof(SECURITY_ATTRIBUTES), 0, 0};

    DWORD creation_disposition = 0;
    if (!(flags & OS_CREATE)) {
        creation_disposition = OPEN_EXISTING;
    }

    DWORD flags_and_attributes = 0;
    HANDLE template_file = 0;
    HANDLE result = CreateFileA(cpath,
                                access,
                                shared,
                                &security_attributes,
                                creation_disposition,
                                flags_and_attributes,
                                template_file);

    HeapFree(cpath);

    return (OS_Handle) result;
}

void CloseFile(OS_Handle file) {
    CloseHandle((HANDLE) file);
}

OS_FileInfo GetFileInfo(OS_Handle file) {
    OS_FileInfo result = {};

    u32 hibits = 0;
    u32 lobits = GetFileSize((HANDLE) file, (DWORD *) &hibits);

    result.size = u64(lobits) | ((u64(hibits)) << 32);

    return result;
}

b32 IsValidFile(OS_Handle file) {
    return ((HANDLE) file) != INVALID_HANDLE_VALUE;
}

String ReadHandle(OS_Handle file, u64 size, void *memory) {
    String result = {};

    LARGE_INTEGER li = {};
    li.QuadPart = 0;

    if (SetFilePointerEx((HANDLE) file, li, 0, FILE_BEGIN)) {
        result.ptr = (u8 *) memory;

        u8 *ptr = result.ptr;
        u8 *end = ptr + size;

        for (;;) {
            u64 unread = (u64) (end - ptr);
            DWORD toread = (DWORD) ClampTop(unread, max_u32);
            DWORD read = 0;
            if (!ReadFile((HANDLE) file, ptr, toread, &read, 0)) {
                break;
            }

            ptr += read;
            result.len += read;

            if (ptr >= end) {
                break;
            }
        }

        *end = 0;
    }

    return result;
}

u64 GetTimeNowUs() {
    LARGE_INTEGER li = {};

    if (QueryPerformanceCounter(&li)) {
        return (li.QuadPart * 1000000) / platform_state.us_resolution;
    }

    return 0;
}

void SleepMs(u32 ms) {
    Sleep(ms);
}

OS_Handle StartThread(ThreadFunc function, void *args) {
    DWORD id;
    HANDLE handle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE) function, args, 0, &id);

    return (OS_Handle) handle;
}

void JoinThread(OS_Handle thread) {
    HANDLE handle = (HANDLE) thread;

    WaitForSingleObject(handle, INFINITE);
    CloseHandle(handle);
}

OS_Handle CreateMutex() {
    return (OS_Handle) CreateMutex(0, FALSE, 0);
}

void DestroyMutex(OS_Handle mutex) {
    CloseHandle((HANDLE) mutex);
}

void AcquireMutex(OS_Handle mutex) {
    WaitForSingleObject((HANDLE) mutex, INFINITE);
}

void ReleaseMutex(OS_Handle mutex) {
    ReleaseMutex((HANDLE) mutex);
}

OS_Handle CreateSemaphore(s32 value, s32 max) {
    return (OS_Handle) CreateSemaphoreA(0, value, max, 0);
}

void DestroySemaphore(OS_Handle semaphore) {
    CloseHandle((HANDLE) semaphore);
}

b32 TakeSemaphore(OS_Handle semaphore) {
    return WaitForSingleObject((HANDLE) semaphore, INFINITE) == WAIT_OBJECT_0;
}

void DropSemaphore(OS_Handle semaphore) {
    ReleaseSemaphore((HANDLE) semaphore, 1, 0);
}

u32 AtomicIncrement(volatile u32 *value) {
    return InterlockedIncrement(value);
}

u32 AtomicCompareExchange(volatile u32 *dst, u32 value, u32 comperand) {
    return InterlockedCompareExchange(dst, value, comperand);
}

extern void NKMain();

void WinMainCRTStartup() {
    InitPlatform();

    NKMain();

    Exit(0);
}

void mainCRTStartup() {
    InitPlatform();

    NKMain();

    Exit(0);
}

int main() {
    InitPlatform();

    NKMain();
    return 0;
}

extern "C" int _fltused = 0;
