//------------------------------------------------
//  Ray Tracing & Photon Mapping
//  ORIGINAL : Grant Schindler, 2007 (in Java)
//  http://www.cc.gatech.edu/~phlosoft/photon/
//
//  MODIFIED : Kenrato Doba, 2013/02/24
//------------------------------------------------

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <algorithm>

#include <pthread.h>

#include "vector3.h"
#include "global.h"
#include "main.h"

using std::vector;
using std::max;
using std::min;
using namespace WebCore;

std::vector<CObj*> objects;

template <typename T> inline T
constrain(T src, T lower, T upper) { return min(upper, max(src, lower)); }
inline bool odd(int x) { return x & 1; }

int
main(int argc, char *argv[]) {

  initObje();

  emitPhotons();
  resetRender();

  //--  initialize glut (option, pos, size)
  glutInit(&argc,argv);
  glutInitWindowPosition(WPOSX, WPOSY);
  glutInitWindowSize(WINW, WINH);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH);

  glutCreateWindow("GLUT Template");

  //--  set callback functions
  glutDisplayFunc(display);
  glutTimerFunc(10, onTimer, 0);
  glutReshapeFunc(resize);

  //--  event handler
  glutKeyboardFunc(onKeyPress);
  glutMouseFunc(onClick);
  glutMotionFunc(onDrag);

  //--  set callback at exit of program
  atexit(freeObje);

  glClear(GL_COLOR_BUFFER_BIT);
  glutMainLoop();

  return 1;
}

//----------------------------
//  Ray-Geometry Intersections
//----------------------------

void
checkDistance(float lDist, CObj *ob)
{
  //--  when current distance is valid & lowest : update
  if (lDist < gDist && lDist > 0.0){
    gObje      = ob;
    gDist      = lDist;
    //--  intersection success
    gIntersect = true;
  }
}

void
rayObject(CObj *ob, const Vector3 &r, const Vector3 &o){
  double dist = NOT_INTERSECTED;

  int tp = ob->getType();
  //--  switch intersection func with object type
  if      (tp == TYPE_SPHERE) {
    dist = ob->calcSphereIntersection(r, o);
  } else if (tp == TYPE_PLANE) {
    dist = ob->calcPlaneIntersection(r, o);
  }

  checkDistance(dist, ob);
}

//----------
//  Lighting
//----------

float
lightDiffuse(const Vector3 &N, const Vector3 &P)
{
  //  Diffuse Lighting at Point P with Surface Normal N
  Vector3 L = Light - P;
  L.normalize();
  return dot(N,L);
}

Vector3
surfaceNormal(CObj *ob, const Vector3 &P, const Vector3 &Inside){
  if (ob->getType() == TYPE_SPHERE)     {
    return ob->calcSphereNormal(P, Inside);
  } else if (ob->getType() == TYPE_PLANE) {
    return ob->calcPlaneNormal(P, Inside);
  }
  return Vector3();
}

float
lightObject(CObj *ob, const Vector3 &P, float lightAmbient){
  Vector3 N = surfaceNormal(ob, P, Light);
  float   i = lightDiffuse(N, P);
  //--  add in ambient light by constraining min value
  return min(1.0f, max(i, lightAmbient));
}

//------------
//  Raytracing
//------------

void
raytrace(const Vector3 &ray, const Vector3 &origin)
{
  //--  init intersection status
  gIntersect = false;
  gDist = NOT_INTERSECTED;

  //--  check intersection for each object
  for (int i=0; i<nrObjects; i++) {
    rayObject(objects[i], ray, origin);
  }
}

