// NOTE(cch): platform-specific code for the renderer

#include <windows.h>
#include <stdio.h>
#include <stdint.h>

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
	fputc(byte, writer->file);
}

internal void
Win_Write16(WIN_GIF_WRITER *writer, uint16 value)
{
	fputc((uint8) value, writer->file);
	fputc((uint8) (value >> 8), writer->file);
}

internal void
Win_WriteString(WIN_GIF_WRITER *writer, char *string)
{
	while(*string)
	{
		fputc(*(string++),writer->file);
	}
}

int main(int argc, const char* argv[])
{
	MEMORY memory = {};
	FRAME frame = {};
	uint8 * video = {};
	int frame_count = InitializeMemory(&memory, &frame);

	// NOTE(cch): the goal is for all allocations to be held exclusively in the
	// platform layer. this way it's easier to keep track of where it's
	// happening and is easily portable
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

	// NOTE(cch): the actual rendering, saved into a single massive video data block
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
	VirtualFree(frame.memory,0,MEM_RELEASE);
	VirtualFree(memory_block,0,MEM_RELEASE);


////////////////////////////////////////////////////////////////////////////////
	// making a gif //
////////////////////////////////////////////////////////////////////////////////


	WIN_CODE_TABLE_NODE *code_table = (WIN_CODE_TABLE_NODE *) VirtualAlloc(0, sizeof(WIN_CODE_TABLE_NODE) * 4096,MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	uint16 *code_stream = (uint16 *) VirtualAlloc(0, frame_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	// NOTE(cch): the size of a frame should be enough for the code stream,
	// seeing how it's a compression algorithm and thus should produce something
	// smaller
	WIN_GIF_WRITER w = {};
	w.file = 0;
	fopen_s(&(w.file), "test.gif", "wb");
	if (w.file == 0)
	{
		return 1;
	}

	// NOTE(cch): header
	Win_WriteString(&w,"GIF89a");

	// NOTE(cch): logical screen descriptor
	Win_Write16(&w,frame.width);
	Win_Write16(&w,frame.height);
	Win_Write8(&w,(1 << 7) | (1 << 4) | COLOR_TABLE_SIZE_COMPRESSED);
	Win_Write8(&w,0x00); // NOTE(cch): background color index
	Win_Write8(&w,0x00); // NOTE(cch): pixel aspect ratio

	// NOTE(cch): global color table
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
	for (int current_frame = 0; current_frame < frame_count; current_frame++)
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

		// NOTE(cch): creating the code stream
		uint16 table_position = COLOR_TABLE_SIZE; // NOTE(cch): the code table
		uint16 clear_code = table_position++;
		uint16 end_of_information_code = table_position++;
		int code_stream_length = 0;
		code_stream[code_stream_length] = clear_code;
		int i = 0;
		while (i < (frame.width * frame.height))
		{
			uint16 index_buffer[256] = {};
			uint16 index_buffer_length = 0;
			index_buffer[index_buffer_length++] = example_image[i++];
			int code_table_size = 0;

			uint16 previous_code = index_buffer[0];
			while (i < (frame.width * frame.height))
			{
				index_buffer[index_buffer_length++] = example_image[i++];
				int index = 0;
				for (uint16 code = 0; code < code_table_size; code++)
				{
					index = 0;
					if (index_buffer_length == code_table[code].length)
					{
						while ((index < index_buffer_length) && (index_buffer[index] == code_table[code].values[index]))
						{
							index++;
						}
						if (index == index_buffer_length)
						{
							previous_code = code + table_position;
							break;
						}
					}
				}
				if (index != index_buffer_length)
				{
					for (int index = 0; index < index_buffer_length; index++)
					{
						code_table[code_table_size].values[index] = index_buffer[index];
					}
					code_table[code_table_size].length = index_buffer_length;
					code_table_size++;
					code_stream[code_stream_length++] = previous_code;
					index_buffer[0] = index_buffer[index_buffer_length - 1];
					for (int index = 1; index < index_buffer_length; index++)
					{
						index_buffer[index] = 0;
					}
					index_buffer_length = 1;
					previous_code = index_buffer[0];

					// NOTE(cch): if the code table reaches the maximum size, we
					// start fresh with a new table
					if (code_table_size == 4096)
					{
						break;
					}
				}
			}
			code_stream[code_stream_length++] = previous_code;
		}
		code_stream[code_stream_length++] = end_of_information_code;
		VirtualFree(code_table,0,MEM_RELEASE);

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

    fclose(w.file);

	return(0);
}
