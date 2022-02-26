
#define NL "\n"

static inline FILE *operator, (FILE *f, char *s)
{
	fprintf (f, "%s", s);
	return f;
}

static inline FILE *operator, (FILE *f, int i)
{
	fprintf (f, "%i", i);
	return f;
}

static inline FILE *operator (FILE *f, void *p)
{
	fprintf (f, "%p", p);
	return f;
}

static inline FILE *operator!! (char *s)
{
	return stderr, s;
}

static inline FILE *operator postfix !!  (char *s)
{
	return stdout, s;
}

linkonce int cc_pchar;
class _pchar_ {} *CHAR (char c)	{ cc_pchar = c; return 0; }

FILE *operator, (FILE *f, _pchar_ *p)
{
	if (cc_pchar < ' ' || cc_pchar > '~')
		fprintf (f, "'\\x%x'", cc_pchar);
	else	fprintf (f, "'%c'", cc_pchar);
	return f;
}

static inline void operator ~ (void *p)
{
	free (p);
}
