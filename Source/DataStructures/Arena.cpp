#include "Arena.h"

#include "../Platform/Platform.h"

Arena CreateArena(u64 size) {
    Arena result = {};

    size = AlignPow2(size, GetLargePageSize());

    result.size = size;
    result.top = 0;
    result.ptr = (u8 *)ReserveMemoryLargeIfPossible(size);

    return result;
}

void FreeArena(Arena *a) {
    ReleaseMemory(a->ptr, a->size);
}

Arena CreateSubArena(Arena *parent, u64 size) {
    Arena result = {};

    result.size = size;
    result.top = 0;
    result.ptr = PushSize(parent, size);

    return result;
}

void ResetArena(Arena *a) {
    a->top = 0;
}

TempArena BeginTempArena(Arena *owning) {
    TempArena result = {};

    result.owning = owning;
    result.saved_top = owning->top;

    return result;
}

void EndTempArena(TempArena ta) {
    ta.owning->top = ta.saved_top;
}

u8 *PushSize(Arena *a, u64 size, u64 align) {
    u8 *result = 0;

    u64 base = (u64) (a->ptr + a->top);
    u64 offset = 0;

    u64 mask = align - 1;
    if (base & mask) {
        offset = align - (base & mask);
    }

    size += offset;

    result = (u8 *) (base + offset);

    a->top += size;

    return result;
}
