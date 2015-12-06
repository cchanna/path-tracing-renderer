// NOTE(cch): where the magic happens

internal void
InitializeMemory(MEMORY *memory, FRAME *frame)
{
	*memory = {};
	*frame = {};

	memory->permanent_storage_size = Megabytes(1);
	memory->transient_storage_size = Kilobytes(1);


	frame->width = 500;
	frame->height = 500;
	frame->color_depth_bytes = 1;
	frame->bytes_per_pixel = frame->color_depth_bytes * 4;
	frame->pitch = frame->bytes_per_pixel * frame->width;
	frame->delay = 50;
	frame->dithering = 7.0f;
}

internal float
Raytrace(uint8 rgb[3], VECTOR3D *eye, VECTOR3D *vector, STATE *state, float dithering)
{
	float sphere_mtx[4][4],sphere_inv[4][4];
	Matrix3D_GetIdentity(sphere_mtx);
	Matrix3D_GetIdentity(sphere_inv);
	Matrix3D_Translate(sphere_mtx,sphere_inv,0.0f,2.0f,0.0f);
	VECTOR3D eye_object = {};
	VECTOR3D vector_object = {};
	Matrix3D_MultiplyVector(&eye_object,sphere_inv,sphere_mtx,eye);
	Matrix3D_MultiplyVector(&vector_object,sphere_inv,sphere_mtx,vector);
	float distance = 0;
	{
		// NOTE(cch): intersection with a sphere
		// NOTE(cch): quadratic formula
		float a = vector_object.x*vector_object.x + vector_object.y*vector_object.y + vector_object.z*vector_object.z;
		float b = 2*((eye_object.x*vector_object.x) + (eye_object.y*vector_object.y) + (eye_object.z*vector_object.z));
		float c = eye_object.x*eye_object.x + eye_object.y*eye_object.y + eye_object.z*eye_object.z - 1;
		float root = b*b - 4*a*c;
		if (root < 0) distance = 0;
		else if (root == 0)
		{
			distance = -b/(2*a);
		}
		else
		{
			float t1 = (float)(-b + sqrt(root))/(2*a);
			float t2 = (float)(-b - sqrt(root))/(2*a);

			if ((t1 < 0) && (t2 < 0)) distance = 0;
			else if ((t2 < 0) || ((t1 >= 0) && (t1 <= t2)))
			{
				distance =  t1;
			}
			else
			{
				distance = t2;
			}
		}
	}
	if (distance <= 0)
	{
		rgb[0] = 0x00; rgb[1] = 0x00; rgb[2] = 0x00;
		return 0;
	}
	VECTOR3D point_object = {};
	point_object.x = eye_object.x + (vector_object.x * distance);
	point_object.y = eye_object.y + (vector_object.y * distance);
	point_object.z = eye_object.z + (vector_object.z * distance);
	point_object.is_point = TRUE;

	VECTOR3D light = {};
	light.x = -1.0f;
	light.y = -1.0f;
	light.z = 2.0f;
	VECTOR3D light_object = {};
	Matrix3D_MultiplyVector(&light_object,sphere_inv,sphere_mtx,&light);
	float diffuse = 0;
	{
		float dot_product = 0;
		Vector3D_Normalize(&light_object,&light_object);
		dot_product = Vector3D_DotProduct(&light_object,&point_object);
		if (dot_product <= 0.0f) diffuse =  0.0f;
		else diffuse = dot_product;
	}
	float random = ((rand() * dithering)/RAND_MAX) - (dithering / 2.0f);
	diffuse = (diffuse*255.0f) + random;
	if (diffuse < 0.0f) diffuse = 0.0f;
	if (diffuse > 255.0f) diffuse = 255.0f;
	rgb[0] = (uint8) (diffuse);
	rgb[1] = (uint8) (diffuse);
	rgb[2] = (uint8) (diffuse);
	return distance;

}

internal bool32
GetNextFrame(MEMORY *memory, FRAME *frame)
{
	STATE *state = (STATE *) memory->permanent_storage;
	CAMERA *camera = &state->camera;
	if (!state->is_initialized)
	{
		state->frame_count = 0;
		state->is_initialized = TRUE;
		camera->half_angle_x = TAU_32/8;
		camera->half_angle_y = TAU_32/8;
		camera->height = (float) tan(camera->half_angle_y)*2;
		camera->width = (float) tan(camera->half_angle_x)*2;
		camera->yon = 500;
		camera->eye.is_point = TRUE;
		camera->coi.is_point = TRUE;
		camera->up.is_point = TRUE;
	}
	if (state->frame_count >= 1)
	{
		return FALSE;
	}
	uint8 *pixel = (uint8 *) frame->memory;
	for (uint32 y = 0; y < frame->height; y++)
	{
		for (uint32 x = 0; x < frame->width; x++)
		{
			VECTOR3D vector = {};
			vector.x = (x - frame->width/2.0f)*(camera->width / frame->width);
			vector.y = 1.0f;
			vector.z = -(y - frame->height/2.0f)*(camera->height / frame->height);
			Vector3D_Normalize(&vector,&vector);
			uint8 rgb[3] = {};
			Raytrace(rgb,&camera->eye,&vector,state,frame->dithering);
			*pixel++ = rgb[0]; // NOTE(cch): red
			*pixel++ = rgb[1]; // NOTE(cch): green
			*pixel++ = rgb[2]; // NOTE(cch): blue
			*pixel++;        // NOTE(cch): unused alpha layer
		}
	}
	state->frame_count++;
	return TRUE;
}
