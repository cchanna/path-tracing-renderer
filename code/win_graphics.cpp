// NOTE(cch): platform-specific code for the renderer

#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#include "graphics.h"
#include "graphics.cpp"
#include "win_graphics.h"




internal void
inline void
Win_Write8(WIN_GIF_WRITER *writer, uint8 byte)
{
	fputc(byte, writer->file);
}

inline void
Win_Write16(WIN_GIF_WRITER *writer, uint16 value)
{
	fputc((uint8) value, writer->file);
	fputc((uint8) (value >> 8), writer->file);
}

inline void
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
	while (writer->bits_written >= 8)
	{
		writer->data_block[writer->block_length++] = writer->byte;
		writer->byte = 0;
		writer->byte = (uint8)(code >> (8 - writer->bits_written + current_code_size));
		writer->bits_written -= 8;
		if (writer->block_length == 255)
		{
			Win_WriteBlock(writer);
		}
	}
}

inline void
Win_CopyColor(WIN_COLOR *color1, WIN_COLOR *color2)
{
	color1->red = color2->red;
	color1->green = color2->green;
	color1->blue = color2->blue;
}

internal int
Win_GetSubcube(WIN_COLOR_CUBE_HEAD *c, int cube, int level, WIN_COLOR color)
{
	WIN_COLOR_CUBE *cubes = c->cubes;
	int red = (color.red >> level) & 1;
	int green = (color.green >> level) & 1;
	int blue = (color.blue >> level) & 1;
	int subcube = cubes[cube].subcube[red][green][blue];
	if (subcube == 0)
	{
		subcube = c->size;
		cubes[cube].subcube[red][green][blue] = subcube;
	}
	return subcube;
}

internal void
Win_CubeInsert(WIN_COLOR_CUBE_HEAD *c, int cube, int level, WIN_COLOR color, int amount)
{
	WIN_COLOR_CUBE *cubes = c->cubes;
	if (cube == -1)
	{
		cube = 0;
	}
	else
	{
		cube = Win_GetSubcube(c,cube,level,color);
	}
	if (cubes[cube].amount == 0)
	{
		Win_CopyColor(&(cubes[cube].color), &color);
		c->size++;
	}
	cubes[cube].amount += amount;
	if (cubes[cube].has_subcubes)
	{
		Win_CubeInsert(c,cube,level - 1,color,1);
	}
	else
	{
		if (
			cubes[cube].color.red == color.red &&
			cubes[cube].color.green == color.green &&
			cubes[cube].color.blue == color.blue
		){
			// NOTE(cch): do nothing
		}
		else
		{
			cubes[cube].has_subcubes = TRUE;
			Win_CubeInsert(c,cube,level - 1,cubes[cube].color,cubes[cube].amount - amount);
			Win_CubeInsert(c,cube,level - 1,color,1);
		}
	}
}

inline void
Win_CubeInsert(WIN_COLOR_CUBE_HEAD *c, WIN_COLOR color)
{
	Win_CubeInsert(c,-1,7,color,1);
}

internal void
Win_SortCubes(WIN_COLOR_CUBE *cubes, int cube)
{
	int highest_cube = 0;
	int highest_amount = 0;
	int total_amount = 0;
	if (cubes[cube].has_subcubes)
	{
		for (int r = 0; r < 2; r++)
		{
			for (int g = 0; g < 2; g++)
			{
				for (int b = 0; b < 2; b++)
				{
					int subcube = cubes[cube].subcube[r][g][b];
					if (subcube > 0)
					{
						Win_SortCubes(cubes,subcube);
						total_amount += cubes[subcube].amount;
						if (cubes[subcube].amount > highest_amount)
						{
							highest_cube = subcube;
							highest_amount = cubes[subcube].amount;
						}
					}
				}
			}
		}
		Assert(cubes[cube].amount == total_amount);
		Win_CopyColor(&(cubes[cube].color),&(cubes[highest_cube].color));
	}
}

inline void
Win_SortCubes(WIN_COLOR_CUBE *cubes)
{
	Win_SortCubes(cubes,0);
}


internal uint8
Win_CubeSearch(WIN_COLOR_CUBE_HEAD *c, int cube, int level, WIN_COLOR color)
{
	WIN_COLOR_CUBE *cubes = c->cubes;
	if (cube == -1)
	{
		cube = 0;
	}
	else
	{
		int subcube = Win_GetSubcube(c,cube,level,color);
		Assert(cubes[subcube].has_subcubes || cubes[subcube].complete);
		cube = subcube;
	}
	if (cubes[cube].complete)
	{
		return cubes[cube].color_index;
	}
	else
	{
		return Win_CubeSearch(c,cube,level - 1, color);
	}
}

