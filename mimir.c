#include "mimir.h"

U64 mim_kilo_bytes(U64 count)
{
	U64 result = 1024 * count;
	return result;
}

U64 mim_mega_bytes(U64 count)
{
	U64 result = 1024 * mim_kilo_bytes(count);
	return result;
}

U64 mim_giga_bytes(U64 count)
{
	U64 result = 1024 * mim_mega_bytes(count);
	return result;
}

U64 mim_tera_bytes(U64 count)
{
	U64 result = 1024 * mim_giga_bytes(count);
	return result;
}

void* mim_alloc(U64 size)
{
	void* result = VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	return result;
}

B32 mim_free(void* data)
{
	B32 result = (B32)VirtualFree(data, 0, MEM_RELEASE);
	return result;
}

void mim_zero(void* data, U64 size)
{
	Assert(size > 8);
	__stosq(data, 0, size / 8);
}

U8* mim_arena_pointer(Mim_Arena* arena)
{
	U8* result = arena->base + arena->pos;
	return result;
}

Mim_Arena mim_arena_create(U64 size)
{
	Assert(size > sizeof(Mim_Arena));
	Mim_Arena arena = {0};
	U8* data = (U8*)mim_alloc(size);
	if (data)
	{
		arena.size = size;
		arena.pos = 0;
		arena.scratch_count = 0;
		arena.base = data;
		arena.reserved = false;
	}
	return arena;
}

B32 mim_arena_destroy(Mim_Arena* arena)
{
	Assert(arena);
	Assert(arena->base);
	Assert(!(arena->reserved));
	B32 result = mim_free(arena->base);
	arena->size = 0;
	arena->pos = 0;
	return result;
}

U8* mim_arena_push_size(Mim_Arena* arena, U64 size)
{
	Assert(arena->base);
	Assert(arena->pos + size <= arena->size);
	Assert(!(arena->reserved));
	U8* result = arena->base + arena->pos;
	arena->pos += size;
	return result;
}

U64 mim_arena_push_u32(Mim_Arena* arena, U32 value)
{
	U64 size = sizeof(U32);
	U32* dst = (U32*)mim_arena_push_size(arena, size);
	*dst = value;
	return size;
}

U64 mim_arena_push_u64(Mim_Arena* arena, U64 value)
{
	U64 size = sizeof(U64);
	U64* dst = (U64*)mim_arena_push_size(arena, size);
	*dst = value;
	return size;
}

U64 mim_arena_push_value(Mim_Arena* arena, U64 size, U8* value)
{
	U8* dst = (U8*)mim_arena_push_size(arena, size);
	mim_copy(dst, value, size);
	return size;
}

U64 mim_arena_push_buffer(Mim_Arena* arena, U64 size, U8* data)
{
	mim_arena_push_u64(arena, size);
	mim_arena_push_value(arena, size, data);
	return size + sizeof(size);
}

void mim_arena_pop_size(Mim_Arena* arena, U64 size)
{
	Assert(arena);
	Assert(arena->base);
	Assert(arena->pos >= size);
	Assert(!(arena->reserved));
	arena->pos -= size;
}

void mim_arena_reset(Mim_Arena* arena)
{
	Assert(arena);
	Assert(arena->base);
	Assert(!(arena->reserved));
	arena->pos = 0;
}

U8* mim_arena_reserve(Mim_Arena* arena)
{
	Assert(arena);
	Assert(!(arena->reserved));
	arena->reserved = true;
	U8* result = arena->base + arena->pos;
	return result;
}

void mim_arena_commit(Mim_Arena* arena, U64 size)
{
	Assert(arena);
	Assert(arena->reserved);
	Assert(arena->pos + size < arena->size);
	arena->reserved = false;
	arena->pos += size;
}

Mim_ScratchArena mim_scratch_begin(Mim_Arena* arena)
{
	Assert(arena);
	Assert(arena->base);
	Assert(!(arena->reserved));
	Mim_ScratchArena result;
	result.arena = arena;
	result.pos = arena->pos;
	arena->scratch_count++;
	return result;
}

void mim_scratch_end(Mim_ScratchArena* scratch)
{
	Assert(scratch);
	Assert(scratch->arena);
	Assert(scratch->arena->base);
	Assert((I64)scratch->arena->pos - (I64)scratch->pos >= 0);
	Mim_Arena* arena = scratch->arena;
	Assert(arena->scratch_count > 0);
	arena->pos = scratch->pos;
	arena->scratch_count--;
}

