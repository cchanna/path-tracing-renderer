#if !defined (VECTOR_3D_H)

struct VECTOR3D
{
	float x,y,z;
	bool32 is_point;
};

void Vector3D_Copy(VECTOR3D *a, VECTOR3D *b);
float Vector3D_Length(float a[3]);
void Vector3D_GetVectorFromPoints(VECTOR3D *result, VECTOR3D *origin, VECTOR3D *direction);

#define VECTOR_3D_H
#endif
