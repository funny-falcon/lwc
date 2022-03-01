
// :::::::::::::::::::::::::::::::::::::::::::
// Special values to the important keys
// :::::::::::::::::::::::::::::::::::::::::::

enum {
 K_SKIP = 1000,
 K_UP,
 K_DOWN,
 K_LEFT,
 K_RIGHT,
 K_DEL,
 K_END,
 K_HOME,
 K_PGUP,
 K_PGDOWN,
 K_ESC
};

class line_struct
{
	int allocd;
   public:
	line_struct ();
	int cursor, linelen;
	char *text;
	void add_char (char);
	void del ();
	void backspace ();
	void preset (char* = 0);
	void strcat (char*);
	~line_struct ();
};

class base_readline
{
	int maxlen, cc;
virtual	void fix_vcur	();
virtual void do_tab	() = 0;
virtual	void do_varrow	() = 0;
virtual void do_harrow	();
virtual void do_enter	() = 0;
virtual void do_bsdel	();
virtual void do_pg	() = 0;
virtual void do_hoend	();
virtual	void do_cchar	();
virtual void present	() = 0;
   public:
	base_readline (char * = 0, int = 1024);
	line_struct Line;
	char *prompt;
	int width, vcur;
	void do_char (int);
virtual ~base_readline	();	
};
