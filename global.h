// ----- Raytracing Globals -----
extern bool gIntersect;       //For Latest Raytracing Call... Was Anything Intersected by the Ray?
extern int gType;                        //... Type of the Intersected Object (Sphere or Plane)
extern int gIndex;                       //... Index of the Intersected Object (Which Sphere/Plane Was It?)
extern float gSqDist, gDist;      //... Distance from Ray Origin to Intersection
extern float gPoint[3]; //... Point At Which the Ray Intersected the Object

