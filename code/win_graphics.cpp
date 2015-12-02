// platform-specific code for the renderer
// devenv ..\..\build\win_graphics.exe -- load VS

// #include win_graphics.h
// #include graphics.h

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

// internal WinCompressColor(uint32)

int main(int argc, const char* argv[])
{
	bool test = FALSE;
	uint32 delay = 42;

	MEMORY memory = {};
	FRAME frame = {};
	uint8 * image = {};

	Initialize(&memory, &frame);

	int frame_memory_size = frame.width * frame.height * frame.bytes_per_pixel;
	frame.memory = VirtualAlloc(0, frame_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	int image_size = frame.width * frame.height * WIN_COLOR_DEPTH_BYTES;
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

	uint8 *row = image;
	for (uint64 y = 0; y < frame.height; y++)
	{
		uint8 *pixel = row;
		for(uint64 x = 0; x < frame.width; x++)
		{
			*pixel++ = 0xFF;
			*pixel++ = 0x00;
			*pixel++ = 0xFF;
			*pixel++;
		}
		row += frame.width * 4;
	}


	const uint8 * firstImage = (uint8 *)image;

	GifWriter writer;
	test = GifBegin(&writer, "test.gif", frame.width, frame.height, delay);
	test = GifWriteFrame(&writer, firstImage, frame.width, frame.height, delay);

	return(0);
}
