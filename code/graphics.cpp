// NOTE(cch): where the magic happens

internal void
InitializeMemory(MEMORY* memory, FRAME* frame)
{
	*memory = {};
	*frame = {};

	memory->permanent_storage_size = Megabytes(1);
	memory->transient_storage_size = Kilobytes(1);


	frame->width = 800;
	frame->height = 800;
	frame->color_depth_bytes = 1;
	frame->bytes_per_pixel = frame->color_depth_bytes * 4;
	frame->pitch = frame->bytes_per_pixel * frame->width;
	frame->delay = 42;
}

internal bool32
GetNextFrame(MEMORY* memory, FRAME* frame)
{
	STATE *state = (STATE *) memory->permanent_storage;
	if (state->frame_count > 0)
	{
		return FALSE;
	}
	uint8 *pixel = (uint8 *) frame->memory;
	for (uint64 y = 0; y < frame->height; y++)
	{
		for (uint64 x = 0; x < frame->width; x++)
		{
			*pixel++ = 0xFF; // NOTE(cch): red
			*pixel++ = 0xAA; // NOTE(cch): green
			*pixel++ = 0xAF; // NOTE(cch): blue
			*pixel++;        // NOTE(cch): unused alpha layer
		}
	}
	state->frame_count++;
	return TRUE;
}
