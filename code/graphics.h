#if !defined (GRAPHICS_H)

#define Kilobytes(Value) (Value*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define TAU_32 6.2831853071f
#define internal static
#define local_persist static
#define global_variable static

#include "matrix_3d.h"
#include "matrix_3d.cpp"

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
	void *memory;
    uint32 width;
    uint32 height;
    uint32 pitch;
	uint32 color_depth_bytes;
	uint32 bytes_per_pixel;
	uint32 delay;
	float dithering;
};

struct CAMERA
{
	float half_angle_x;
	float half_angle_y;
	float height;
	float width;
	float yon;
	VECTOR3D eye, coi, up;
};

struct STATE
{
	bool32 is_initialized;
	uint32 frame_count;
	CAMERA camera;
};

internal void
Initialize(MEMORY *memory, FRAME *frame);
// NOTE(cch): this sets up the MEMORY and FRAME structs so that the platform
// layer can know how much memory needs to be allocated for them. this way you
// can edit the properties of the image without touching win_graphics.cpp
// NOTE(cch): i guess this is a constructor? except without all the baggage that
// might come with that term

internal bool32
GetNextFrame(MEMORY *memory, FRAME *frame);

#define GRAPHICS_H
#endif