inline uint8
Win_CubeSearch(WIN_COLOR_CUBE_HEAD *c, WIN_COLOR color)
{
	return Win_CubeSearch(c,-1,7,color);
}

inline void
Win_HeapSwap(int *heap, int node_1, int node_2)
{
	int temp = heap[node_1];
	heap[node_1] = heap[node_2];
	heap[node_2] = temp;
}

internal void
Win_HeapPush(int *heap, int cube, WIN_COLOR_CUBE *cubes)
{
	heap[0]++;
	int current_node = heap[0];
	heap[current_node] = cube;
	int parent_node = current_node/2;
	while (current_node > 1 && cubes[heap[parent_node]].amount < cubes[heap[current_node]].amount)
	{
		Win_HeapSwap(heap,current_node,parent_node);
		current_node = parent_node;
		parent_node = current_node/2;
	}
}

internal int
Win_HeapPop(int *heap, WIN_COLOR_CUBE *cubes)
{
	int current_node = 1;
	int return_value = heap[current_node];
	heap[current_node] = heap[heap[0]];
	heap[heap[0]] = 0;
	heap[0]--;
	while (current_node <= heap[0])
	{
		int child_1 = current_node * 2;
		if (child_1 <= heap[0])
		{
			int greater_child = child_1;
			int child_2 = (current_node * 2) + 1;
			if (child_2 <= heap[0] && cubes[heap[child_2]].amount > cubes[heap[child_1]].amount)
			{
				greater_child = child_2;
			}
			Win_HeapSwap(heap,current_node,greater_child);
			current_node = greater_child;
		}
		else
		{
			break;
		}
	}
	return return_value;
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
	int pixels_in_frame = frame.width * frame.height;
	int pixels_in_video = pixels_in_frame * frame_count;
	int frame_memory_size = pixels_in_frame * frame.bytes_per_pixel;
	frame.memory = VirtualAlloc(0, frame_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	int video_memory_size = pixels_in_video * frame.bytes_per_pixel;
	video = (uint8 *) VirtualAlloc(0, video_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

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
	{
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
	}
	VirtualFree(frame.memory,0,MEM_RELEASE);
	VirtualFree(memory_block,0,MEM_RELEASE);

////////////////////////////////////////////////////////////////////////////////
	// making a gif //
////////////////////////////////////////////////////////////////////////////////

	int video_index_memory_size = pixels_in_video;
	uint8 *video_index = (uint8 *) VirtualAlloc(0, video_index_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	int index_buffer_size = 8;
	uint16 *index_buffer = (uint16 *) VirtualAlloc(0, sizeof(uint16) * index_buffer_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	int color_heap[COLOR_TABLE_SIZE + 1] = {};
	uint16 color_heap_size = 0;

	WIN_COLOR color_table[COLOR_TABLE_SIZE] = {};
	int num_colors = 0;

	WIN_COLOR_CUBE_HEAD color_cube = {};
	color_cube.cubes = (WIN_COLOR_CUBE *) VirtualAlloc(0, sizeof(WIN_COLOR_CUBE) * pixels_in_video * 8, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	int cube_size = 0;

	// NOTE(cch): build the color cube
	{
		uint8 *pixel = video;
		int current_pixel = 0;
		for (int i = 0; i < frame_count * frame.height * frame.width; i++)
		{
			WIN_COLOR color;
			color.red = *pixel++;
			color.green = *pixel++;
			color.blue = *pixel++;
			*pixel++;
			Win_CubeInsert(&color_cube,color);
		}
		Win_SortCubes(color_cube.cubes);
	}
	{
		Win_HeapPush(color_heap, 0, color_cube.cubes);
		while (color_heap[0] + num_colors < COLOR_TABLE_SIZE - 8)
		{
			int cube = Win_HeapPop(color_heap, color_cube.cubes);
			if (color_cube.cubes[cube].has_subcubes)
			{
				for (int r = 0; r < 2; r++)
				{
					for (int g = 0; g < 2; g++)
					{
						for (int b = 0; b < 2; b++)
						{
							int subcube = color_cube.cubes[cube].subcube[r][g][b];
							if (subcube > 0)
							{
								Win_HeapPush(color_heap, subcube, color_cube.cubes);
							}
						}
					}
				}
			}
			else
			{
				color_cube.cubes[cube].complete = TRUE;
				color_cube.cubes[cube].color_index = (uint8) num_colors;
				Win_CopyColor(&color_table[num_colors], &(color_cube.cubes[cube].color));
				num_colors++;
			}
		}
		while (color_heap[0] > 0)
		{
			int cube = Win_HeapPop(color_heap, color_cube.cubes);
			color_cube.cubes[cube].complete = TRUE;
			color_cube.cubes[cube].color_index = (uint8) num_colors;
			Win_CopyColor(&color_table[num_colors], &(color_cube.cubes[cube].color));
			num_colors++;
			Assert(num_colors <= 256);
		}
	}
	{
		uint8 *pixel = video;
		uint8 *pixel_index = video_index;
		int current_pixel = 0;
		for (int i = 0; i < frame_count * frame.height * frame.width; i++)
		{
			WIN_COLOR color;
			color.red = *pixel++;
			color.green = *pixel++;
			color.blue = *pixel++;
			*pixel++;
			uint8 index = Win_CubeSearch(&color_cube,color);
			*pixel_index++ = index;
		}

	}

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
	for (int i = 0; i < COLOR_TABLE_SIZE; i++)
	{
		Win_Write8(&w,color_table[i].red);
		Win_Write8(&w,color_table[i].green);
		Win_Write8(&w,color_table[i].blue);
	}

	// NOTE(cch): graphics control extension
	Win_Write8(&w,0x21); // NOTE(cch): extension introducer
	Win_Write8(&w,0xF9); // NOTE(cch): graphic control label
	Win_Write8(&w,0x04); // NOTE(cch): byte size
	Win_Write8(&w,0x01); // NOTE(cch): packaged field -- if transparency set to 01
	Win_Write16(&w,frame.delay);
	Win_Write8(&w,0x00); // NOTE(cch): transparent color index
	Win_Write8(&w,0x00); // NOTE(cch): block terminator

	for (int current_frame = 0; current_frame < frame_count; current_frame++)
	{
		// NOTE(cch): image descriptor
		Win_Write8(&w,0x2C);
		Win_Write16(&w,0);   // NOTE(cch): image left
		Win_Write16(&w,0);   // NOTE(cch): image top
		Win_Write16(&w,frame.width);  // NOTE(cch): image width
		Win_Write16(&w,frame.height);  // NOTE(cch): image height
		Win_Write8(&w,0x00); // NOTE(cch): block terminator

		// NOTE(cch): image data
		uint8 lzw_minimum_code_size = COLOR_TABLE_SIZE_COMPRESSED + 1;
		Win_Write8(&w,lzw_minimum_code_size);


		// NOTE(cch): creating the code stream
		uint16 table_position = COLOR_TABLE_SIZE; // NOTE(cch): the code table
		uint16 clear_code = table_position++;
		uint16 end_of_information_code = table_position++;
		int i = 0;
		uint8 current_code_size = 0;
		while (i < (frame.width * frame.height))
		{
			Win_WriteCode(&w, clear_code, current_code_size);
			current_code_size = lzw_minimum_code_size + 1;
			uint16 index_buffer_length = 0;
			index_buffer[index_buffer_length++] = video_index[i++];
			int code_table_size = 0;

			uint16 previous_code = index_buffer[0];
			while (i < (frame.width * frame.height))
			{

				if (i >= 213*142 + 104)
				{
					char Buffer[256];
					sprintf_s(Buffer, "%d\n, %d\n", i % 220, i / 220);
					OutputDebugStringA(Buffer);
				}
				if (index_buffer_length >= index_buffer_size)
				{
					index_buffer_size = index_buffer_size << 1;
					uint16 *temp_index_buffer = (uint16 *) VirtualAlloc(0, sizeof(uint16) * index_buffer_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
					for (int index = 0; index < index_buffer_length; index++)
					{
						temp_index_buffer[index] = index_buffer[index];
					}
					VirtualFree(index_buffer,0,MEM_RELEASE);
					index_buffer = temp_index_buffer;
				}
				index_buffer[index_buffer_length++] = video_index[i++];
				uint16 code;
				for (code = 0; code < code_table_size; code++)
				{
					int index = 0;
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
				if (code == code_table_size)
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
					if (code_table_size >= 2000)
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
	VirtualFree(index_buffer,0,MEM_RELEASE);

	// NOTE(cch): trailer
	Win_Write8(&w,0x3B);

    fclose(w.file);

	return(0);
}
