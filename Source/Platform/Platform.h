#pragma once

#include "PlatformContext.h"

#include "../General.h"
#include "../DataStructures/String.h"

// TODO: try to not include windows in header 
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define _NO_CRT_STDIO_INLINE
#include <Windows.h>
#undef CopyMemory
#undef MoveMemory
#undef ZeroMemory
#undef near
#undef far

typedef u64 OS_Handle;
typedef u32 OS_Flags;
enum {
    OS_READ = (1 << 0),
    OS_WRITE = (1 << 1),
    OS_CREATE = (1 << 2),
    OS_SHARED = (1 << 3),
};

struct OS_FileInfo {
    u64 size;
};

typedef u32 (*ThreadFunc)(void *args);

struct TaskQueue;
typedef void (*TaskFunc)(TaskQueue *queue, void *ptr);

struct Task {
    TaskFunc function;
    void *ptr;
};

struct TaskQueue {
    OS_Handle semaphore;
    volatile u32 enqueue_index;
    volatile u32 dequeue_index;
    volatile b32 canceled;

    Task tasks[256];
};

// Memory
b32 EnableLargePages();
u64 GetPageSize();
u64 GetLargePageSize();

void *ReserveMemory(u64 size);
b32 CommitMemory(void *ptr, u64 size);
void *ReserveMemoryLarge(u64 size);
void *ReserveMemoryLargeIfPossible(u64 size);
void DecommitMemory(void *ptr, u64 size);
void ReleaseMemory(void *ptr, u64 size);

void *HeapAlloc(u64 size);
void HeapFree(void *ptr);

// Print
void Print(const char *fmt, ...);
void PrintLiteral(const char *literal);

// Exit
void Exit(int code);

// File Management
OS_Handle OpenFile(String path, OS_Flags flags);
void CloseFile(OS_Handle file);

OS_FileInfo GetFileInfo(OS_Handle file);
b32 IsValidFile(OS_Handle file);

String ReadHandle(OS_Handle file, u64 size, void *memory);

// Time
u64 GetTimeNowUs();
void SleepMs(u32 ms);

// Threading
OS_Handle StartThread(ThreadFunc function, void *args);
void JoinThread(OS_Handle thread);

// Mutex
OS_Handle CreateMutex();
void DestroyMutex(OS_Handle mutex);
void AcquireMutex(OS_Handle mutex);
void ReleaseMutex(OS_Handle mutex);

// Semaphores
OS_Handle CreateSemaphore(s32 value, s32 max);
void DestroySemaphore(OS_Handle semaphore);
b32 TakeSemaphore(OS_Handle semaphore);
void DropSemaphore(OS_Handle semaphore);

// Atomic Operations
u32 AtomicIncrement(volatile u32 *value);
u32 AtomicCompareExchange(volatile u32 *dst, u32 value, u32 comperand);

// Defined in Platform.cpp - not OS specific
void _CopyMemory(u8 *dst, u8 *src, u64 size);
void _MoveMemory(u8 *dst, u8 *src, u64 size);
void _SetMemory(u8 *ptr, u8 byte, u64 size);
int _CompareMemory(u8 *a, u8 *b, u64 size);

#define CopyMemory(dst, src, size) _CopyMemory((u8 *)(dst), (u8 *)(src), (size))
#define MoveMemory(dst, src, size) _MoveMemory((u8 *)(dst), (u8 *)(src), (size))
#define SetMemory(ptr, byte, size) _SetMemory((u8 *)(ptr), (u8)(byte), (size))
#define CompareMemory(a, b, size) _CompareMemory((u8 *)(a), (u8 *)(b), (size))
#define ZeroMemory(ptr, size) _SetMemory((u8 *)(ptr), 0, (size))

String ReadFile(String path);

void CreateTaskQueue(TaskQueue *queue, u32 thread_count);
void EnqueueTask(TaskQueue *queue, TaskFunc function, void *ptr);
void CancelTaskQueue(TaskQueue *queue);
