#include "../Platform/PlatformContext.h"

#if ARCH_X64
#include "SIMD_x64.cpp"
#elif ARCH_ARM64
#error Not Implemented ARM64 SIMD.
#else
#error Unsupported Architecture for SIMD.
#endif
