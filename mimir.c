#include "mimir.h"

#if !defined(_WIN32)
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#endif

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

#if defined(_WIN32)
void* mim_alloc(U64 size)
{
	void* result = VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	return result;
}

B32 mim_free(void* data, U64 size)
{
	size;
	B32 result = (B32)VirtualFree(data, 0, MEM_RELEASE);
	return result;
}
#else
void* mim_alloc(U64 size)
{
	void* result = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	return result;
}

B32 mim_free(void* data, U64 size)
{
	B32 result = false;
	if (data)
	{
		munmap(data, size);
		result = true;
	}
	return result;
}
#endif

void mim_copy(void* dst, void* src, U64 size)
{
	Assert(dst);
	Assert(src);
	memcpy(dst, src, size);
}

void mim_fill(void* data, U64 size, U64 value)
{
	U64 count = size / 8;
#if defined(_WIN32)
	__stosq((U64*)data, value, count);
#else
	U64* arr = (U64*)data;
	for (U64 i = 0; i < count; i++)
	{
		arr[i] = value;
	}
#endif
}

void mim_zero(void* data, U64 size)
{
	mim_fill(data, size, 0);
	U64 count = size / 8;
	U8* bytes = (U8*)data;
	for (U64 i = count * 8; i < size; i++)
	{
		bytes[i] = 0;
	}
}

