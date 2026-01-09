#pragma once

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef UINT8        U8;
typedef UINT16       U16;
typedef UINT32       U32;
typedef UINT64       U64;
typedef unsigned int uint;

typedef INT8         I8;
typedef INT16        I16;
typedef INT32        I32;
typedef INT64        I64;

typedef UINT32       B32;

typedef float        F32;
typedef double       F64;
#else
#include <stdint.h>

typedef uint8_t      U8;
typedef uint16_t     U16;
typedef uint32_t     U32;
typedef uint64_t     U64;
typedef unsigned int uint;

typedef int8_t       I8;
typedef int16_t      I16;
typedef int32_t      I32;
typedef int64_t      I64;

typedef uint32_t     B32;

typedef float        F32;
typedef double       F64;
#endif

#define true (1)
#define false (0)

#if !defined(ENCLOSE)
#define MIM_CONCAT(a, b) a ## b
#define ENCLOSE_IMPL2(begin, end, index) for (U32 index = ((begin), 0); !index; ((end), index++))
#define ENCLOSE_IMPL(begin, end, index, counter) ENCLOSE_IMPL2(begin, end, MIM_CONCAT(index, counter))
#define ENCLOSE(begin, end) ENCLOSE_IMPL(begin, end, base_index_, __COUNTER__)
#endif
#if !defined(DEFER)
#define MIM_CONCAT(a, b) a ## b
#define DEFER_IMPL2(end, index) for (U32 index = 0; !index; ((end), index++))
#define DEFER_IMPL(end, index, counter) DEFER_IMPL2(end, MIM_CONCAT(index, counter))
#define DEFER(end) DEFER_IMPL(end, base_index_, __COUNTER__)
#endif
#if !defined(Assert)
#define Assert(expr)
#endif

typedef struct
{
	U64 size;
	U8* data;
}
Mim_Buffer;

typedef struct
{
	U64 size;
	U64 pos;
	U64 scratch_count;
	U8* base;
	B32 reserved;
	U32 _padding;
}
Mim_Arena;

typedef struct
{
	Mim_Arena* arena;
	U64 pos;
}
Mim_ScratchArena;

#define MIM_PUSH_ARRAY(arena, type, count) (type*)mim_arena_push_size((arena), sizeof(type) * (count))
#define MIM_PUSH(arena, type) (type*)mim_arena_push_size((arena), sizeof(type))

#define MIM_MAX_U8  0xff
#define MIM_MAX_U16 0xffff
#define MIM_MAX_U32 0xffffffff
#define MIM_MAX_U64 0xffffffffffffffff

U64 mim_kilo_bytes(U64 count);
U64 mim_mega_bytes(U64 count);
U64 mim_giga_bytes(U64 count);
U64 mim_tera_bytes(U64 count);

void* mim_alloc(U64 size);
B32 mim_free(void* data, U64 size);
void mim_copy(void* dst, void* src, U64 size);
void mim_fill(void* data, U64 size, U64 value);
void mim_zero(void* data, U64 size);
U64 mim_align16(U64 size);
U8* mim_arena_pointer(Mim_Arena* arena);
Mim_Arena mim_arena_create(U64 size);
B32 mim_arena_destroy(Mim_Arena* arena);
U8* mim_arena_push_size(Mim_Arena* arena, U64 size);
void mim_arena_pop_size(Mim_Arena* arena, U64 size);
void mim_arena_reset(Mim_Arena* arena);
U8* mim_arena_reserve(Mim_Arena* arena);
void mim_arena_commit(Mim_Arena* arena, U64 size);
Mim_ScratchArena mim_scratch_begin(Mim_Arena* arena);
void mim_scratch_end(Mim_ScratchArena* scratch);

/** Serialization *********************************************************************************/

typedef struct
{
	U64 size;
	U64 pos;
	U8* base;
	Mim_Arena* arena;
}
Mim_WriteBuffer;

#define MIM_SERIALIZE(write_buffer, arena) ENCLOSE(mim_begin_serializing(write_buffer, arena), mim_end_serializing(write_buffer))

void mim_begin_serializing(Mim_WriteBuffer* write_buffer, Mim_Arena* arena);
void mim_end_serializing(Mim_WriteBuffer* write_buffer);
U8* mim_write_buffer_pointer(Mim_WriteBuffer* write_buffer);
void mim_write_data(Mim_WriteBuffer* write_buffer, U8* data, U64 size);
void mim_write_u32(Mim_WriteBuffer* write_buffer, U32 value);
void mim_write_u64(Mim_WriteBuffer* write_buffer, U64 value);
void mim_write_f32(Mim_WriteBuffer* write_buffer, F32 value);
void mim_write_f64(Mim_WriteBuffer* write_buffer, F64 value);
void mim_write_buffer(Mim_WriteBuffer* write_buffer, U8* data, U64 size);

typedef struct
{
	U64 size;
	U64 pos;
	U8* base;
}
Mim_ReadBuffer;

Mim_ReadBuffer mim_create_read_buffer(Mim_Buffer* buffer);
U8* mim_read_buffer_pointer(Mim_ReadBuffer* read_buffer);
B32 mim_read_data(U8* out, Mim_ReadBuffer* buffer, U64 size);
B32 mim_read_u32(U32* out, Mim_ReadBuffer* buffer);
B32 mim_read_u64(U64* out, Mim_ReadBuffer* buffer);
B32 mim_read_f32(F32* out, Mim_ReadBuffer* buffer);
B32 mim_read_f64(F64* out, Mim_ReadBuffer* buffer);
B32 mim_read_buffer(U8* output_data, U64* output_size, Mim_ReadBuffer* buffer);
