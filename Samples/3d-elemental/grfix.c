#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/extensions/XShm.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "global.h"

#define BPP 2
#define bPP (BPP*8)
#define SCRMEM (SCRX*SCRY*BPP)

static Display*		D;
static Window		W;
static GC		oneGConly;	/* ? the right way */
static XImage*		image;
static XShmSegmentInfo	X_shminfo;

static struct {
	int col;
	const char *cname;
} colors [] = {
	{ 0, "black" },
	{ 0, "white" },
	{ 0, "yellow" },
	{ 0, "sandy brown" },
	{ 0, "dark red" },
	{ 0, "blue" },
	{ 0, "dark green" },
	{ 0, "light grey" },
	{ 0, "dark grey" }
};

#define NCOLORS (sizeof colors / sizeof colors [0])

static void init_colors ()
{
	unsigned int	i;
	XColor		g, d;
	Colormap	cmap = DefaultColormap (D, XDefaultScreen (D));
	XGCValues	v;

	for (i = 0; i < NCOLORS; i++) {
		XAllocNamedColor (D, cmap, colors [i].cname, &g, &d);
		colors [i].col = g.pixel;
	}

	v.fill_style = FillSolid;
	oneGConly = XCreateGC (D, W, GCFillStyle, &v);
}

static void Fawk (char *d)
{
	fprintf (stderr, "Fatal: %s\n", d);
	exit (1);
}

int initialize_graphics ()
{
	XSizeHints		sh;
	XSetWindowAttributes	attr;

	if (!(D = XOpenDisplay (NULL))) return -1;

	attr.backing_store = Always;
	W = XCreateWindow (D, DefaultRootWindow (D), 0, 0, SCRX, SCRY, 1,
			   CopyFromParent, CopyFromParent, CopyFromParent,
			   CWBackingStore, &attr);

	sh.min_width = sh.max_width = SCRX;
	sh.min_height = sh.max_height = SCRY;
	sh.flags = PMinSize | PMaxSize;
	XSetNormalHints (D, W, &sh);

	XMapWindow (D, W);
	XFlush (D);

	XStoreName (D, W, "hello");
	init_colors ();
	XSelectInput (D, W, ButtonPressMask);

	if (!XShmQueryExtension (D)) Fawk ("Fawked");

	image = XShmCreateImage (D, DefaultVisual (D, DefaultScreen (D)),
				 bPP, ZPixmap, 0, &X_shminfo, SCRX, SCRY);
	X_shminfo.shmid = shmget (IPC_PRIVATE, SCRMEM, IPC_CREAT|0777);
	if (X_shminfo.shmid < 0) Fawk ("1");
	image->data = X_shminfo.shmaddr = (char*) shmat (X_shminfo.shmid, 0, 0);
	X_shminfo.readOnly = 0;
	if (!XShmAttach (D, &X_shminfo)) Fawk ("Attachments");
	memset (image->data, 0, SCRMEM);

	return 0;
}

static int FPC;
static void fps ()
{
#define CUTBASE 100
static	struct timeval t1;
	unsigned short int *screen = (unsigned short int*)image->data;
	screen [FPC] = screen [FPC+SCRX] = screen [FPC+SCRX*2] = 
	screen [FPC+3*SCRX] = FPC*32;
	if (FPC++ == 0) {
		if (t1.tv_sec) {
			struct timeval t2;
			unsigned long int diff;

			gettimeofday (&t2, NULL);
			t2.tv_sec -= t1.tv_sec;
			t2.tv_usec -= t1.tv_usec;
			diff = t2.tv_sec * 1000000 + t2.tv_usec;

			printf ("fps = %li\n", 1000000*CUTBASE / diff);
		}
		gettimeofday (&t1, NULL);
	}
	FPC %= CUTBASE;
}

void v_blitt ()
{
	XSync (D, False);
	XShmPutImage (D, W, oneGConly, image, 0, 0, 0, 0, SCRX, SCRY, 1);
	XFlush (D);
	memset (image->data, 0, SCRMEM);
	fps ();
}

static int ccol;
void v_setcolor (int c)
{
	ccol = c;
}

void v_vline (int x, int y1, int y2)
{
	unsigned short int *screen = (unsigned short int*) image->data;

	if (x < 0 || x >= SCRX) return;
	if (y2 < y1) {
		int i = y1;
		y1 = y2;
		y2 = i;
	}
	if (y1 >= SCRY || y2 < 0) return;
	if (y2 >= SCRY) y2 = SCRY-1;
	if (y1 < 0) y1 = 0;

	screen += x + y1 * SCRX;
	for (y2 -= y1; y2; y2--, screen += SCRX)
		screen [0] = ccol;
}

