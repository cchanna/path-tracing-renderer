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

internal void
Win_WriteBlock(WIN_GIF_WRITER *writer)
{
	Win_Write8(writer,writer->block_length);
	for (int i = 0; i < writer->block_length; i++)
	{
		Win_Write8(writer, writer->data_block[i]);
	}
	writer->block_length = 0;
}

internal void
Win_WriteCode(WIN_GIF_WRITER *writer, uint16 code, uint8 current_code_size)
{
	writer->byte = (uint8)(writer->byte | (code << writer->bits_written));
	writer->bits_written += current_code_size;
	if (writer->bits_written >= 8)
	{
		writer->data_block[writer->block_length++] = writer->byte;
		writer->byte = 0;
		writer->byte = (uint8)(code >> (8 - writer->bits_written + current_code_size));
		writer->bits_written %= 8;
		if (writer->block_length == 255)
		{
			Win_WriteBlock(writer);
		}
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
	// TODO(cch): actually compute the global color table
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

		uint8 current_code_size = lzw_minimum_code_size + 1;

		// NOTE(cch): creating the code stream
		uint16 table_position = COLOR_TABLE_SIZE; // NOTE(cch): the code table
		uint16 clear_code = table_position++;
		uint16 end_of_information_code = table_position++;
		Win_WriteCode(&w, clear_code, current_code_size);
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
					Win_WriteCode(&w, previous_code, current_code_size);

					for (int index = 0; index < index_buffer_length; index++)
					{
						code_table[code_table_size].values[index] = index_buffer[index];
					}
					code_table[code_table_size].length = index_buffer_length;
					if ((code_table_size + table_position) == (1 << current_code_size))
					{
						current_code_size++;
					}
					code_table_size++;

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
			Win_WriteCode(&w, previous_code, current_code_size);
		}
		Win_WriteCode(&w, end_of_information_code, current_code_size);
		if (w.block_length > 0)
		{
			Win_WriteBlock(&w);
		}
		Win_Write8(&w,0x00); //NOTE(cch): block terminator
	}
	VirtualFree(video,0,MEM_RELEASE);
	VirtualFree(code_table,0,MEM_RELEASE);

	// NOTE(cch): trailer
	Win_Write8(&w,0x3B);

    fclose(w.file);

	return(0);
}