U64 mim_align16(U64 value)
{
	U64 result = (value + 15) & ~15;
	return result;
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
	B32 result = mim_free(arena->base, arena->size);
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

void mim_begin_serializing(Mim_WriteBuffer* write_buffer, Mim_Arena* arena)
{
	Assert(write_buffer);
	Assert(arena);
	Assert(arena->size >= arena->pos);
	write_buffer->size = arena->size - arena->pos;
	write_buffer->pos = 0;
	write_buffer->base = mim_arena_reserve(arena);
	write_buffer->arena = arena;
}

void mim_end_serializing(Mim_WriteBuffer* write_buffer)
{
	Assert(write_buffer);
	Assert(write_buffer->arena);
	Assert(write_buffer->arena->reserved);
	Assert(write_buffer->pos <= write_buffer->size);
	mim_arena_commit(write_buffer->arena, write_buffer->pos);
	write_buffer->size = write_buffer->pos;
	write_buffer->pos = 0;
}

U8* mim_write_buffer_pointer(Mim_WriteBuffer* write_buffer)
{
	Assert(write_buffer);
	Assert(write_buffer->base);
	Assert(write_buffer->pos <= write_buffer->size);
	U8* result = write_buffer->base + write_buffer->pos;
	return result;
}

// Internal
void mim_write_buffer_push_size(Mim_WriteBuffer* write_buffer, U64 size)
{
	Assert(write_buffer->pos + size <= write_buffer->size);
	write_buffer->pos += size;
}

void mim_write_data(Mim_WriteBuffer* write_buffer, U8* data, U64 size)
{
	Assert(data);
	Assert(write_buffer->arena->reserved);
	U8* dst = mim_write_buffer_pointer(write_buffer);
	mim_copy(dst, data, size);
	mim_write_buffer_push_size(write_buffer, size);
}

void mim_write_u32(Mim_WriteBuffer* write_buffer, U32 value)
{
	U32* dst = (U32*)mim_write_buffer_pointer(write_buffer);
	*dst = value;
	mim_write_buffer_push_size(write_buffer, sizeof(U32));
}

void mim_write_u64(Mim_WriteBuffer* write_buffer, U64 value)
{
	U64* dst = (U64*)mim_write_buffer_pointer(write_buffer);
	*dst = value;
	mim_write_buffer_push_size(write_buffer, sizeof(U64));
}

void mim_write_f32(Mim_WriteBuffer* write_buffer, F32 value)
{
	F32* dst = (F32*)mim_write_buffer_pointer(write_buffer);
	*dst = value;
	mim_write_buffer_push_size(write_buffer, sizeof(F32));
}

void mim_write_f64(Mim_WriteBuffer* write_buffer, F64 value)
{
	F64* dst = (F64*)mim_write_buffer_pointer(write_buffer);
	*dst = value;
	mim_write_buffer_push_size(write_buffer, sizeof(F64));
}

void mim_write_buffer(Mim_WriteBuffer* write_buffer, U8* data, U64 size)
{
	mim_write_u64(write_buffer, size);
	mim_write_data(write_buffer, data, size);
}

Mim_ReadBuffer mim_create_read_buffer(Mim_Buffer* buffer)
{
	Mim_ReadBuffer result;
	result.pos = 0;
	result.size = buffer->size;
	result.base = buffer->data;
	return result;
}

U8* mim_read_buffer_pointer(Mim_ReadBuffer* read_buffer)
{
	Assert(read_buffer);
	Assert(read_buffer->base);
	Assert(read_buffer->pos <= read_buffer->size);
	U8* result = read_buffer->base + read_buffer->pos;
	return result;
}

B32 mim_read_buffer_can_push_size(Mim_ReadBuffer* read_buffer, U64 size)
{
	B32 valid_pos = read_buffer->pos <= read_buffer->size;
	B32 result = valid_pos && (read_buffer->pos + size) <= read_buffer->size;
	return result;
}

B32 mim_read_data(U8* out, Mim_ReadBuffer* read_buffer, U64 size)
{
	Assert(out);
	Assert(read_buffer);
	B32 result = false;
	if (mim_read_buffer_can_push_size(read_buffer, size))
	{
		U8* data = read_buffer->base + read_buffer->pos;
		mim_copy(out, data, size);
		read_buffer->pos += size;
		result = true;
	}
	return result;
}

B32 mim_read_u32(U32* out, Mim_ReadBuffer* read_buffer)
{
	Assert(out);
	Assert(read_buffer);
	B32 result = false;
	U64 size = sizeof(U32);
	if (mim_read_buffer_can_push_size(read_buffer, size))
	{
		U32* data = (U32*)(read_buffer->base + read_buffer->pos);
		*out = *data;
		read_buffer->pos += size;
		result = true;
	}
	return result;
}

B32 mim_read_u64(U64* out, Mim_ReadBuffer* read_buffer)
{
	Assert(out);
	Assert(read_buffer);
	B32 result = false;
	U64 size = sizeof(U64);
	if (mim_read_buffer_can_push_size(read_buffer, size))
	{
		U64* data = (U64*)(read_buffer->base + read_buffer->pos);
		*out = *data;
		read_buffer->pos += size;
		result = true;
	}
	return result;
}

B32 mim_read_f32(F32* out, Mim_ReadBuffer* read_buffer)
{
	Assert(out);
	Assert(read_buffer);
	B32 result = false;
	U64 size = sizeof(F32);
	if (mim_read_buffer_can_push_size(read_buffer, size))
	{
		F32* data = (F32*)(read_buffer->base + read_buffer->pos);
		*out = *data;
		read_buffer->pos += size;
		result = true;
	}
	return result;
}

B32 mim_read_f64(F64* out, Mim_ReadBuffer* read_buffer)
{
	Assert(out);
	Assert(read_buffer);
	B32 result = false;
	U64 size = sizeof(F64);
	if (mim_read_buffer_can_push_size(read_buffer, size))
	{
		F64* data = (F64*)(read_buffer->base + read_buffer->pos);
		*out = *data;
		read_buffer->pos += size;
		result = true;
	}
	return result;
}

B32 mim_read_buffer(U8* output_data, U64* output_size, Mim_ReadBuffer* buffer)
{
	Assert(output_data);
	Assert(output_size);
	Assert(buffer);
	B32 result = mim_read_u64(output_size, buffer) && mim_read_data(output_data, buffer, *output_size);
	return result;
}
