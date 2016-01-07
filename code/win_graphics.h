#if !defined (WIN_GRAPHICS_H)

struct WIN_FRAME
{
	void * memory;
};

#define COLOR_TABLE_SIZE_COMPRESSED 7
#define COLOR_TABLE_SIZE (2 << COLOR_TABLE_SIZE_COMPRESSED)

struct WIN_GIF_WRITER
{
	FILE *file;
	uint8 data_block[255];
	uint8 block_length;
	uint8 byte;
	uint8 bits_written;
};

struct WIN_COLOR
{
	uint8 red, green, blue;
};

#define MAX_CODE_TREE_SIZE 4096

struct WIN_LZW_NODE
{
	uint16 next[256];
};

struct WIN_TIMER
{
	int64 time;
	int64 last_tick;
	int64 performance_frequency;
};

struct WIN_COLOR_CUBE
{
	int subcube[2][2][2];
	int amount;
	bool32 has_subcubes;
	bool32 complete;
	uint8 color_index;
	WIN_COLOR color;
};

struct WIN_COLOR_CUBE_HEAD
{
	WIN_COLOR_CUBE *cubes;
	int size;
};



#define WIN_GRAPHCS_H
#endif
