//main.h
#include <GL/glut.h>
#include "object.h"

//using namespace std;
#define WINW 512
#define WINH 512
#define WPOSX 100
#define WPOSY 50


// ----- Scene Description -----
int szImg = 512;            //--  rendering screen size
int nrTypes = 2;            //--  object tpye = 0:SPHERE, 1:PLANE
int nrObjects = 0;          //--  num of object
float gAmbient = 0.1;       //--  ambient lighting energy
const Vector3 gOrigin(0.0,0.0,0.0);  //World Origin for Convenient Re-Use Below (Constant)
Vector3 Light(0.0,1.2,3.75);   //Point Light-Source Position

// ----- Photon Mapping -----
int nrPhotons = 1000;     //--  Number of Photons Emitted
int nrBounces = 3;        //--  Number of Times Each Photon Bounces
bool lightPhotons = true; //--  Enable Photon Lighting?
float sqRadius = 0.7;     //--  Photon Integration Area (Squared for Efficiency)
float exposure = 50.0;    //--  Number of Photons Integrated at Brightest Pixel
int   numPhotons[64];     //--  Photon Count for Each Scene Object

//  Allocated Memory for Per-Object Photon Info
//  0 : location
//  1 : direction
//  2 : energy
Vector3 photons[64][5000][3];

// ----- Raytracing Globals (also declared in "global.h") -----
bool    gIntersect = false;  //--  Was Anything Intersected by the Ray?
int     gType;               //--  Type of the Intersected Object (Sphere or Plane)
int     gIndex;              //--  Index of the Intersected Object (Which Sphere/Plane Was It?)
float   gSqDist;
float   gDist;               //--  Distance from Ray Origin to Intersection (raw, sruared)
Vector3 gPoint;              //--  intersection point
CObj    *gObje;              //--  intersected object
float   gRefractive;

/**functions**/
void    rayObject(CObj *ob, float *r, float *o);
void    checkDistance(float lDist, CObj *ob);
float   lightDiffuse(float *N, float *P);
float   lightObject(CObj *ob, float *P, float lightAmbient);
void    raytrace(float *ray, float *origin);

Vector3 surfaceNormal(CObj *ob, float *P, float *Inside);
Vector3 calcPixelColor(float x, float y);

Vector3 reflect(const Vector3 &ray, const Vector3 &fromPoint);
Vector3 refract(const Vector3 &ray, const Vector3 &fromPoint);

Vector3 gatherPhotons(const Vector3 &p, CObj *ob);
void    emitPhotons();
void    storePhoton(CObj *ob,
    const Vector3 &location,
    const Vector3 &direction,
    const Vector3 &energy );
void    shadowPhoton(const Vector3 &ray);
void    drawPhoton(const Vector3 &rgb, const Vector3 &p);

Vector3 mulColor(const Vector3 &rgbIn, CObj *ob);

void render();
void resetRender();

void initObje();
void freeObje();

void display();
void resize (int w, int h);
void onKeyPress(unsigned char key, int x, int y);
void onClick(int button, int state, int x, int y);
void onDrag(int x, int y);
void onTimer(int val);

//--  screen status variables
//--  to stop drawing
bool empty = true;
//--  to switch Views
bool view3D = false;

bool mouseDragging = false;
int  mouseX, mouseY;

const int  reflection_limit = 2;
//--  rendering pixel info
int  pRow, pCol, pIteration, pMax;