void v_hline (int x1, int x2, int y)
{
	unsigned short int *screen = (unsigned short int*) image->data;

	if (x2 < x1) {
		int i = x1;
		x1 = x2;
		x2 = i;
	}

	screen += x1 + SCRX * y;
	for (x2 -= x1; x2; x2--)
		*screen++ = ccol;
}

/* Clipping based heavily on code from                       */

/* http://www.ncsa.uiuc.edu/Vis/Graphics/src/clipCohSuth.c   */

/* Severely corrected code. sxanth				*/

#define CLIP_LEFT_EDGE   0x1
#define CLIP_RIGHT_EDGE  0x2
#define CLIP_BOTTOM_EDGE 0x4
#define CLIP_TOP_EDGE    0x8
#define CLIP_INSIDE(a)   (!a)
#define CLIP_REJECT(a,b) (a&b)
#define CLIP_ACCEPT(a,b) (!(a|b))
#define CLIPX (SCRX-1)
#define CLIPY (SCRY-1)

static inline int clipEncode(int x, int y)
{
	int code = 0;

	if (x < 0) code |= CLIP_LEFT_EDGE;
	else if (x > CLIPX) code |= CLIP_RIGHT_EDGE;
	if (y < 0) code |= CLIP_TOP_EDGE;
	else if (y > CLIPY) code |= CLIP_BOTTOM_EDGE;
	return code;
}

static inline int clipLine(int *px1, int *py1, int *px2, int *py2)
{
	int code1, code2;
	float m;
	int x1 = *px1, y1 = *py1, x2 = *px2, y2 = *py2;

	while (1) {
		code1 = clipEncode(x1, y1);
		code2 = clipEncode(x2, y2);
		if (CLIP_ACCEPT(code1, code2)) return 1;
		if (CLIP_REJECT(code1, code2)) return 0;
		if (!CLIP_INSIDE(code1)) {
			m = (x2 == x1) ? 1.0f :
				(y2 - y1) / (float) (x2 - x1);
			if (code1 & CLIP_LEFT_EDGE) {
				*py1 = y1 += (int) ((0 - x1) * m);
				*px1 = x1 = 0;
			} else if (code1 & CLIP_RIGHT_EDGE) {
				*py1 = y1 += (int) ((CLIPX - x1) * m);
				*px1 = x1 = CLIPX;
			} else if (code1 & CLIP_BOTTOM_EDGE) {
				if (x2 != x1)
					*px1 = x1 += (int) ((CLIPY - y1) / m);
				*py1 = y1 = CLIPY;
			} else {
				if (x2 != x1)
					*px1 = x1 += (int) ((0 - y1) / m);
				*py1 = y1 = 0;
			}
		}
		if (!CLIP_INSIDE(code2)) {
			m = (x2 == x1) ? 1.0f :
				(y1 - y2) / (float) (x1 - x2);
			if (code2 & CLIP_LEFT_EDGE) {
				*py2 = y2 += (int) ((0 - x2) * m);
				*px2 = x2 = 0;
			} else if (code2 & CLIP_RIGHT_EDGE) {
				*py2 = y2 += (int) ((CLIPX - x2) * m);
				*px2 = x2 = CLIPX;
			} else if (code2 & CLIP_BOTTOM_EDGE) {
				if (x2 != x1)
					*px2 = x2 += (int) ((CLIPY - y2) / m);
				*py2 = y2 = CLIPY;
			} else {
				if (x2 != x1)
					*px2 = x2 += (int) ((0 - y2) / m);
				*py2 = y2 = 0;
			}
		}
    }

    return 1;
}

void v_putpixel (int x, int y)
{
	unsigned short int *screen = (unsigned short int*) image->data;
	if (x >= 0 && x <= CLIPX && y >= 0 && y <= CLIPY)
		screen [x + SCRX * y] = ccol;
}

void v_line (int x1, int y1, int x2, int y2)
{
	unsigned short int *screen = (unsigned short int*) image->data;
	int x, y, dx, dy, sx, sy;
	int pixx, pixy;

	if (!clipLine (&x1, &y1, &x2, &y2)) return;
	if (0 == (dx = x2 - x1)) {
		if (y2 - y1 == 0) v_putpixel (x1, y1);
		else v_vline (x1, y1, y2);
		return;
	}
	if (0 == (dy = y2 - y1)) {
		v_hline (x1, x2, y1);
		return;
	}

	pixx = sx = dx > 0 ? 1 : -1;
	pixy = SCRX * (sy = dy > 0 ? 1 : -1);
	dx = sx * dx + 1;
	dy = sy * dy + 1;
	if (dx < dy) {
		int s = pixx;
		pixx = pixy;
		pixy = s;
		s = dx;
		dx = dy;
		dy = s;
	}

	screen += x1 + SCRX * y1;
	for (x = y = 0; x < dx; x++, screen += pixx) {
		screen [0] = ccol;
		y += dy;
		if (y >= dx) {
			y -= dx;
			screen += pixy;
		}
	}
}

