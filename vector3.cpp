#include "vector3.h"
#include "global.h"
#include <math.h>
#include <stdlib.h>


//---------------------------------------------------------------------------------------
//Vector Operations ---------------------------------------------------------------------
//---------------------------------------------------------------------------------------

void normalize3(float *v, float *dst){        //Normalize 3-Vector
  float L = sqrt(dot3(v,v));
  mul3c(v, 1.0/L, dst);
}

void copy3(float *a,float *dst){   //Subtract 3-Vectors
  dst[0] = a[0];
  dst[1] = a[1];
  dst[2] = a[2];
}

void sub3(float *a, float *b, float *dst){   //Subtract 3-Vectors
  dst[0] = a[0] - b[0];
  dst[1] = a[1] - b[1];
  dst[2] = a[2] - b[2];
}

void add3(float *a, float *b, float *dst){   //Add 3-Vectors
  dst[0] = a[0] + b[0];
  dst[1] = a[1] + b[1];
  dst[2] = a[2] + b[2];
}

void mul3c(float *a, float c, float *dst){    //Multiply 3-Vector with Scalar
  dst[0] = c * a[0];
  dst[1] = c * a[1];
  dst[2] = c * a[2];
}

float dot3(float *a, float *b){     //Dot Product 3-Vectors
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void rand3(float s, float *dst){               //Random 3-Vector
	for(int i=0; i<3; i++) dst[i] = (double)rand()*2*s/RAND_MAX - s;
}

bool gatedSqDist3(float *a, float *b, float sqradius){ //Gated Squared Distance
  float c = a[0] - b[0];          //Efficient When Determining if Thousands of Points
  float d = c*c;                  //Are Within a Radius of a Point (and Most Are Not!)
  if (d > sqradius) return false; //Gate 1 - If this dimension alone is larger than
  c = a[1] - b[1];                //         the search radius, no need to continue
  d += c*c;
  if (d > sqradius) return false; //Gate 2
  c = a[2] - b[2];
  d += c*c;
  if (d > sqradius) return false; //Gate 3
  gSqDist = d;      return true ; //Store Squared Distance Itself in Global State
}

bool gatedSqDist2(float *a, float *b, float sqradius){ //Gated Squared Distance
  float c = a[0] - b[0];          //Efficient When Determining if Thousands of Points
  float d = c*c;                  //Are Within a Radius of a Point (and Most Are Not!)
  if (d > sqradius) return false; //Gate 1 - If this dimension alone is larger than
  c = a[1] - b[1];                //         the search radius, no need to continue
  d += c*c;
  if (d > sqradius) return false; //Gate 2
  gSqDist = d;      return true ; //Store Squared Distance Itself in Global State
}