Vector3
calcPixelColor(float x, float y){
  Vector3 rgb(0.0,0.0,0.0);

  //--  generate Ray for each pixel
  //--  Convert Pixels to Image Plane Coordinates
  Vector3 ray(
      x / szImg - 0.5 ,
    -(y / szImg - 0.5),
    1.0
    //Focal Length = 1.0
  );

  raytrace(ray, gOrigin);

  if (gIntersect){
    //--  get point of intersection
    gPoint = ray * gDist;

    int ref = 0;
    //  Mirror Surface on This Specific Object
    while (gObje->getOptics() == OPT_REFLECT && ref < reflection_limit){
      ray = reflect(ray,gOrigin);        //Reflect Ray Off the Surface
      ref++;
      raytrace(ray, gPoint);             //Follow the Reflected Ray
      if (gIntersect){
        Vector3 tmp = ray * gDist;
        gPoint = gPoint + tmp;
      }
    } //3D Point of Intersection

    if (lightPhotons){
      //--  Lighting via Photon Mapping
      rgb = gatherPhotons(gPoint, gObje);
    } else {
      //--  Lighting via Standard Illumination Model (Diffuse + Ambient)
      //--  Remember Intersected Object
      CObj *tObje = gObje;

      //--  If in Shadow, Use Ambient Color of Original Object
      float i = gAmbient;

      //--  Raytrace from Light to Object
      Vector3 tmp = gPoint - Light;
      raytrace(tmp, Light);

      if (tObje == gObje) {
        //--  Ray from Light -> Object Hits Object First? : not in shadow
        i = lightObject(gObje, gPoint, gAmbient);
      }
      Vector3 energy(i, i, i);
      rgb = mulColor(energy, tObje);
    }
  }
  return rgb;
}

Vector3
reflect(const Vector3 &ray, const Vector3 &fromPoint)
{
  Vector3 N = surfaceNormal(gObje, gPoint, fromPoint);

  Vector3 ans = ray - N * (2 * dot(ray,N));
  ans.normalize();
  return ans;
}

Vector3
refract(const Vector3 &ray, const Vector3 &fromPoint)
{
  //--  @TODO : implement refraction
  return Vector3();
}

//----------------
//  Photon Mapping
//----------------

Vector3
gatherPhotons(const Vector3 &p, CObj *ob)
{
  Vector3 energy;
  int id = ob->getIndex();
  Vector3 N = surfaceNormal(ob, p, gOrigin);

  for (int i = 0; i < numPhotons[id]; i++) {
    //--  Photons Which Hit Current Object
    double cur_sqdist = distance(p, photons[id][i][0]);

    //--  Is Photon Close to Point?
    if (cur_sqdist < sqRadius) {
      gSqDist = cur_sqdist;
      float weight = max(0.0, -dot(N, photons[id][i][1]) );

      //--  Single Photon Diffuse Lighting
      //--  Weight by Photon-Point Distance
      weight *= (1.0 - sqrt(gSqDist)) / exposure;
      Vector3 tmp = photons[id][i][2] * weight;
      energy = energy + tmp;
    }
  }
  return energy;
}

Vector3
randDir(double s)
{
  //--  generate vector with random derection
  double tmp[3];
  for(int i=0; i<3; i++) {
    tmp[i] = (double)rand() * 2 * s / RAND_MAX - s;
  }
  Vector3 ans(tmp);
  ans.normalize();
  return ans;
}

void emitPhotons(){

  //--  "randomized" photons are generated with the same properties indeed
  srand(0);

  //--  init photon num
  for (int t = 0; t < nrObjects; t++) { numPhotons[t] = 0; }

  Vector3 rgb, ray, col, prevPoint;
  Vector3 white(1.0, 1.0, 1.0);

  //--  control photon num with rendering option
  const int num_photon = view3D ? nrPhotons * 3.0 : nrPhotons;
  for (int i = 0; i < num_photon; i++){
    int bounces = 1;

    //--  initialize photon properties (color, direction, location)
    rgb = white;
    ray = randDir(1.0);
    prevPoint = Light;

    //--  randomize photon locations
    while (prevPoint.y() >= Light.y()) {
      //--  +Y dir
      prevPoint = randDir(1.0) * 0.75 + Light;
    }

    //--  photons outside of the room : invalid
    if (fabs(prevPoint[0]) > 1.5 || fabs(prevPoint[1]) > 1.2 ) { bounces = nrBounces + 1; }

    //--  photons inside any objects : invalid
    for(int dx=0; dx<nrObjects; dx++) {
      CObj *ob = objects[dx];
      if(ob->getType() != TYPE_SPHERE) continue;
      Vector3 center(ob->coords[0], ob->coords[1], ob->coords[2]);
      if(distance(prevPoint, center) < ob->coords[3]) { bounces = nrBounces+1; }
    }

    //--  calc intersection (1st time)
    raytrace(ray, prevPoint);

    //--  calc bounced photon's intercection (2nd, 3rd, ...)
    while (gIntersect && bounces <= nrBounces){
      gPoint = ray * gDist + prevPoint;

      col = mulColor(rgb, gObje);
      rgb = col * (1.0/sqrt((double)bounces));

      storePhoton(gObje, gPoint, ray, rgb);

      drawPhoton(rgb, gPoint);
      shadowPhoton(ray);

      ray = reflect(ray, prevPoint);

      raytrace(ray, gPoint);

      prevPoint = gPoint;
      bounces++;
    }
  }
}

