//main.h
#include <GL/glut.h>
#include "cObje.h"

//using namespace std;
#define WINW 512
#define WINH 512
#define WPOSX 100
#define WPOSY 50


// ----- Scene Description -----
int szImg = 512;                  //レンダリングサイズ
int nrTypes = 2;                  //2 Object Types (Sphere = 0, Plane = 1)
int nrObjects = 0;          //2 Spheres, 5 Planes
float gAmbient = 0.1;             //Ambient Lighting
float gOrigin[] = {0.0,0.0,0.0};  //World Origin for Convenient Re-Use Below (Constant)
float Light[] = {0.0,1.2,3.75};   //Point Light-Source Position

// ----- Photon Mapping -----
int nrPhotons = 1000;             //Number of Photons Emitted
int nrBounces = 3;                //Number of Times Each Photon Bounces
bool lightPhotons = true;      //Enable Photon Lighting?
float sqRadius = 0.7;             //Photon Integration Area (Squared for Efficiency)
float exposure = 50.0;            //Number of Photons Integrated at Brightest Pixel
int numPhotons[64];              //Photon Count for Each Scene Object
float photons[64][5000][3][3]; //Allocated Memory for Per-Object Photon Info

// ----- Raytracing Globals -----
bool gIntersect = false;       //For Latest Raytracing Call... Was Anything Intersected by the Ray?
int gType;                        //... Type of the Intersected Object (Sphere or Plane)
int gIndex;                       //... Index of the Intersected Object (Which Sphere/Plane Was It?)
float gSqDist, gDist = -1.0;      //... Distance from Ray Origin to Intersection
float gPoint[3]; //... Point At Which the Ray Intersected the Object
cObje *gObje;
float gRefractive;

/**functions**/
void raySphere(cObje *ob, float *r, float *o);
void rayPlane(cObje *ob, float *r, float *o);
void rayObject(cObje *ob, float *r, float *o);
void checkDistance(float lDist, cObje *ob);
float lightDiffuse(float *N, float *P);
void sphereNormal(cObje *ob, float *P, float *dst);
void planeNormal(cObje *ob, float *P, float *O, float *dst);
void surfaceNormal(cObje *ob, float *P, float *Inside, float *dst);
float lightObject(cObje *ob, float *P, float lightAmbient);
void raytrace(float *ray, float *origin);
void computePixelColor(float x, float y, float *dst);
void reflect(float *ray, float *fromPoint, float *dst);
void refract(float *ray, float *fromPoint, float *dst);
void gatherPhotons(float *p, cObje *ob, float *dst);
void emitPhotons();
void storePhoton(cObje *ob, float *location, float *direction, float *energy);
void shadowPhoton(float *ray);
void filterColor(float *rgbIn, float r, float g, float b, float *dst);
void getColor(float *rgbIn, cObje *ob, float *dst);

void drawPhoton(float *rgb, float *p);
void render();
void resetRender();

void initObje();
void freeObje();

//GLFWのコールバック関数
//void GLFWCALL KeyFunc(int,int);
//void GLFWCALL MouseButtonFunc(int,int);
//void GLFWCALL MousePosFunc(int,int);
void display();
void resize (int w, int h);
void keyboard(unsigned char key, int x, int y);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);

//変数たち（GUI関連）
bool empty = true, view3D = false; //Stop Drawing, Switch Views
bool mouseDragging = false;
int pRow, pCol, pIteration, pMax;     //Pixel Rendering Order
int mouseX, mouseY;
int rayReflects = 2;

//マクロ的関数
bool odd(int x) {return x % 2 != 0;}	//奇数かどうかを判別(描画判定に使用)