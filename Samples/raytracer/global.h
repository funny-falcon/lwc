
#include "system.h"
#include "scr.h"
#include "vector.h"
#include "prefs.h"

#define METRIC (30.0f)

typedef int RGB;

static inline RGB rgbcolor (int r, int g, int b)
{
	r&=0xf8, g&=0xfc;
	r<<=8, g<<=3, b>>=3;
	return r|g|b;
}

extern unsigned short int *screen;
extern int initialize_graphics ();
extern void v_blitt ();
extern int x_input ();
extern void max_blitt ();

static inline bool zero (float f)
{
	return fabsf (f) < 0.003f;
}

/*
 * Crawler module
 */

/*
 * General surface properties
 */
class surface
{
	inline virtual;
	RGB plaincol;
   public:
	float l1;
	bool hits		()	{ return false; }
	RGB hitcolor		()	{ return plaincol; }
	void precalc_for_viewer	()	{ }
virtual	void land		()	{ }
};

/*
 * the coordinates of the ray's start and direction
 */
extern Vec Viewer, Ray;

extern Vec DV1, DVx, DVy;

/*
 *
 */

void gauss (float*);
int gauss_check (float);
