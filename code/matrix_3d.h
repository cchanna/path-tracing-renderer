// NOTE(cch): new matrix math library

#include <math.h>
#include "vector_3d.h"
#include "vector_3d.cpp"

void Matrix3D_Copy(float a[4][4], float b[4][4]);
// NOTE(cch): a = b

void Matrix3D_Multiply(float res[4][4], float a[4][4], float b[4][4]);
// NOTE(cch): res = a * b

void Matrix3D_GetIdentity(float a[4][4]);
// NOTE(cch): a = I

void Matrix3D_Translate(float mtx[4][4], float inv[4][4], float dx, float dy, float dz);

void Matrix3D_Scale(float mtx[4][4], float inv[4][4], float sx, float sy, float sz);

void Matrix3D_RotateX(float mtx[4][4], float inv[4][4], float cs, float sn);
void Matrix3D_RotateX(float mtx[4][4], float inv[4][4], float radians);
// NOTE(cch): rotate around the X axis. give the radians to rotate, or if you
// already happen to have the cosine and sine of the radians you use those

void Matrix3D_RotateY(float mtx[4][4], float inv[4][4], float cs, float sn);
void Matrix3D_RotateY(float mtx[4][4], float inv[4][4], float radians);

void Matrix3D_RotateZ(float mtx[4][4], float inv[4][4], float cs, float sn);
void Matrix3D_RotateZ(float mtx[4][4], float inv[4][4], float radians);

void Matrix3D_ReflectX(float mtx[4][4], float inv[4][4]);
void Matrix3D_ReflectY(float mtx[4][4], float inv[4][4]);
void Matrix3D_ReflectZ(float mtx[4][4], float inv[4][4]);

void Matrix3D_MultiplyPoint(float out[3], float mtx[4][4], float in[3]);
void Matrix3D_MultiplyPoints(float *out[3], float mtx[4][4], float *in[3], int count);
// NOTE(cch): out = mtx * in, where out and in are vertically-aligned vectors

int Matrix3D_View(float view[4][4], float view_inverse[4][4], float eye[3], float coi[3], float up[3]);
int Matrix3D_View(float view[4][4], float view_inverse[4][4], float eye[3], float coi[3]);
// NOTE(cch): construct the view matrix and its inverse given the location of
// the eye, the center of interest, and an up point.
// NOTE(cch): if no up point is given it'll assume 'up' is along positive z
// NOTE(cch): returns 1 on success, 0 on error
