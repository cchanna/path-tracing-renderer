internal void
Initialize(MEMORY* memory, FRAME* frame)
{
	*memory = {};
	*frame = {};

	memory->permanent_storage_size = Megabytes(1);
	memory->transient_storage_size = Kilobytes(1);


	frame->width = 4;
	frame->height = 4;
	frame->bytes_per_pixel = 32;
	frame->pitch = frame->bytes_per_pixel * frame->width;
	frame->delay = 42;
}

// internal bool32
// GetNextFrame(MEMORY* memory, FRAME* frame)
// {
// 	if (!memory)
// 	{
//
// 	}
// 	if (!frame)
// 	{
// 		frame->width =
// 	}
// 	return TRUE;
// }
