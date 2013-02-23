#include "cObje.h"

cObje::cObje(int tp, int idx, float *cod) {
	
	index = idx;
	optic = OPT_NONE;
	refractive = 1.0;
	type = tp;

	int ncods = 0;
	switch(tp) {
	case TYPE_SPHERE : ncods=4; break;
	case TYPE_PLANE : ncods=2; break;
	}
	for(int i=0; i<ncods; i++) coords[i] = cod[i];
	for(int i=0; i<3; i++) color[i] = 1.0;
}

int cObje::getType() { return type; }
int cObje::getOptics() { return optic; }
int cObje::getIndex() { return index; }
void cObje::setOptics(int op) { optic = op; }
void cObje::setColor(float *cl) { for(int i=0; i<3; i++) color[i] = cl[i]; }