void
storePhoton(CObj *ob, const Vector3 &location, const Vector3 &direction, const Vector3 &energy){
  int id = ob->getIndex();
  //  0 : location
  //  1 : direction
  //  2 : energy
  photons[id][numPhotons[id]][0] = location;
  photons[id][numPhotons[id]][1] = direction;
  photons[id][numPhotons[id]][2] = energy;
  numPhotons[id]++;
}

void
shadowPhoton(const Vector3 &ray){
  Vector3 shadow (-0.25,-0.25,-0.25);

  //  save current state
  Vector3 tPoint = gPoint;
  CObj *tObje = gObje;

  Vector3 bumpedPoint, shadowPoint;

  //Start Just Beyond Last Intersection
  bumpedPoint = gPoint + ray * 1.0e-5;

  //Trace to Next Intersection (In Shadow)
  raytrace(ray, bumpedPoint);

  //3D Point
  shadowPoint = bumpedPoint + ray * gDist;

  storePhoton(gObje, shadowPoint, ray, shadow);

  //Restore State
  gPoint = tPoint;
  gObje  = tObje;
}

Vector3
mulColor(const Vector3 &rgbIn, CObj *ob)
{
  //--  Specifies Material Color of Each Object
  return Vector3(
      ob->color[0] * rgbIn[0],
      ob->color[1] * rgbIn[1],
      ob->color[2] * rgbIn[2] );
}

//------------------------------
//  User Interaction and Display
//------------------------------

void
resize(int w, int h) {
  //--  set orthogonal view
  glViewport(0, 0, WINW, WINH);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0,(double)WINW,0.0,(double)WINH,-10.0,10.0);
}

void display(){
  if (view3D){
    if (empty){
      //--  Emit & Draw Photons
      glClear(GL_COLOR_BUFFER_BIT);
      emitPhotons();
      empty = false;
      sleep(1); 
    }
  }else{
    if (empty) render();
    else sleep(1);  //Only Draw if Image Not Fully Rendered
  }
  glFlush();
}

void
render(){ //Render Several Lines of Pixels at Once Before Drawing
  int x,y,iterations = 0;
  Vector3 rgb;

  while (iterations < (mouseDragging ? 1024 : max(pMax, 256) )){

    //Render Pixels Out of Order With Increasing Resolution: 2x2, 4x4, 16x16... 512x512
    if (pCol >= pMax) {
      pRow++; pCol = 0;
      if (pRow >= pMax) {
        pIteration++;
        pRow = 0;
        pMax = int(pow(2.0,(double)pIteration));
      }
    }
    bool pNeedsDrawing = (pIteration == 1 || odd(pRow) || (!odd(pRow) && odd(pCol)));
    x = pCol * (szImg/pMax); y = pRow * (szImg/pMax);
    pCol++;

    if (pNeedsDrawing){
      iterations++;
      rgb = calcPixelColor(x,y);
      //mul3c( tmp, 1.0, rgb);               //All the Magic Happens in Here!
      //mul3c( tmp, 255.0, rgb);

      //ƒsƒNƒZƒ‹‚²‚Æ‚É•`‰æ‚µ‚Ä‚¢‚­
      glColor3d(rgb[0],rgb[1],rgb[2]);
      glPointSize((float)szImg/pMax);
      glBegin(GL_POINTS);
      glVertex2d((double)x,(double)WINH-y);
      glEnd();

    }
  }
  if (pRow == szImg-1) {empty = false;}
}

void resetRender(){ //Reset Rendering Variables
  pRow=0; pCol=0; pIteration=1; pMax=2;
  empty=true;
  if (lightPhotons && !view3D) emitPhotons();
}

void drawPhoton(const Vector3 &rgb, const Vector3 &p){           //Photon Visualization
  if (view3D && p[2] > 0.0){                       //Only Draw if In Front of Camera
    int x = (szImg/2) + (int)(szImg *  p[0]/p[2]); //Project 3D Points into Scene
    int y = (szImg/2) + (int)(szImg * -p[1]/p[2]); //Don't Draw Outside Image
    if (y <= szImg) {
      glColor3d(rgb[0],rgb[1],rgb[2]);
      glPointSize(1.0);
      glBegin(GL_POINTS);
      glVertex2d((double)x,(double)WINH-y);
      glEnd();
    }
  }
}

