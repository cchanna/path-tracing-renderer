#if !defined (VECTOR_3D_CPP)

struct VECTOR3D
{
	float x,y,z;
	bool32 is_point;
};

void Vector3D_Copy(VECTOR3D *a, VECTOR3D *b)
{
	a->x = b->x;
	a->y = b->y;
	a->z = b->z;
	a->is_point = b->is_point;
}

//TODO(cch): update to use new VECTOR3D struct
float Vector3D_Length(float a[3])
{
	return (float) sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
}

void Vector3D_GetVectorFromPoints(VECTOR3D *result, VECTOR3D *origin, VECTOR3D *direction)
{
	result->x = direction->x - origin->x;
	result->y = direction->y - origin->y;
	result->z = direction->z - origin->z;
	result->is_point = FALSE;
}

#define VECTOR_3D_CPP
#endif
