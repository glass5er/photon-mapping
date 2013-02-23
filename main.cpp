//Ray Tracing & Photon Mapping
//Grant Schindler, 2007

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <algorithm>

#include <pthread.h>

#include "vector3.h"
#include "global.h"
#include "main.h"

using namespace std;

std::vector<cObje*> objects;

//---------------------------------------------------------------------------------------
//Ray-Geometry Intersections  -----------------------------------------------------------
//---------------------------------------------------------------------------------------

void raySphere(cObje *ob, float *r, float *o) //Ray-Sphere Intersection: r=Ray Direction, o=Ray Origin
{
  float s[3];
  sub3(ob->coords,o,s);  //s=Sphere Center Translated into Coordinate Frame of Ray Origin
  float radius = ob->coords[3];    //radius=Sphere Radius

  //Intersection of Sphere and Line     =       Quadratic Function of Distance
  float A = dot3(r,r);                       // Remember This From High School? :
  float B = -2.0 * dot3(s,r);                //    A x^2 +     B x +               C  = 0
  float C = dot3(s,s) - radius*radius;          // (r'r)x^2 - (2s'r)x + (s's - radius^2) = 0
  float D = B*B - 4*A*C;                     // Precompute Discriminant

  if (D > 0.0){                              //Solution Exists only if sqrt(D) is Real (not Imaginary)
    float sign = (C < -0.00001) ? 1 : -1;    //Ray Originates Inside Sphere If C < 0
    float lDist = (-B + sign*sqrt(D))/(2*A); //Solve Quadratic Equation for Distance to Intersection
    checkDistance(lDist,ob);             //Is This Closest Intersection So Far?
  }
}

void rayPlane(cObje *ob, float *r, float *o){ //Ray-Plane Intersection
  int axis = (int) ob->coords[0];            //Determine Orientation of Axis-Aligned Plane
  if (r[axis] != 0.0){                        //Parallel Ray -> No Intersection
    float lDist = (ob->coords[1] - o[axis]) / r[axis]; //Solve Linear Equation (rx = p-o)
    checkDistance(lDist,ob);
  }
}

void rayObject(cObje *ob, float *r, float *o){
  int tp = ob->getType();
  if (tp == TYPE_SPHERE) raySphere(ob,r,o);
  else if (tp == TYPE_PLANE) rayPlane(ob,r,o);    //光線の交差判定(0:球，1:平面)
}

void checkDistance(float lDist, cObje *ob){
  if (lDist < gDist && lDist > 0.0){ //距離が最小値かつ0以上(reyの進行方向)ならば更新
    gObje = ob; gDist = lDist; gIntersect = true; //Save Intersection in Global State
  }
}
//---------------------------------------------------------------------------------------
// Lighting -----------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

float lightDiffuse(float *N, float *P){   //Diffuse Lighting at Point P with Surface Normal N
  float L[3], tmp[3];
  sub3(Light,P,tmp);
  normalize3( tmp,L );          //Light Vector (Point to Light)
  return dot3(N,L);                       //Dot Product = cos (Light-to-Surface-Normal Angle)
}

void sphereNormal(cObje *ob, float *P, float *dst){
  float tmp[3];
  sub3(P,ob->coords,tmp);       //球の中心を引き算-->法線ベクトル
  normalize3(tmp,dst);          //Surface Normal (Center to Point)
}

void planeNormal(cObje *ob, float *P, float *O, float *dst){
  int axis = (int) ob->coords[0];
  float N[3] = {0.0,0.0,0.0};
  N[axis] = O[axis] - ob->coords[1];      //Vector From Surface to Light
  normalize3(N,dst);
}

void surfaceNormal(cObje *ob, float *P, float *Inside, float *dst){
  if (ob->getType() == TYPE_SPHERE)     {sphereNormal(ob,P,dst);}
  else if (ob->getType() == TYPE_PLANE) {planeNormal(ob,P,Inside,dst);}
}

float lightObject(cObje *ob, float *P, float lightAmbient){
  float tmp[3];
  surfaceNormal(ob, P, Light, tmp);
  float i = lightDiffuse( tmp , P );
  return min(1.0f, max(i, lightAmbient));   //Add in Ambient Light by Constraining Min Value
}

//---------------------------------------------------------------------------------------
// Raytracing ---------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

void raytrace(float *ray, float *origin)
{
  gIntersect = false; //No Intersections Along This Ray Yet
  gDist = 999999.9;   //Maximum Distance to Any Object

  for (int i=0; i<nrObjects; i++)
    rayObject(objects[i],ray,origin);
}