//--------------------------------
//  Mouse and Keyboard Interaction
//--------------------------------
int prevMouseX = -9999, prevMouseY = -9999, sphereIndex = -1;
float s = 130.0;

void
onKeyPress(unsigned char key,int, int) {
  switch(key) {
    case 49 /*1*/ : view3D = false; lightPhotons = false; break;
    case 50 /*2*/ : view3D = false; lightPhotons = true; break;
    case 51 /*3*/ : view3D = true; break;
    default     : return;
  }
  resetRender();
  printf("No. %d key pressed\n",key);
}

void
onClick(int button,int action, int x, int y) {
  //--  if not Left-Button : do nothing
  if(button != 0) return;

  //--  when pushing
  if(action == 0) {
    mouseDragging = true;
    //-- set invalid sphere index (NOT_SELECTED) at first
    sphereIndex = nrObjects;

    mouseX = x;
    mouseY = y;
    //--  mouse coords to screen coords
    float mousecoord[] = {
       (mouseX - szImg/2)/s,
      -(mouseY - szImg/2)/s
    };

    for(int i=0; i<nrObjects; i++) {
      CObj *ob = objects[i];
      if (ob->getType() != TYPE_SPHERE) { continue; }
      Vector3 mouse2screen(
          mousecoord[0],
          mousecoord[1],
          ob->coords[2]
          );
      Vector3 center(ob->coords[0], ob->coords[1], ob->coords[2]);
      if (distance(mouse2screen, center) < ob->coords[3]) { sphereIndex = i; }
    }
    printf("sphere %d\n",sphereIndex);
  }
  //--  when releasing
  else {
    prevMouseX = -9999;
    prevMouseY = -9999;
    mouseDragging = false;
  }
}

void
onDrag(int x,int y) {
  //--  current mouse coords
  mouseX = x;
  mouseY = y;

  if(mouseDragging) {
    if (prevMouseX > -9999 && sphereIndex > -1){
      if (sphereIndex < nrObjects){ //Drag Sphere
        objects[sphereIndex]->coords[0] += (mouseX - prevMouseX)/s;
        objects[sphereIndex]->coords[1] -= (mouseY - prevMouseY)/s;
      }else{ //Drag Light
        Light = Vector3(
            constrain(Light[0] + (mouseX - prevMouseX)/s, -1.4, 1.4),
            constrain(Light[1] - (mouseY - prevMouseY)/s, -0.4, 1.2),
            Light[2] );
      }
      resetRender();
    }
    prevMouseX = mouseX;
    prevMouseY = mouseY;
  }
}

void
onTimer(int val) {
  //--  call display callback func
  glutPostRedisplay();
  //--  set next timer
  glutTimerFunc(10, onTimer, val);
}


void initObje() {
  //--  color literal
  static const float white[3] = {1.0,1.0,1.0};
  static const float red[3]   = {1.0,0.0,0.0};
  static const float green[3] = {0.0,1.0,0.0};
  static const float blue[3]  = {0.0,0.0,1.0};

  float v_sphere[][4] = {
    //-- {center(x,y,z), radius}
    { 1.0,  0.0, 4.0, 0.5},
    {-0.6, -1.0, 4.5, 0.5},
    { 0.0,  0.0, 3.0, 0.2},
  };

  float v_plane[][2]  = {
    //--  {(axis_id), (distance_from_origin)}
    //--  axis_id = 0:X, 1:Y, 2:Z
    {0,  1.5},
    {1, -1.5},
    {0, -1.5},
    {1,  1.5},
    {2,  5.0}
  };

  //--  cleate objects and register them
  objects.resize(0);

  CObj *ob;

  //--  cleate spheres
  for(int i=0; i<3; i++) {
    ob = new CObj(TYPE_SPHERE,nrObjects++,v_sphere[i]);
    objects.push_back(ob);
  }

  //--  cleate planes
  for(int i=0; i<5; i++) {
    ob = new CObj(TYPE_PLANE,nrObjects++,v_plane[i]);
    objects.push_back(ob);
  }

  //--  set optical properties
  objects[2]->setOptics(OPT_REFLECT);
  objects[4]->setColor(green);
  objects[6]->setColor(red);

}

void
freeObje() {
  for(int i=0; i<nrObjects; i++) { delete objects[i]; }
  objects.clear();
}


