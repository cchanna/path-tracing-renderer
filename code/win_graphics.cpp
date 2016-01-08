// NOTE(cch): platform-specific code for the renderer

#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#include "graphics.h"
#include "graphics.cpp"
#include "win_graphics.h"

inline int64
Win_GetTime(void)
{
	LARGE_INTEGER result;
	QueryPerformanceCounter(&result);
	return result.QuadPart;
}

inline void
Win_StartTimer(WIN_TIMER *timer)
{
#if GRAPHICS_SLOW
	if (timer->last_tick == 0)
	{
		timer->last_tick = Win_GetTime();
		timer->time = 0;
		LARGE_INTEGER performance_frequency;
		QueryPerformanceFrequency(&performance_frequency);
		timer->performance_frequency = performance_frequency.QuadPart;
	}
	else
	{
		timer->last_tick = Win_GetTime();
	}
#endif
}

inline float
Win_StopTimer(WIN_TIMER *timer)
{
#if GRAPHICS_SLOW
	int64 current_time = Win_GetTime();
	timer->time += current_time - timer->last_tick;
	timer->last_tick = current_time;
	return ((float)timer->time) / ((float)timer->performance_frequency);
#else
	return 0.0f;
#endif
}

inline float
Win_GetTimerTime(WIN_TIMER *timer)
{
#if GRAPHICS_SLOW
	return ((float)timer->time) / ((float)timer->performance_frequency);
#else
	return 0.0f;
#endif
}


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

inline void
Win_WriteByteToBlock(WIN_GIF_WRITER *writer)
{
	writer->data_block[writer->block_length++] = writer->byte;
	writer->byte = 0;
	if (writer->block_length == 255)
	{
		Win_WriteBlock(writer);
	}
}

internal void
Win_WriteCode(WIN_GIF_WRITER *writer, int32 code, uint8 current_code_size)
{
	Assert(code >= 0 && code < 4096);
	writer->byte = (uint8)(writer->byte | (code << writer->bits_written));
	writer->bits_written += current_code_size;
	while (writer->bits_written >= 8)
	{
		Win_WriteByteToBlock(writer);
		writer->byte = (uint8)(code >> (8 - writer->bits_written + current_code_size));
		writer->bits_written -= 8;
	}
}

inline void
Win_CopyColor(WIN_COLOR *color1, WIN_COLOR *color2)
{
	color1->red = color2->red;
	color1->green = color2->green;
	color1->blue = color2->blue;
}

internal void
Win_CubeCopy(WIN_COLOR_CUBE *c_out, WIN_COLOR_CUBE *c_in)
{
	if (c_in->has_subcubes)
	{
		for (int r = 0; r < 2; r++)
		{
			for (int g = 0; g < 2; g++)
			{
				for (int b = 0; b < 2; b++)
				{
					c_out->subcube[r][g][b] = c_in->subcube[r][g][b];
				}
			}
		}
	}
	c_out->amount = c_in->amount;
	c_out->has_subcubes = c_in->has_subcubes;
	c_out->color_index = c_in->color_index;
	c_out->color = c_in->color;
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
		if (c->size == c->max_size)
		{
			c->max_size *= 2;
			WIN_COLOR_CUBE *cubes_tmp = (WIN_COLOR_CUBE *) VirtualAlloc(0, sizeof(WIN_COLOR_CUBE) * c->max_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			for (int i = 0; i < c->size; i++)
			{
				Win_CubeCopy(&(cubes_tmp[i]),&(cubes[i]));
			}
			VirtualFree(cubes, 0, MEM_RELEASE);
			c->cubes = cubes_tmp;
			cubes = c->cubes;
		}
		subcube = c->size;
		cubes[cube].subcube[red][green][blue] = subcube;
	}
	return subcube;
}

