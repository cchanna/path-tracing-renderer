// NOTE(cch): not sure if I actually need a library for this, but it might prove
// useful

void Vector3D_Copy(float a[3], float b[3])
{
	a[0] = b[0];
	a[1] = b[1];
	a[2] = b[2];
}

float Vector3D_Length(float a[3])
{
	return (float) sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
}