void computePixelColor(float x, float y, float *dst){
  float rgb[3] = {0.0,0.0,0.0};
  float ray[3] = {  x/szImg - 0.5 ,       //Convert Pixels to Image Plane Coordinates
    -(y/szImg - 0.5), 1.0}; //Focal Length = 1.0
  raytrace(ray, gOrigin);                //Raytrace!!! - Intersected Objects are Stored in Global State

  if (gIntersect){                       //Intersection                    
    mul3c(ray,gDist,gPoint);           //3D Point of Intersection

    int ref = 0;
    while (gObje->getOptics()==OPT_REFLECT && ref<rayReflects){      //Mirror Surface on This Specific Object
      reflect(ray,gOrigin,ray);        //Reflect Ray Off the Surface
      ref++;
      raytrace(ray, gPoint);             //Follow the Reflected Ray
      if (gIntersect){
        float tmp[3];
        mul3c(ray,gDist,tmp);
        add3( tmp, gPoint, gPoint);
      }
    } //3D Point of Intersection

    if (lightPhotons){                   //Lighting via Photon Mapping
      gatherPhotons(gPoint,gObje,rgb);
    }else{                                //Lighting via Standard Illumination Model (Diffuse + Ambient)
      cObje *tObje = gObje;       //Remember Intersected Object
      float i = gAmbient;                //If in Shadow, Use Ambient Color of Original Object
      float tmp[3];
      sub3(gPoint,Light,tmp);
      raytrace( tmp, Light);  //Raytrace from Light to Object
      if (tObje == gObje) //Ray from Light->Object Hits Object First?
        i = lightObject(gObje, gPoint, gAmbient); //Not In Shadow - Compute Lighting
      rgb[0]=i; rgb[1]=i; rgb[2]=i;
      getColor(rgb,tObje,rgb);
    }
  }
  copy3(rgb,dst);
}

void reflect(float *ray, float *fromPoint, float *dst){                //Reflect Ray
  float N[3],tmp[3];
  surfaceNormal(gObje, gPoint, fromPoint, N);  //Surface Normal
  mul3c(N,(2 * dot3(ray,N)),tmp);
  sub3(ray,tmp,tmp);
  normalize3(tmp,dst);     //Approximation to Reflection
}

void refract(float *ray, float *fromPoint, float *dst){                //Reflect Ray
  float N[3],tmp[3];
  surfaceNormal(gObje, gPoint, fromPoint, N);  //Surface Normal
  mul3c(N,(2 * dot3(ray,N)),tmp);
  sub3(ray,tmp,tmp);
  normalize3(tmp,dst);     //Approximation to Reflection
}

//---------------------------------------------------------------------------------------
//Photon Mapping ------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

void gatherPhotons(float *p, cObje *ob, float *dst){
  float energy[3] = {0.0,0.0,0.0};  
  float N[3],tmp[3];
  int id = ob->getIndex();
  int tp = ob->getType();
  surfaceNormal(ob, p, gOrigin,N);                //Surface Normal at Current Point

  for (int i = 0; i < numPhotons[id]; i++){           //Photons Which Hit Current Object
    if (gatedSqDist3(p,photons[id][i][0],sqRadius)){        //Is Photon Close to Point?
      float weight = max(0.0f, -dot3(N, photons[id][i][1] ));   //Single Photon Diffuse Lighting
      weight *= (1.0 - sqrt(gSqDist)) / exposure;           //Weight by Photon-Point Distance
      mul3c(photons[id][i][2], weight,tmp);
      add3(energy,tmp,energy); //Add Photon's Energy to Total
    }
  } 
  copy3(energy,dst);
}

void emitPhotons(){

  srand(0);                             //乱数表の初期化．毎回同じフォトンが出ることを保証
  for (int t = 0; t < nrObjects; t++)                 //オブジェクトの種類ごとに判定を区分（球，平面）
    numPhotons[t] = 0;


  float rgb[3], ray[3], tmp[3], col[3], prevPoint[3];
  for (int i = 0; i < (view3D ? nrPhotons * 3.0 : nrPhotons); i++){ //フォトンのみ描画のときは多めに出しておく
    int bounces = 1;
    rgb[0] = 1.0; rgb[1] = 1.0; rgb[2] = 1.0;           //フォトンの初期色は白
    rand3(1.0,tmp); normalize3( tmp, ray );             //フォトンの放出方向はランダム
    for(int j=0; j<3; j++) prevPoint[j] = Light[j];                 //フォトンの初期位置は光源

    //光子の初期位置をばらけさせる
    while (prevPoint[1] >= Light[1]){ //y軸下方向に集中
      /**  prevPoint = Light + (0.75e(random))  **/
      rand3(1.0,tmp);
      normalize3(tmp,tmp);
      mul3c(tmp,0.75,tmp);
      add3(Light,tmp,prevPoint);
    }
    //結果として部屋の外，物体の中に入った光子は扱わない
    if (fabs(prevPoint[0]) > 1.5 || fabs(prevPoint[1]) > 1.2 ) bounces = nrBounces+1;
    for(int dx=0; dx<nrObjects; dx++) {
      cObje *ob = objects[dx];
      if(ob->getType() != TYPE_SPHERE) continue;
      if(gatedSqDist3(prevPoint,ob->coords,ob->coords[3]*ob->coords[3])) bounces = nrBounces+1;
    }

    raytrace(ray, prevPoint);                          //Trace the Photon's Path

    while (gIntersect && bounces <= nrBounces){        //Intersection With New Object
      mul3c(ray,gDist,tmp);
      add3( tmp, prevPoint,gPoint);   //3D Point of Intersection
      getColor(rgb,gObje,col);
      mul3c (col, 1.0/sqrt((double)bounces),rgb);
      storePhoton(gObje, gPoint, ray, rgb);  //Store Photon Info

      drawPhoton(rgb, gPoint);                       //Draw Photon
      shadowPhoton(ray);                             //Shadow Photon
      reflect(ray,prevPoint,ray);                  //Bounce the Photon
      raytrace(ray, gPoint);                         //Trace It to Next Location
      copy3(gPoint,prevPoint);
      bounces++;
    }
  }
}

