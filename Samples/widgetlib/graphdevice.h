
_lwc_config_ {
//	lwcdebug TDEF_TRACE;
//	lwcdebug DCL_TRACE;
};
/*******************************************
##  The X headers, use the reserved words
##  class and new as variable names, unless
##  __cplusplus is defined, in which case they
##  use c_class and c_new as the variable
##  names.  We do this here to avoid the
##  breakage.  They *must* be undefined afer
*********************************************/
#define class c_class;
#define new c_new
extern "X11/Xlib.h" {
#include <X11/Xlib.h>
}
extern "X11/Xutil.h" {
#include <X11/Xutil.h>
}
#undef class
#undef new
/*************************************
###
*************************************/

enum color_n {
	COLOR_BLACK, COLOR_WHITE, COLOR_YELLOW, COLOR_SANDYBROWN,
	COLOR_DARKRED, COLOR_BLUE, COLOR_DARKGREEN, COLOR_LIGHGREY,
	COLOR_DARKGREY, COLOR_NCOLORS
};

enum font_n {
	FONT_NORMAL, FONT_BOLD, FONT_SMALL, FONT_NFONTS
};

class graphdevice
{
   public:
	int scrX, scrY;
virtual	void DrawLine (int, int, int, int, color_n) = 0;
virtual	void DrawPoint (int, int, color_n) = 0;
virtual	void FillRect (int, int, int, int, color_n) = 0;
virtual void DrawRect (int, int, int, int, color_n) = 0;
virtual	void DrawText (int, int, char*, font_n, color_n) = 0;
virtual	void DrawText (int, int, char*, font_n, color_n, color_n) = 0;
virtual void *SaveUnders (int, int, int, int) = 0;
virtual void RestoreUnders (void*) = 0;
virtual	void Flush () = 0;
};

class XWinDevice : graphdevice
{
	Display *D;
	Window W;
	GC xGC;
	int colorval [COLOR_NCOLORS];
	XFontStruct *fontval [FONT_NFONTS];

	color_n curfg, curbg;
	void init_colors ();
	void init_fonts ();
	void setfg (color_n);
	void setbg (color_n);
	void setfont (font_n);
    public:
	void DrawLine (int, int, int, int, color_n);
	void DrawPoint (int, int, color_n);
	void FillRect (int, int, int, int, color_n);
	void DrawRect (int, int, int, int, color_n);
	void DrawText (int, int, char*, font_n, color_n);
	void DrawText (int, int, char*, font_n, color_n, color_n);
	void *SaveUnders (int, int, int, int);
	void RestoreUnders (void*);
	void Flush ();

	XWinDevice (char*, int, int, char* = 0);
	bool success;
	~XWinDevice ();
};

