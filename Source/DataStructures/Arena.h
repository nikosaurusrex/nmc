#pragma once

#include "../General.h"

struct Arena {
    u64 size;
    u64 top;
    u8 *ptr;
};

struct TempArena {
    Arena *owning;
    u64 saved_top;
};

Arena CreateArena(u64 size);
void FreeArena(Arena *a);
Arena CreateSubArena(Arena *parent, u64 size);

void ResetArena(Arena *a);

TempArena BeginTempArena(Arena *owning);
void EndTempArena(TempArena ta);

#define PushArray(a, ty, n, ...) (ty *) PushSize(a, n * sizeof(ty), ##__VA_ARGS__)
#define PushStruct(a, ty, ...) (ty *) PushSize(a, sizeof(ty), ##__VA_ARGS__)
u8 *PushSize(Arena *a, u64 size, u64 align = 4);
