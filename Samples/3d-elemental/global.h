#ifndef __LWC__
#include <stdio.h>
#include <stdlib.h>
#define class c_class
#define new c_new
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#undef class
#undef new
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#endif



#define SCRX 800
#define SCRY 800

extern int initialize_graphics ();
extern void v_setcolor (int);
extern void v_line (int, int, int, int);
extern void v_vline (int, int, int);
extern void v_putpixel (int, int);
extern void v_blitt ();

static inline int rgbcolor (int r, int g, int b)
{
	r&=0xf8, g&=0xfc;
	r<<=8, g<<=3, b>>=3;
	return r|g|b;
}


#define COL_BLACK	0
#define COL_WHITE	1
#define COL_YELLOW	2
#define COL_BROWN	3
#define COL_DRED	4
#define COL_DGREEN	5
#define COL_LGREEN	6
#define COL_DGREY	7

extern int randup (int);
extern double random_gaussian ();
extern void t2_function (float[], int, int, float, int);

