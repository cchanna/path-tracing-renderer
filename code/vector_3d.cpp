#if !defined (VECTOR_3D_CPP)

void Vector3D_Copy(VECTOR3D *a, VECTOR3D *b)
{
	a->x = b->x;
	a->y = b->y;
	a->z = b->z;
	a->is_point = b->is_point;
}

float Vector3D_Length(VECTOR3D *vector)
{
	return (float) sqrt(vector->x*vector->x + vector->y*vector->y + vector->z*vector->z);
}

void Vector3D_GetVectorFromPoints(VECTOR3D *result, VECTOR3D *origin, VECTOR3D *direction)
{
	result->x = direction->x - origin->x;
	result->y = direction->y - origin->y;
	result->z = direction->z - origin->z;
	result->is_point = FALSE;
}

void Vector3D_Normalize(VECTOR3D *result, VECTOR3D *vector)
{
	float length = Vector3D_Length(vector);
	result->x = vector->x/length;
	result->y = vector->y/length;
	result->z = vector->z/length;
	result->is_point = vector->is_point;
}

#define VECTOR_3D_CPP
#endif