void storePhoton(cObje *ob, float *location, float *direction, float *energy){
  int id = ob->getIndex();
  copy3(location,photons[id][numPhotons[id]][0]);  //Location
  copy3(direction,photons[id][numPhotons[id]][1]); //Direction
  copy3(energy,photons[id][numPhotons[id]][2]);//Attenuated Energy (Color)
  numPhotons[id]++;
}

void shadowPhoton(float *ray){                               //Shadow Photons
  float tmp[3];
  float shadow[3] = {-0.25,-0.25,-0.25};
  float tPoint[3]; copy3(gPoint,tPoint); 
  cObje *tObje = gObje;                         //Save State
  float bumpedPoint[3], shadowPoint[3];
  mul3c(ray,0.00001,tmp);
  add3(gPoint,tmp,bumpedPoint);      //Start Just Beyond Last Intersection
  raytrace(ray, bumpedPoint);                                 //Trace to Next Intersection (In Shadow)
  mul3c(ray,gDist,tmp);
  add3( tmp, bumpedPoint, shadowPoint); //3D Point
  storePhoton(gObje, shadowPoint, ray, shadow);
  copy3(tPoint,gPoint); gObje = tObje;            //Restore State
}



void getColor(float *rgbIn, cObje *ob, float *dst){ //Specifies Material Color of Each Object
  dst[0] = ob->color[0]*rgbIn[0];
  dst[1] = ob->color[1]*rgbIn[1];
  dst[2] = ob->color[2]*rgbIn[2];
}



//---------------------------------------------------------------------------------------
// User Interaction and Display ---------------------------------------------------------
//---------------------------------------------------------------------------------------


void resize(int w, int h) {
  //描画範囲の設定(コールバック不要)
  glViewport(0, 0, WINW, WINH);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0,(double)WINW,0.0,(double)WINH,-10.0,10.0);
}

void display(){  
  if (view3D){
    if (empty){
      glClear(GL_COLOR_BUFFER_BIT);
      emitPhotons();
      empty = false;
      sleep(1); //Emit & Draw Photons
    }
  }else{
    if (empty) render();
    else sleep(1);  //Only Draw if Image Not Fully Rendered
  }
  glFlush();
}