void mim_copy(void* dst, void* src, U64 size)
{
	Assert(dst);
	Assert(src);
	memcpy(dst, src, size);
}

U64 mim_align16(U64 value)
{
	U64 result = (value + 15) & ~15;
	return result;
}

Mim_WriteBuffer mim_begin_serializing(Mim_Arena* arena)
{
	Assert(arena);
	Mim_WriteBuffer write_buffer;
	write_buffer.size = arena->size;
	write_buffer.pos = 0;
	write_buffer.data = mim_arena_reserve(arena);
	write_buffer.arena = arena;
	return write_buffer;
}

void mim_end_serializing(Mim_WriteBuffer* write_buffer)
{
	Assert(write_buffer);
	Assert(write_buffer->arena);
	Assert(write_buffer->arena->reserved);
	Assert(write_buffer->pos <= write_buffer->size);
	mim_arena_commit(write_buffer->arena, write_buffer->pos);
	write_buffer->pos = 0;
}

void mim_write_data(Mim_WriteBuffer* buffer, U64 size, U8* data)
{
	Assert(buffer);
	Assert(buffer->data);
	Assert(data);
	Assert(buffer->pos <= buffer->size);
	Assert(buffer->pos + size <= buffer->size);
	U8* dst = buffer->data + buffer->pos;
	mim_copy(dst, data, size);
	buffer->pos += size;
}

void mim_write_u32(Mim_WriteBuffer* buffer, U32 u32)
{
	Assert(buffer);
	Assert(buffer->data);
	Assert(buffer->pos <= buffer->size);
	Assert(buffer->pos + sizeof(U32) <= buffer->size);
	U32* dst = (U32*)(buffer->data + buffer->pos);
	*dst = u32;
	buffer->pos += sizeof(U32);
}

void mim_write_u64(Mim_WriteBuffer* buffer, U64 u64)
{
	Assert(buffer);
	Assert(buffer->data);
	Assert(buffer->pos <= buffer->size);
	Assert(buffer->pos + sizeof(U64) <= buffer->size);
	U64* dst = (U64*)(buffer->data + buffer->pos);
	*dst = u64;
	buffer->pos += sizeof(U64);
}

void mim_write_buffer(Mim_WriteBuffer* buffer, U64 size, U8* data)
{
	mim_write_u64(buffer, size);
	mim_write_data(buffer, size, data);
}

Mim_ReadBuffer mim_create_read_buffer(Mim_Buffer* buffer)
{
	Mim_ReadBuffer result;
	result.pos = 0;
	result.size = buffer->size;
	result.data = buffer->data;
	return result;
}

B32 mim_read_data(U8* out, Mim_ReadBuffer* buffer, U64 size)
{
	Assert(out);
	Assert(buffer);
	B32 result = false;
	if (buffer->pos <= buffer->size && (buffer->pos + size) <= buffer->size)
	{
		U8* data = buffer->data + buffer->pos;
		mim_copy(out, data, size);
		buffer->pos += size;
		result = true;
	}
	return result;
}

B32 mim_read_u32(U32* out, Mim_ReadBuffer* buffer)
{
	Assert(out);
	Assert(buffer);
	B32 result = false;
	U64 size = sizeof(U32);
	if (buffer->pos <= buffer->size && (buffer->pos + size) <= buffer->size)
	{
		U32* data = (U32*)(buffer->data + buffer->pos);
		*out = *data;
		buffer->pos += size;
		result = true;
	}
	return result;
}

B32 mim_read_u64(U64* out, Mim_ReadBuffer* buffer)
{
	Assert(out);
	Assert(buffer);
	B32 result = false;
	U64 size = sizeof(U64);
	if (buffer->pos <= buffer->size && (buffer->pos + size) <= buffer->size)
	{
		U64* data = (U64*)(buffer->data + buffer->pos);
		*out = *data;
		buffer->pos += size;
		result = true;
	}
	return result;
}

B32 mim_read_buffer(U64* size, U8* data, Mim_ReadBuffer* buffer)
{
	Assert(size);
	Assert(data);
	Assert(buffer);
	B32 result = false;
	if (mim_read_u64(size, buffer) && mim_read_data(data, buffer, *size))
	{
		result = true;
	}
	return result;
}