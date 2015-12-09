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
	frame->delay = 2;
	frame->dithering = 10.0f;
}

internal float
Raytrace(float rgb[3], VECTOR3D *eye, VECTOR3D *vector, STATE *state, float dithering)
{
	VECTOR3D eye_object = {};
	VECTOR3D vector_object = {};
	float distance = state->camera.yon;
	float distance_object = 0;
	uint32 sphere_num = 0;
	for (uint32 i = 0; i < state->num_spheres; i++)
	{
		SPHERE *sphere = &(state->spheres[i]);
		VECTOR3D eye_sphere = {};
		VECTOR3D vector_sphere = {};
		Matrix3D_MultiplyVector(&eye_sphere,sphere->inv,sphere->mtx,eye);
		Matrix3D_MultiplyVector(&vector_sphere,sphere->inv,sphere->mtx,vector);
		float length = Vector3D_Length(&vector_sphere);
		Vector3D_Normalize(&vector_sphere,&vector_sphere);
		// NOTE(cch): intersection with a sphere
		// NOTE(cch): quadratic formula
		float a = vector_sphere.x*vector_sphere.x + vector_sphere.y*vector_sphere.y + vector_sphere.z*vector_sphere.z;
		float b = 2*((eye_sphere.x*vector_sphere.x) + (eye_sphere.y*vector_sphere.y) + (eye_sphere.z*vector_sphere.z));
		float c = eye_sphere.x*eye_sphere.x + eye_sphere.y*eye_sphere.y + eye_sphere.z*eye_sphere.z - 1;
		float root = b*b - 4*a*c;
		float distance_sphere = 0;
		if (root < 0) distance_sphere = 0;
		else if (root == 0)
		{
			distance_sphere = -b/(2*a);
		}
		else
		{
			float t1 = (float)(-b + sqrt(root))/(2*a);
			float t2 = (float)(-b - sqrt(root))/(2*a);

			if ((t1 < 0) && (t2 < 0)) distance_sphere = 0;
			else if ((t2 < 0) || ((t1 >= 0) && (t1 <= t2)))
			{
				distance_sphere =  t1;
			}
			else
			{
				distance_sphere = t2;
			}
		}
		distance_sphere *= length;
		if (distance_sphere <= distance && distance_sphere >= state->camera.fore)
		{
			distance = distance_sphere;
			distance_object = distance_sphere/length;
			Vector3D_Copy(&eye_object,&eye_sphere);
			Vector3D_Copy(&vector_object,&vector_sphere);
			sphere_num = i;
		}
	}
	VECTOR3D light = {};
	light.x = -80.0f;
	light.y = -100.0f;
	light.z = 50.0f;
	light.is_point = FALSE;
	Vector3D_Normalize(&light, &light);
	if (distance >= state->camera.yon)
	{

		float sunlight = Vector3D_DotProduct(vector, &light);
		if (sunlight > 0.0f)
		{
			sunlight = powf(sunlight, 2.0);
			rgb[0] = sunlight*0.8f + 1.0f*0.2f;
			rgb[1] = sunlight*0.8f + 1.0f*0.2f;
			rgb[2] = sunlight*0.8f + 1.0f*0.2f;
		}
		else
		{
			rgb[0] = 1.0f*0.2f;
			rgb[1] = 1.0f*0.2f;
			rgb[2] = 1.0f*0.2f;
		}
		return 0;
	}
	float diffuse = 0;
	{
		VECTOR3D point_object = {};
		point_object.x = eye_object.x + (vector_object.x * distance_object);
		point_object.y = eye_object.y + (vector_object.y * distance_object);
		point_object.z = eye_object.z + (vector_object.z * distance_object);
		point_object.is_point = TRUE;
		VECTOR3D light_object = {};
		SPHERE *sphere = &(state->spheres[sphere_num]);
		Matrix3D_MultiplyVector(&light_object,sphere->inv,sphere->mtx,&light);
		float dot_product = 0;
		Vector3D_Normalize(&light_object,&light_object);
		dot_product = Vector3D_DotProduct(&light_object,&point_object);
		if (dot_product <= 0.0f) diffuse =  0.0f;
		else diffuse = dot_product;
		float random = ((rand() * dithering)/RAND_MAX) - (dithering / 2.0f);
		diffuse = (diffuse) + (random / 255.f);
		if (diffuse < 0.0f) diffuse = 0.0f;
		if (diffuse > 1.0f) diffuse = 1.0f;
	}
	rgb[0] = (diffuse);
	rgb[1] = (diffuse);
	rgb[2] = (diffuse);
	return distance;
}

