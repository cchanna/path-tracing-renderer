#if !defined (WIN_GRAPHICS_H)

struct WIN_FRAME
{
	void * memory;
};

#define COLOR_TABLE_SIZE_COMPRESSED 1
#define COLOR_TABLE_SIZE (2 << COLOR_TABLE_SIZE_COMPRESSED)

struct WIN_GIF_WRITER
{
	uint8 * location;
	uint8 * cursor;
};

struct WIN_COLOR
{
	uint8 red, green, blue;
};

#define WIN_GRAPHCS_H
#endif