internal void
Win_CubeInsert(WIN_COLOR_CUBE_HEAD *c, int cube, int level, WIN_COLOR color, uint64 amount)
{
	if (cube == -1)
	{
		cube = 0;
	}
	else
	{
		cube = Win_GetSubcube(c,cube,level,color);
	}
	WIN_COLOR_CUBE *cubes = c->cubes;
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
	uint64 highest_amount = 0;
	uint64 total_amount = 0;
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
		cube = subcube;
	}
	if (cubes[cube].has_subcubes)
	{
		return Win_CubeSearch(c,cube,level - 1, color);
	}
	else
	{
		return cubes[cube].color_index;
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

// internal int
// Win_CodeTable_GetNextNode(WIN_CODES *codes)
// {
// 	Assert(codes->node_count <= codes->max_nodes);
// 	if (codes->node_count == codes->max_nodes)
// 	{
// 		codes->max_nodes *= 2;
// 		WIN_CODE_TABLE_NODE *temp_nodes = (WIN_CODE_TABLE_NODE *) VirtualAlloc(0, sizeof(WIN_CODE_TABLE_NODE) * codes->max_nodes, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
// 		for (int i = 0; i < codes->node_count; i++)
// 		{
// 			temp_nodes[i].next_node = codes->nodes[i].next_node;
// 			temp_nodes[i].value = codes->nodes[i].value;
// 		}
// 		VirtualFree(codes->nodes,0,MEM_RELEASE);
// 		codes->nodes = temp_nodes;
// 	}
// 	return codes->node_count++;
// }

internal uint8
Win_GetNextValue(WIN_IMAGE *image)
{
	int i = 0;
	i += ((image->current_pixel % image->width) + image->left); // NOTE(cch):x
	i += (((image->current_pixel / image->width) + image->top) * image->frame_width); // NOTE(cch):y
	image->current_pixel++;
	image->current_pixel %= image->width * image->height;
	return image->values[i];
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

	WIN_IMAGE image = {};
	uint64 frame_index_memory_size = pixels_in_frame;
	image.values = (uint8 *) VirtualAlloc(0, pixels_in_frame, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	image.previous_values = (uint8 *) VirtualAlloc(0, pixels_in_frame, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	image.frame_width = frame.width;
	image.frame_height = frame.height;

	int color_heap[COLOR_TABLE_SIZE + 1] = {};
	uint16 color_heap_size = 0;

	WIN_COLOR color_table[COLOR_TABLE_SIZE] = {};
	int num_colors = 0;

	WIN_COLOR_CUBE_HEAD color_cube = {};
	color_cube.max_size = 2048;
	color_cube.cubes = (WIN_COLOR_CUBE *) VirtualAlloc(0, sizeof(WIN_COLOR_CUBE) * color_cube.max_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	int cube_size = 0;

	uint64 code_tree_memory_size = sizeof(WIN_LZW_NODE) * MAX_CODE_TREE_SIZE;
	WIN_LZW_NODE *code_tree = (WIN_LZW_NODE *) VirtualAlloc(0,code_tree_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	// NOTE(cch): build the color cube
	{
		uint8 *pixel = video;
		int current_pixel = 0;
		for (int i = 0; i < pixels_in_video; i++)
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
				color_cube.cubes[cube].has_subcubes = FALSE;
				color_cube.cubes[cube].color_index = (uint8) num_colors;
				Win_CopyColor(&color_table[num_colors], &(color_cube.cubes[cube].color));
				num_colors++;
			}
		}
		while (color_heap[0] > 0)
		{
			int cube = Win_HeapPop(color_heap, color_cube.cubes);
			color_cube.cubes[cube].has_subcubes = FALSE;
			color_cube.cubes[cube].color_index = (uint8) num_colors;
			Win_CopyColor(&color_table[num_colors], &(color_cube.cubes[cube].color));
			num_colors++;
			Assert(num_colors <= 256);
		}
	}

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

	// NOTE(cch) application extension (animation)
	Win_Write8(&w,0x21); // NOTE(cch): extension introducer
	Win_Write8(&w,0xFF); // NOTE(cch): application extension label
	Win_Write8(&w,11); // NOTE(cch): eleven bytes follow
	Win_WriteString(&w,"NETSCAPE2.0");
	Win_Write8(&w,3); // NOTE(cch): three bytes follow
	Win_Write8(&w,0x01); // NOTE(cch): *shrug*
	Win_Write16(&w,0); // NOTE(cch): repeat forever
	Win_Write8(&w,0x00); // NOTE(cch): block terminator

	WIN_TIMER timer = {};
	uint8 *pixel = video;
	for (int current_frame = 0; current_frame < frame_count; current_frame++)
	{
		image.left = frame.width - 1;
		image.right = 0;
		image.top = frame.height - 1;
		image.bottom = 0;
		image.width;
		image.height;
		{
			for (int i = 0; i < pixels_in_frame; i++)
			{
				WIN_COLOR color;
				color.red = *pixel++;
				color.green = *pixel++;
				color.blue = *pixel++;
				*pixel++;
				uint8 index = Win_CubeSearch(&color_cube,color);
				image.values[i] = index;
				if (current_frame > 0)
				{
					if (image.values[i] != image.previous_values[i])
					{
						uint16 x = i % frame.width;
						if (x < image.left)
						{
							image.left = x;
						}
						if (x > image.right)
						{
							image.right = x;
						}
						uint16 y = (uint16) (i / frame.width);
						if (y < image.top)
						{
							image.top = y;
						}
						if (y > image.bottom)
						{
							image.bottom = y;
						}
					}
				}
			}
			if (current_frame > 0)
			{
				image.width = image.right - image.left + 1;
				image.height = image.bottom - image.top + 1;
			}
			else
			{
				image.left = 0;
				image.top = 0;
				image.width = frame.width;
				image.height = frame.height;
			}
		}
		// NOTE(cch): graphics control extension
		Win_Write8(&w,0x21); // NOTE(cch): extension introducer
		Win_Write8(&w,0xF9); // NOTE(cch): graphic control label
		Win_Write8(&w,0x04); // NOTE(cch): byte size
		uint8 packaged_field = 0;
		if (IMAGE_TRANSPARENT)
		{
			packaged_field = packaged_field | 1;
		}
		packaged_field = packaged_field | (1 << 2); // NOTE(cch): disposal method
		Win_Write8(&w,packaged_field);
		Win_Write16(&w,frame.delay);
		Win_Write8(&w,0x00); // NOTE(cch): transparent color index
		Win_Write8(&w,0x00); // NOTE(cch): block terminator

		// NOTE(cch): image descriptor
		Win_Write8(&w,0x2C);
		Win_Write16(&w,image.left);   // NOTE(cch): image left
		Win_Write16(&w,image.top);   // NOTE(cch): image top
		Win_Write16(&w,image.width);  // NOTE(cch): image width
		Win_Write16(&w,image.height);  // NOTE(cch): image height
		Win_Write8(&w,0x00); // NOTE(cch): no local color table

		// NOTE(cch): image data
		uint8 lzw_minimum_code_size = COLOR_TABLE_SIZE_COMPRESSED + 1;
		Win_Write8(&w,lzw_minimum_code_size);

		// NOTE(cch): creating the code stream
		uint16 tree_size = COLOR_TABLE_SIZE;
		uint16 clear_code = tree_size++;
		uint16 end_of_information_code = tree_size++;
		uint8 current_code_size = lzw_minimum_code_size + 1;
		Win_WriteCode(&w, clear_code, current_code_size);

		int32 current_code = Win_GetNextValue(&image);
		for (int i = 1; i < image.width * image.height; i++)
		{
			uint16 next_index = Win_GetNextValue(&image);
			Win_StartTimer(&timer);

			// NOTE(cch): search for matching code run in code tree
			if (code_tree[current_code].next[next_index])
			{
				current_code = code_tree[current_code].next[next_index];
			}
			else
			{
				Win_WriteCode(&w, current_code, current_code_size);
				if (tree_size == (1 << current_code_size))
				{
					current_code_size++;
				}
				code_tree[current_code].next[next_index] = tree_size++;

				// NOTE(cch): if the code table reaches the maximum size, we
				// start fresh with a new table
				Assert(tree_size <= MAX_CODE_TREE_SIZE)
				if (tree_size == MAX_CODE_TREE_SIZE)
				{
					Win_WriteCode(&w, clear_code, current_code_size);
					memset(code_tree,0,code_tree_memory_size);
					current_code_size = lzw_minimum_code_size + 1;
					tree_size = end_of_information_code + 1;
				}
				current_code = next_index;
			}
		}
		memset(code_tree,0,code_tree_memory_size);
		Win_WriteCode(&w, current_code, current_code_size);
		Win_WriteCode(&w, clear_code, current_code_size);
		Win_WriteCode(&w, end_of_information_code, current_code_size);
		if (w.bits_written > 0)
		{
			Win_WriteByteToBlock(&w);
			w.bits_written = 0;
		}
		if (w.block_length > 0)
		{
			Win_WriteBlock(&w);
		}
		Win_Write8(&w,0x00); //NOTE(cch): block terminator
		uint8 *values_tmp = image.previous_values;
		image.previous_values = image.values;
		image.values = values_tmp;

	}
#if GRAPHICS_SLOW
	{
		char Buffer[256];
		sprintf_s(Buffer, "%f", Win_GetTimerTime(&timer));
		OutputDebugStringA(Buffer);
	}
#endif
	VirtualFree(video,0,MEM_RELEASE);
	VirtualFree(image.values,0,MEM_RELEASE);
	VirtualFree(image.previous_values,0,MEM_RELEASE);
	VirtualFree(code_tree,0,MEM_RELEASE);
	VirtualFree(color_cube.cubes,0,MEM_RELEASE);

	// NOTE(cch): trailer
	Win_Write8(&w,0x3B);

    fclose(w.file);

	return(0);
}
