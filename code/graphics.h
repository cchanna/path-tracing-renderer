#if !defined (GRAPHICS_H)


#define Kilobytes(Value) (Value*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))


#define Pi32 3.1415926539f
#define internal static
#define local_persist static
#define global_variable static


struct MEMORY
{
	// bool32 is_initialized;

	uint64 permanent_storage_size;
	void *permanent_storage;

	uint64 transient_storage_size;
	void *transient_storage;
};

struct FRAME
{
	void * memory;
    uint32 width;
    uint32 height;
    uint32 pitch;
	uint32 bytes_per_pixel;
	uint32 delay;
};

internal void
Initialize(MEMORY* memory, FRAME* frame);

// internal bool32
// GetNextFrame(MEMORY* memory, FRAME* frame);

#include "graphics.cpp"

#define GRAPHICS_H
#endif
