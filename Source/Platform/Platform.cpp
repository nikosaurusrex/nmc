#include "Platform.h"

#if ARCH_X64 && COMPILER_CLANG

#pragma function(memcpy)
void *memcpy(void *dst, void const *src, size_t size) {
    __asm__ volatile (
        "rep movsb"
        : "=D"(dst), "=S"(src), "=c"(size)
        : "D"(dst), "S"(src), "c"(size)
        : "memory"
    );
    return dst;
}

#pragma function(memset)
void *memset(void *ptr, int byte, size_t size) {
    __asm__ volatile (
        "rep stosb"
        : "=D"(ptr), "=a"(byte), "=c"(size)
        : "D"(ptr), "a"(byte), "c"(size)
        : "memory"
    );
    return ptr;
}

#pragma function(memmove)
void *memmove(void *_dst, const void *_src, size_t size) {
    u8 *dst = (u8 *)_dst;
    u8 *src = (u8 *)_src;

    if (dst < src) {
        __asm__ volatile (
            "rep movsb"
            : "=D"(dst), "=S"(src), "=c"(size)
            : "D"(dst), "S"(src), "c"(size)
            : "memory"
        );
    } else if (dst > src) {
        __asm__ volatile (
            "std\n"
            "rep movsb\n"
            "cld"
            : "=D"(dst), "=S"(src), "=c"(size)
            : "D"(dst + size - 1), "S"(src + size - 1), "c"(size)
            : "memory"
        );
    }
    return dst;
}

int _CompareMemory(u8 *a, u8 *b, u64 size) {
    if (size == 0) {
        return 1;
    }

    int result = 1;

    __asm__ volatile (
        "repe cmpsb\n"
        "jne 1f\n"
        "movl $1, %[Result]\n"
        "jmp 2f\n"
    "1:\n"
        "movl $0, %[Result]\n"
    "2:\n"
        : [result] "=r"(result)
        : "D"(a), "S"(b), "c"(size)
        : "cc", "memory"
    );

    return result;
}

#else

#pragma function(memcpy)
void *memcpy(void *_dst, void const *_src, size_t size) {
    u8 *dst = (u8 *)_dst;
    u8 *src = (u8 *)_src;
    for (u64 i = 0; i < size; i++) {
        dst[i] = src[i];
    }
    return dst;
}

#pragma function(memset)
void *memset(void *_ptr, int Byte, size_t size) {
    u8 *ptr = (u8 *)_ptr;
    for (u64 i = 0; i < size; i++) {
        ptr[i] = Byte;
    }
    return ptr;
}

#pragma function(memmove)
void *memmove(void *_dst, const void *_src, size_t size) {
    u8 *dst = (u8 *)_dst;
    u8 *src = (u8 *)_src;
    if (dst < src) {
        for (u64 i = 0; i < size; i++) {
            dst[i] = src[i];
        }
    } else {
        for (u64 i = size; i > 0; i--) {
            dst[i - 1] = src[i - 1];
        }
    }

    return dst;
}

int _CompareMemory(u8 *a, u8 *b, u64 size) {
    int result = 1;

    for (u64 i = 0; i < size; i++) {
        if (a[i] != b[i]) {
            result = 0;
            break;
        }
    }

    return result;
}

#endif

void _CopyMemory(u8 *dst, u8 *src, u64 size) {
    memcpy(dst, src, size);
}

void _SetMemory(u8 *ptr, u8 byte, u64 size) {
    memset(ptr, byte, size);
}

void _MoveMemory(u8 *dst, u8 *src, u64 size) {
    memmove(dst, src, size);
}

String ReadFile(String path) {
    OS_Handle handle = OpenFile(path, OS_READ | OS_SHARED);

    if (!IsValidFile(handle)) {
        return String();
    }

    OS_FileInfo fileinfo = GetFileInfo(handle);
    void *mem = HeapAlloc(fileinfo.size + 1);
    String contents = ReadHandle(handle, fileinfo.size, mem);
    CloseFile(handle);

    return contents;
}

internal u32 RunTaskQueue(void *args) {
    TaskQueue *queue = (TaskQueue *) args;

    while (!queue->canceled) {
        u32 cur_idx = queue->dequeue_index;
        u32 nxt_idx = (queue->dequeue_index + 1) % ArrayCount(queue->tasks);
        if (cur_idx != queue->enqueue_index) {
            u32 idx = AtomicCompareExchange(&queue->dequeue_index, nxt_idx, cur_idx);

            if (idx == cur_idx) {
                Task to_execute = queue->tasks[idx];
                to_execute.function(queue, to_execute.ptr);
            }
        } else {
            TakeSemaphore(queue->semaphore);
        }
    }

    return 0;
}

void CreateTaskQueue(TaskQueue *queue, u32 thread_count) {
    queue->semaphore = CreateSemaphore(0, thread_count);
    queue->enqueue_index = 0;
    queue->dequeue_index = 0;
    queue->canceled = false;

    for (u32 i = 0; i < thread_count; ++i) {
        StartThread(RunTaskQueue, queue);
    }
}

void EnqueueTask(TaskQueue *queue, TaskFunc function, void *ptr) {
    u32 idx = (queue->enqueue_index + 1) % ArrayCount(queue->tasks);

    Task *_new = queue->tasks + idx;
    _new->function = function;
    _new->ptr = ptr;

    // TODO: Write barrier
    queue->enqueue_index = idx;

    DropSemaphore(queue->semaphore);
}

void CancelTaskQueue(TaskQueue *queue) {
    queue->canceled = true;
}
