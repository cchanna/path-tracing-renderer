// NOTE(cch): platform-specific code for the renderer

#include "libraries/gif.h"
#include <windows.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#include "win_graphics.h"
#include "graphics.h"
#include "graphics.cpp"

internal void
Win_Write8(WIN_GIF_WRITER *writer, uint8 byte)
{
	*(writer->cursor++) = byte;
}

internal void
Win_Write16(WIN_GIF_WRITER *writer, uint16 value)
{
	*(writer->cursor++) = (uint8) value;
	*(writer->cursor++) = (uint8) (value >> 8);
}

internal void
Win_WriteString(WIN_GIF_WRITER *writer, char *string)
{
	while(*string)
	{
		*(writer->cursor++) = *(string++);
	}
}

int main(int argc, const char* argv[])
{
	bool test = FALSE;

	MEMORY memory = {};
	FRAME frame = {};
	uint8 * video = {};
	int frame_count = InitializeMemory(&memory, &frame);

	// NOTE(cch): the goal is for all allocations to be in one place, and held
	// exclusively in the platform layer. memory leaks become impossible.
	int frame_memory_size = frame.width * frame.height * frame.bytes_per_pixel;
	frame.memory = VirtualAlloc(0, frame_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	int video_memory_size = frame_memory_size * frame_count;
	video = (uint8 *)VirtualAlloc(0, video_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

#if GRAPHICS_INTERNAL
	LPVOID base_address = 0;
#else
	LPVOID base_address = Terabytes(2);
#endif

	uint64 total_size = memory.permanent_storage_size + memory.transient_storage_size;
	void * memory_block = VirtualAlloc(base_address, (size_t)total_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	memory.permanent_storage = memory_block;
	memory.transient_storage = ((uint8 *)memory_block + memory.permanent_storage_size);

	WIN_CODE_TREE *code_tree = (WIN_CODE_TREE *) VirtualAlloc(0, sizeof(WIN_CODE_TREE),MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	uint8 *pixel = video;
	for (int i = 0; i < frame_count; i++)
	{
		GetNextFrame(&memory, &frame, i);
		uint8 *source_pixel = (uint8 *) frame.memory;
		for (uint64 y = 0; y < frame.height; y++)
		{
			for(uint64 x = 0; x < frame.width; x++)
			{
				*pixel++ = *source_pixel;
				source_pixel = (uint8 *)(((uint64) source_pixel) + (frame.color_depth_bytes));
				*pixel++ = *source_pixel;
				source_pixel = (uint8 *)(((uint64) source_pixel) + (frame.color_depth_bytes));
				*pixel++ = *source_pixel;
				source_pixel = (uint8 *)(((uint64) source_pixel) + (frame.color_depth_bytes));
				*pixel++;
				source_pixel = (uint8 *)(((uint64) source_pixel) + (frame.color_depth_bytes));
			}
		}
	}
	// uint16 *code_tree[256] = (uint16 **) memory.permanent_storage;
	WIN_GIF_WRITER w = {};
	w.cursor = (uint8 *) memory.permanent_storage;
	w.location = w.cursor;
	// NOTE(cch): header
	Win_WriteString(&w,"GIF89a");

	// NOTE(cch): logical screen descriptor
	Win_Write16(&w,frame.width);
	Win_Write16(&w,frame.height);
	Win_Write8(&w,(1 << 7) | (1 << 4) | COLOR_TABLE_SIZE_COMPRESSED);
	Win_Write8(&w,0x00); // NOTE(cch): background color index
	Win_Write8(&w,0x00); // NOTE(cch): pixel aspect ratio

	// NOTE(cch): global color table&
	WIN_COLOR color_table[COLOR_TABLE_SIZE] = {};
	color_table[0] = {0xFF,0xFF,0xFF};
	color_table[1] = {0xFF,0x00,0x00};
	color_table[2] = {0x00,0x00,0xFF};
	color_table[3] = {0x00,0x00,0x00};
	for (int i = 0; i < COLOR_TABLE_SIZE; i++)
	{
		Win_Write8(&w,color_table[i].red);
		Win_Write8(&w,color_table[i].green);
		Win_Write8(&w,color_table[i].blue);
	}

	// NOTE(cch): graphics control extension
	Win_Write8(&w,0x21); // NOTE(cch): background color index
	Win_Write8(&w,0xF9);
	Win_Write8(&w,0x04);
	Win_Write8(&w,0x00);
	Win_Write16(&w,frame.delay);
	Win_Write8(&w,0x00);
	Win_Write8(&w,0x00);

	uint8 example_image[100] = {
		1,1,1,1,1,2,2,2,2,2,
		1,1,1,1,1,2,2,2,2,2,
		1,1,1,1,1,2,2,2,2,2,
		1,1,1,0,0,0,0,2,2,2,
		1,1,1,0,0,0,0,2,2,2,
		2,2,2,0,0,0,0,1,1,1,
		2,2,2,0,0,0,0,1,1,1,
		2,2,2,2,2,1,1,1,1,1,
		2,2,2,2,2,1,1,1,1,1,
		2,2,2,2,2,1,1,1,1,1,
	};
	for (int i = 0; i < frame_count; i++)
	{
		// NOTE(cch): image descriptor
		Win_Write8(&w,0x2C);
		Win_Write16(&w,0);   // NOTE(cch): image left
		Win_Write16(&w,0);   // NOTE(cch): image top
		Win_Write16(&w,10);  // NOTE(cch): image width
		Win_Write16(&w,10);  // NOTE(cch): image height
		Win_Write8(&w,0x00); // NOTE(cch): block terminator

		// NOTE(cch): image data
		uint8 lzw_minimum_code_size = COLOR_TABLE_SIZE_COMPRESSED + 1;
		Win_Write8(&w,lzw_minimum_code_size);

		int code_cursor = 0;
		uint8 color = 0;
		for (color = 0; color < COLOR_TABLE_SIZE; color++ )
		{
			code_tree->tree[code_cursor++][0];
		}
		code_cursor += 2;

		Win_Write8(&w,0x16); // NOTE(cch): sub-block size

		Win_Write8(&w,0x8C);
		Win_Write8(&w,0x2D);
		Win_Write8(&w,0x99);
		Win_Write8(&w,0x87);
		Win_Write8(&w,0x2A);
		Win_Write8(&w,0x1C);
		Win_Write8(&w,0xDC);
		Win_Write8(&w,0x33);
		Win_Write8(&w,0xA0);
		Win_Write8(&w,0x02);
		Win_Write8(&w,0x75);
		Win_Write8(&w,0xEC);
		Win_Write8(&w,0x95);
		Win_Write8(&w,0xFA);
		Win_Write8(&w,0xA8);
		Win_Write8(&w,0xDE);
		Win_Write8(&w,0x60);
		Win_Write8(&w,0x8C);
		Win_Write8(&w,0x04);
		Win_Write8(&w,0x91);
		Win_Write8(&w,0x4C);
		Win_Write8(&w,0x01);

		Win_Write8(&w,0x00); //NOTE(cch): block terminator
	}

	// NOTE(cch): trailer
	Win_Write8(&w,0x3B);

	DWORD bytes_to_write = (DWORD)(w.cursor - w.location);
	HANDLE file_handle = CreateFileA("test.gif", GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);

	if (file_handle != INVALID_HANDLE_VALUE) {
		DWORD bytes_written;
		bool32 result;
		// NOTE: File read successfully
		LPCVOID *buffer = (LPCVOID *) w.location;
		if(WriteFile(file_handle, buffer, bytes_to_write, &bytes_written, NULL))
		{
			result = (bytes_written == bytes_to_write);
		}
		else
		{
			// TODO: logging
		}
		CloseHandle(file_handle);
	}
	else
	{
		// TODO: logging
	}

	return(0);
}
