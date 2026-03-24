
#define arrayCount(array1) (sizeof(array1) / sizeof(array1[0]))
#define ENUM(value) value,
#define STRING(value) #value,

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef intptr_t intprt;

#if !__cplusplus
typedef u32 bool;
#define true 1
#define false 0
#endif 

#define internal static 

#include "assert.h"
#include "./3DMaths.h"
#include "./easy_memory.h"
#include "./memory_arena.h"
#define EASY_STRING_IMPLEMENTATION 1
#include "./easy_string_utf8.h"
#include "./transform.cpp"
#include "./render.h"
#include "./defines.h"
#include "./color_defines.h"
#include "./resize_array.cpp"