void render(){ //Render Several Lines of Pixels at Once Before Drawing
  int x,y,iterations = 0;
  float rgb[3] = {0.0,0.0,0.0}, tmp[3];

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
      computePixelColor(x,y,tmp);
      mul3c( tmp, 1.0, rgb);               //All the Magic Happens in Here!
      //mul3c( tmp, 255.0, rgb);

      //ピクセルごとに描画していく
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

void drawPhoton(float *rgb, float *p){           //Photon Visualization
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

//---------------------------------------------------------------------------------------
//Mouse and Keyboard Interaction --------------------------------------------------------
//---------------------------------------------------------------------------------------
int prevMouseX = -9999, prevMouseY = -9999, sphereIndex = -1;
float s = 130.0; //マウスでの移動量をあらわす定数
float constrain(float src, float lower, float upper) { return min(upper, max(src, lower)); }

void KeyFunc(unsigned char key,int, int) {
  switch(key) {
    case 49 /*1*/ : view3D = false; lightPhotons = false; break;
    case 50 /*2*/ : view3D = false; lightPhotons = true; break;
    case 51 /*3*/ : view3D = true; break;
    default     : return;
  }
  resetRender();
  printf("No. %d key pressed\n",key);
}
void MouseButtonFunc(int button,int action, int x, int y) {
  if(button != 0) return;     //左クリック以外は無効にする
  if(action == 0) {         //押したとき
    mouseDragging = true;
    sphereIndex = nrObjects; //Click Spheres
    mouseX = x;
    mouseY = y;
    float mouse3[3] = {(mouseX - szImg/2)/s, -(mouseY - szImg/2)/s, 2.5};
    for(int i=0; i<nrObjects; i++) {
      if (objects[i]->getType() != TYPE_SPHERE) continue;
      if (gatedSqDist2(mouse3,objects[i]->coords,objects[i]->coords[3])) sphereIndex = i;
    }
    printf("sphere %d\n",sphereIndex);
    //else if (gatedSqDist3(mouse3,spheres[1],spheres[1][3])) sphereIndex = 1;
  }else {             //離したとき
    prevMouseX = -9999;
    prevMouseY = -9999;
    mouseDragging = false;
  }
}
void MousePosFunc(int x,int y) {
  mouseX = x;
  mouseY = y;
  if(mouseDragging) {
    if (prevMouseX > -9999 && sphereIndex > -1){
      if (sphereIndex < nrObjects){ //Drag Sphere
        objects[sphereIndex]->coords[0] += (mouseX - prevMouseX)/s;
        objects[sphereIndex]->coords[1] -= (mouseY - prevMouseY)/s;
      }else{ //Drag Light
        Light[0] += (mouseX - prevMouseX)/s; Light[0] = constrain(Light[0],-1.4,1.4);
        Light[1] -= (mouseY - prevMouseY)/s; Light[1] = constrain(Light[1],-0.4,1.2);}
        resetRender();
    }
    prevMouseX = mouseX;
    prevMouseY = mouseY;
  }
}


void initObje() {
  float white[3] = {1.0,1.0,1.0};
  float red[3] = {1.0,0.0,0.0};
  float green[3] = {0.0,1.0,0.0};
  float blue[3] = {0.0,0.0,1.0};
  float v_sphere[][4] = {
    {1.0,0.0,4.0,0.5},
    {-0.6,-1.0,4.5,0.5},
    {0.0,0.0,3.0,0.2},
  };//Sphere Center & Radius
  float v_plane[][2]  = {
    {0, 1.5},
    {1, -1.5},
    {0, -1.5},
    {1, 1.5},
    {2,5.0}
  };//Plane Axis & Distance-to-Origin

  cObje *ob;
  objects.resize(0);
  for(int i=0; i<3; i++) {
    ob = new cObje(TYPE_SPHERE,nrObjects++,v_sphere[i]);
    objects.push_back(ob);
  }
  objects[2]->setOptics(OPT_REFLECT);

  for(int i=0; i<5; i++) {
    ob = new cObje(TYPE_PLANE,nrObjects++,v_plane[i]);
    objects.push_back(ob);
  }
  objects[4]->setColor(green);
  objects[6]->setColor(red);



  //鏡面効果や色の変更
  //spheres[0]->setMirror(true);
  //spheres[1]->setMirror(true);
  //spheres[2]->setMirror(true);
  //planes[0]->setColor(green);
  //planes[2]->setColor(red);

}

void freeObje() {
  //for(int i=0; i<nrObjects[TYPE_SPHERE]; i++) delete spheres[i];
  //for(int i=0; i<nrObjects[TYPE_PLANE]; i++) delete planes[i];
  for(int i=0; i<nrObjects; i++) delete objects[i];
  objects.clear();
}

void termination() {
  freeObje();
  return ;
}

void timer(int val) {
  glutPostRedisplay();  //redraw
  glutTimerFunc(10,timer,val);
}

int main(int argc, char *argv[]) {

  initObje();
  //GLUTの初期化
  glutInit(&argc,argv);
  glutInitWindowPosition(WPOSX, WPOSY);             // ウィンドウの表示位置
  glutInitWindowSize(WINW, WINH);               // ウィンドウのサイズ
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH);  // ディスプレイ設定
  glutCreateWindow("GLUT Template");          // ウィンドウの生成

  //コールバック関数のセット
  glutDisplayFunc(display);     // 描画処理関数の設定
  glutTimerFunc(10,timer,0);        // アイドル時の処理関数の設定
  glutReshapeFunc(resize);    // ウィンドウ変形時の処理を行う関数の設定
  glutKeyboardFunc(KeyFunc);    // キー入力時の処理関数の設定
  glutMouseFunc(MouseButtonFunc);     // マウス入力時の処理関数の設定
  glutMotionFunc(MousePosFunc);     // マウスドラッグ時の処理関数の設定

  // 終了処理関数の設定
  atexit(termination);

  emitPhotons();    //フォトンをばらまいておく
  resetRender();    //レンダリング状態の初期化

  glutMainLoop();

  return 1;
}
