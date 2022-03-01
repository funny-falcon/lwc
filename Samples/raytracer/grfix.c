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
#include <sys/time.h>
#include "scr.h"

#define BPP 2
#define bPP (BPP*8)
#define SCRMEM (SCRX*SCRY*BPP)

static Display*		D;
static Window		W;
static GC		oneGConly;	/* ? the right way */
static XImage*		image;
static XShmSegmentInfo	X_shminfo;

scrdata_t *screen;

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

int XShmEvent;

int x_input ()
{
	if (!XEventsQueued (D, QueuedAfterFlush))
		return -1;

	XEvent e;

	XNextEvent (D, &e);

	switch (e.type) {
	case KeyPress: {
		KeySym ks;
		char inkey [2];

		XLookupString (&e.xkey, inkey, 2, &ks, 0);
		if (ks >= XK_space && ks < XK_asciitilde)
			return inkey [0];
		return -2;
	}
	}

	return -1;
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

	if (!(D = XOpenDisplay (NULL))) Fawk ("Can't open X11 display!");

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

	XSelectInput (D, W, KeyPressMask|ExposureMask);

	if (!XShmQueryExtension (D)) Fawk ("Non local display or shm not supported?");

	image = XShmCreateImage (D, DefaultVisual (D, DefaultScreen (D)),
				 bPP, ZPixmap, 0, &X_shminfo, SCRX, SCRY);

	X_shminfo.shmid = shmget (IPC_PRIVATE, SCRMEM, IPC_CREAT|0777);
	if (X_shminfo.shmid < 0) Fawk ("1");
	image->data = X_shminfo.shmaddr = (char*) shmat (X_shminfo.shmid, 0, 0);
	X_shminfo.readOnly = 0;
	if (!XShmAttach (D, &X_shminfo)) Fawk ("Attachments");

	screen = (scrdata_t*) image->data;

	XShmEvent =  XShmGetEventBase (D) + ShmCompletion;

	return 0;
}

#define COUNTFPS

#ifdef COUNTFPS
static int FPC;
static void fps ()
{
#define CUTBASE 100
static	struct timeval t1;
	//screen [FPC] = screen [FPC+SCRX] = screen [FPC+SCRX*2] = 
	//screen [FPC+3*SCRX] = FPC*32;
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
#endif

void v_blitt ()
{
	XSync (D, False);
	XShmPutImage (D, W, oneGConly, image, 0, 0, 0, 0, SCRX, SCRY, 1);
	XFlush (D);
#ifdef COUNTFPS
	fps ();
#endif
}

void max_blitt ()
{
#ifdef COUNTFPS
	printf ("Counting maximum blit capability of device\n");
	int i;
	for (i=0;i<2*CUTBASE + 2;i++)
		v_blitt ();
#endif
}
