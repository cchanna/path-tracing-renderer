// NOTE(cerisa): platform-specific code for the renderer

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

int main(int argc, const char* argv[])
{
	bool test = FALSE;

	MEMORY memory = {};
	FRAME frame = {};
	uint8 * image = {};

	InitializeMemory(&memory, &frame);

	int frame_memory_size = frame.width * frame.height * frame.bytes_per_pixel;
	frame.memory = VirtualAlloc(0, frame_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	int image_size = frame.width * frame.height * WIN_BYTES_PER_PIXEL;
	image = (uint8 *)VirtualAlloc(0, image_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

#if GRAPHICS_INTERNAL
	LPVOID base_address = 0;
#else
	LPVOID base_address = Terabytes(2);
#endif

	uint64 total_size = memory.permanent_storage_size + memory.transient_storage_size;
	void * memory_block = VirtualAlloc(base_address, (size_t)total_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	memory.permanent_storage = memory_block;
	memory.transient_storage = ((uint8 *)memory_block + memory.permanent_storage_size);

	while (GetNextFrame(&memory, &frame))
	{
		uint8 *pixel = image;
		uint8 *source_pixel = (uint8 *)(((uint64)frame.memory) + frame.color_depth_bytes - 1);
		// NOTE(cerisa): tricky conversions to allow for a greater color depth
		// in the renderer than in the output gif, and to accomodate for
		// little-endian memory
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
		const uint8 * output_image = (uint8 *)image;
		GifWriter writer;
		test = GifBegin(&writer, "test.gif", frame.width, frame.height, frame.delay);
		test = GifWriteFrame(&writer, output_image, frame.width, frame.height, frame.delay);

	}
	return(0);
}