internal void
ResetSphere(SPHERE *sphere)
{
	Matrix3D_GetIdentity(sphere->mtx);
	Matrix3D_GetIdentity(sphere->inv);
}

internal SPHERE *
GetSphere(STATE *state)
{
	SPHERE *sphere = &(state->spheres[state->num_spheres]);
	Matrix3D_GetIdentity(sphere->mtx);
	Matrix3D_GetIdentity(sphere->inv);
	state->num_spheres++;
	return sphere;
}

internal void
EnterOrbit(SPHERE *moon, SPHERE *sphere)
{
	Matrix3D_Multiply(moon->mtx,sphere->mtx,moon->mtx);
	Matrix3D_Multiply(moon->inv,moon->inv,sphere->inv);
}

internal void
LeaveOrbit(SPHERE *moon, SPHERE *sphere)
{
	Matrix3D_Multiply(moon->mtx,sphere->inv,moon->mtx);
	Matrix3D_Multiply(moon->inv,moon->inv,sphere->mtx);
}

internal bool32
GetNextFrame(MEMORY *memory, FRAME *frame)
{
	STATE *state = (STATE *) memory->permanent_storage;
	CAMERA *camera = &state->camera;
	{
		uint32 number_of_frames = 300;
		uint32 frames_to_rotate = 300;
		SPHERE *sphere;
		SPHERE *moon;
		if (!state->is_initialized)
		{
			state->frame_count = 0;
			state->is_initialized = TRUE;
			state->num_spheres = 0;
			camera->half_angle_x = TAU_32/12;
			camera->half_angle_y = TAU_32/12;
			camera->height = (float) tan(camera->half_angle_y)*2;
			camera->width = (float) tan(camera->half_angle_x)*2;
			camera->fore = 0.1f;
			camera->yon = 500.0f;
			camera->eye.is_point = TRUE;
			camera->coi.is_point = TRUE;
			camera->up.is_point = TRUE;
			sphere = GetSphere(state);
			moon = GetSphere(state);
		}
		else
		{
			sphere = &(state->spheres[0]);
			moon = &(state->spheres[1]);
		}
		if (state->frame_count >= number_of_frames)
		{
			return FALSE;
		}
		else
		{
			LeaveOrbit(moon,sphere);
			ResetSphere(sphere);
			Matrix3D_Translate(sphere->mtx,sphere->inv,
				5.0f*cosf(TAU_32*state->frame_count/frames_to_rotate),
				5.0f*sinf(TAU_32*state->frame_count/frames_to_rotate) + 15.0f,
				0.0f
			);
			ResetSphere(moon);
			Matrix3D_Scale(moon->mtx,moon->inv,0.2f,0.2f,0.2f);
			Matrix3D_Translate(moon->mtx,moon->inv,
				1.5f*cosf(TAU_32*4.0f*state->frame_count/frames_to_rotate),
				0.0f,
				1.5f*sinf(TAU_32*4.0f*state->frame_count/frames_to_rotate)
			);
			EnterOrbit(moon,sphere);
		}
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
			float rgb_f[3] = {};
			Raytrace(rgb_f,&camera->eye,&vector,state,frame->dithering);
			if (rgb_f[0] > 1.0f) rgb_f[0] = 1.0f;
			if (rgb_f[1] > 1.0f) rgb_f[1] = 1.0f;
			if (rgb_f[2] > 1.0f) rgb_f[2] = 1.0f;
			uint8 rgb[3];
			rgb[0] = (uint8)(rgb_f[0] * 255.0f);
			rgb[1] = (uint8)(rgb_f[1] * 255.0f);
			rgb[2] = (uint8)(rgb_f[2] * 255.0f);
			*pixel++ = rgb[0]; // NOTE(cch): red
			*pixel++ = rgb[1]; // NOTE(cch): green
			*pixel++ = rgb[2]; // NOTE(cch): blue
			*pixel++;        // NOTE(cch): unused alpha layer
		}
	}
	state->frame_count++;
	return TRUE;
}
