//vector3.h


void normalize3(float *v, float *dst);
void copy3(float *a, float *dst);
void sub3(float *a, float *b, float *dst);
void add3(float *a, float *b, float *dst);
void mul3c(float *a, float c, float *dst);
float dot3(float *a, float *b);
void rand3(float s, float *dst);
bool gatedSqDist3(float *a, float *b, float sqradius);
bool gatedSqDist2(float *a, float *b, float sqradius);