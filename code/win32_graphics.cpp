// platform-specific code for the renderer
// devenv ..\..\build\win32_graphics.exe -- load VS

// #include win32_graphics.h
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

#include "win32_graphics.h"

#define Kilobytes(Value) (Value*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

int CALLBACK WinMain(
 HINSTANCE Instance,
 HINSTANCE PrevInstance,
 LPSTR CmdLine,
 int ShowCode)
{
	bool test = FALSE;
	uint32 width = 10;
	uint32 height = 10;
	uint32 delay = 42;

	Memory memory = {};

#if GRAPHICS_INTERNAL
	LPVOID baseAddress = 0;
#else
	LPVOID baseAddress = Terabytes(2);
#endif

	memory.permanentStorageSize = Megabytes(1);
	memory.transientStorageSize = Kilobytes(1);

	uint64 totalSize = memory.permanentStorageSize + memory.transientStorageSize;
	void * memoryBlock = VirtualAlloc(baseAddress, (size_t)totalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	memory.permanentStorage = memoryBlock;
	memory.transientStorage = ((uint8 *)memoryBlock + memory.permanentStorageSize);


	uint8 * image = (uint8 *)memory.permanentStorage;
	for (int i = 0; i < 100; i++)
	{
		image[i] = (uint8)i;
	}

	const uint8 * nextImage = image;

	GifWriter writer;
	test = GifBegin(&writer, "test.gif", width, height, delay);
	test = GifWriteFrame(&writer, nextImage, width, height, delay);

	return(0);
}
