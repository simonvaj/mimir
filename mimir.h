#pragma once

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
typedef INT8         I8;
typedef INT16        I16;
typedef INT32        I32;
typedef INT64        I64;
typedef UINT8        U8;
typedef UINT16       U16;
typedef UINT32       U32;
typedef UINT64       U64;
typedef unsigned int uint;
typedef UINT32       B32;
typedef float        F32;
typedef double       F64;
#endif

#if !defined(ENCLOSE)
#define ENCLOSE_IMPL(begin, end, index) for (U32 index = ((begin), 0); !index; ((end), index++))
#define ENCLOSE(begin, end) ENCLOSE_IMPL(begin, end, base_i ## __COUNTER__)
#endif
#if !defined(DEFER)
#define DEFER_IMPL(end, index) for (U32 index = 0; !index; ((end), index++))
#define DEFER(end) DEFER_IMPL(end, base_i ## __COUNTER__)
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
#define MIM_PUSH_VALUE(arena, type, value) mim_arena_push_value((arena), sizeof(type), value)

#define MIM_MAX_U8  0xff
#define MIM_MAX_U16 0xffff
#define MIM_MAX_U32 0xffffffff
#define MIM_MAX_U64 0xffffffffffffffff

U64 mim_kilo_bytes(U64 count);
U64 mim_mega_bytes(U64 count);
U64 mim_giga_bytes(U64 count);
U64 mim_tera_bytes(U64 count);

void* mim_alloc(U64 size);
B32 mim_free(void* data);
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
void mim_copy(void* dst, void* src, U64 size);
void mim_zero(void* data, U64 size);
U64 mim_align16(U64 size);

/** Serialization *********************************************************************************/

typedef struct
{
	U64 size;
	U64 pos;
	U8* data;
	Mim_Arena* arena;
}
Mim_WriteBuffer;

#define MIM_SERIALIZE(write_buffer, arena) Mim_WriteBuffer write_buffer; ENCLOSE(write_buffer = mim_begin_serializing(arena), mim_end_serializing(&write_buffer))

Mim_WriteBuffer mim_begin_serializing(Mim_Arena* arena);
void mim_end_serializing(Mim_WriteBuffer* write_buffer);
void mim_write_data(Mim_WriteBuffer* buffer, U64 size, U8* data);
void mim_write_u32(Mim_WriteBuffer* buffer, U32 u32);
void mim_write_u64(Mim_WriteBuffer* buffer, U64 u64);
void mim_write_buffer(Mim_WriteBuffer* buffer, U64 size, U8* data);

typedef struct
{
	U64 size;
	U64 pos;
	U8* data;
}
Mim_ReadBuffer;

Mim_ReadBuffer mim_create_read_buffer(Mim_Buffer* buffer);
B32 mim_read_data(U8* out, Mim_ReadBuffer* buffer, U64 size);
B32 mim_read_u32(U32* out, Mim_ReadBuffer* buffer);
B32 mim_read_u64(U64* out, Mim_ReadBuffer* buffer);
B32 mim_read_buffer(U64* size, U8* data, Mim_ReadBuffer* buffer);