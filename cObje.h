//cObje.h


#define TYPE_SPHERE		0
#define TYPE_PLANE		1
#define TYPE_TRIANGLE	2

#define OPT_NONE 0
#define OPT_REFLECT 1
#define OPT_REFRACT 2

class cObje {
/**ä÷êî**/
public :
	cObje(int type, int idx, float *cod);
	int getType();
	int getOptics();
	int getIndex();
	void setOptics(int op);
	void setColor(float *cl);
	float coords[9];
	float color[3];

/**ïœêî**/
private :
	int type;
	int optic;
	int index;
	float refractive;
};

extern cObje *gObje;
extern float gRefractive;