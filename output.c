#include <stdarg.h>
#include "global.h"

#define CHUNK 1022

typedef struct ms_t {
	int data [CHUNK];
	struct ms_t *next;
	int i;
} ms;

struct outstream {
	ms *first, *last;
	int outtok;
};

OUTSTREAM GLOBAL, GLOBAL_INIT_FUNC, FUNCDEFCODE, INTERNAL_CODE, FPROTOS, AUTOFUNCTIONS;
OUTSTREAM VTABLE_DECLARATIONS, VIRTUALTABLES, STRUCTS, GVARS, INCLUDE, REGEXP_CODE;

/********************************************************************


********************************************************************/

OUTSTREAM new_stream ()
{
	OUTSTREAM r = (OUTSTREAM) malloc (sizeof (struct outstream));
	r->first = r->last = (ms*) malloc (sizeof (ms));
	r->first->next = 0;
	r->outtok = r->first->i = 0;
	return r;
}


int ntokens (OUTSTREAM o)
{
	return o->outtok;
}

inline void output_itoken (OUTSTREAM o, int i)
{
	if (!o) return;
	if (o->last->i == CHUNK) {
		o->last->next = (ms*) malloc (sizeof (ms));
		o->last = o->last->next;
		o->last->i = 0;
		o->last->next = NULL;
	}
	o->last->data [o->last->i++] = i;
	o->outtok++;
}

int *combine_output (OUTSTREAM o)
{
	if (!o) {
		int *ret = (int*) malloc (sizeof ret);
		ret [0] = -1;
		return ret;
	}

	int i, k;
	ms *p, *n;
	
	int *ret = (int*) malloc ((1 + o->outtok) * sizeof (int));

	for (k = 0, p = o->first; p; p = n) {
		for (i = 0; i < p->i; i++)
			ret [k++] = p->data [i];
		n = p->next;
		free (p);
	}
	ret [k] = -1;
	free (o);
	return ret;
}

void free_stream (OUTSTREAM o)
{
	ms *p, *n;

	for (p = o->first; p; p = n) {
		n = p->next;
		free (p);
	}
	free (o);
}

void backspace_token (OUTSTREAM o)
{
	if (!o) return;

	if (o->last->i == 0) {
		ms *p;
		for (p = o->first; p->next != o->last; p = p->next);
		free (o->last);
		o->last = p;
		p->next = 0;
	}

	--o->last->i;
	--o->outtok;
//	o->last->data [o->last->i - 1] = BLANKT;
}

OUTSTREAM concate_streams (OUTSTREAM d, OUTSTREAM s)
{
	if (!d || !s) return 0;

	int i;
	ms *p, *n;

	for (p = s->first; p; p = n) {
		for (i = 0; i < p->i; i++)
			output_itoken (d, p->data [i]);
		n = p->next;
		free (p);
	}
	free (s);
	return d;
}

void outprintf (OUTSTREAM o, Token first, ...)
{
	if (!o) return;

	Token *str;
	int n;
	va_list ap;
	va_start (ap, first);
	while (first != -1) {
		if (first == STRNEXT) {
			str = va_arg (ap, Token*);
			while (*str != EOST)
				output_itoken (o, *str++);
		} else if (first == NSTRNEXT) {
			str = va_arg (ap, Token*);
			n = va_arg (ap, int);
			while (n--) output_itoken (o, *str++);
		} else if (first == BACKSPACE)
			backspace_token (o);
		else if (first != BLANKT)
			output_itoken (o, first);
		first = va_arg (ap, Token);
	}
	va_end (ap);
}

Token *sintprintf (Token *buf, Token first, ...)
{
	Token *r = buf;
	Token *str;
	int n;
	va_list ap;
	va_start (ap, first);
	while (first != -1) {
		if (first == STRNEXT) {
			str = va_arg (ap, Token*);
			while (*str != EOST)
				*buf++ = *str++;
		} else if (first == NSTRNEXT) {
			str = va_arg (ap, Token*);
			n = va_arg (ap, int);
			while (n--) 
				*buf++ = *str++;
		} else if (first == NONULL)
			goto ret_nonull;
		else if (first != BLANKT)
			*buf++ = first;
		first = va_arg (ap, Token);
	}
	*buf = -1;
ret_nonull:
	va_end (ap);
	return r;
}

int get_stream_pos (OUTSTREAM o)
{
	return o->outtok;
}

void wipeout_unwind (OUTSTREAM o, int pos)
{
	int i, j;
	ms *p;

	for (i = CHUNK, p = o->first; i < pos; p = p->next, i += CHUNK);
	for (j = pos % CHUNK; p; p = p->next, j = 0)
		for (i = j; i < p->i; i++)
			if (p->data [i] == UWMARK)
//{PRINTF ("wipeout in %s\n", expand (in_function));
				for (;;) {
					p->data [i] = NOTHING;
					if (++i == p->i) i = 0, p = p->next;
					if (p->data [i] == UWMARK) {
						p->data [i] = NOTHING;
						break;
					}
				}
//}
}

void nowipeout_unwind (OUTSTREAM o, int pos)
{
	int i, j;
	ms *p;

	for (i = CHUNK, p = o->first; i < pos; p = p->next, i += CHUNK);
	for (j = pos % CHUNK; p; p = p->next, j = 0)
		for (i = j; i < p->i; i++)
			if (p->data [i] == UWMARK) p->data [i] = NOTHING;
}

/* This can be gone 
void substitute_output (OUTSTREAM o, Token t, Token r)
{
	int i, k;
	ms *p;
	for (k = 0, p = o->first; p; p = p->next)
		for (i = 0; i < p->i; i++)
			if (p->data [i] == t)
				p->data [i] = r;
}	*/

void export_output (OUTSTREAM o)
{
	int i; //, k;
	ms *p;
	FILE *pstream = stdout;
#ifdef DEBUG
	if (debugflag.OUTPUT_INDENTED||1) {
		/* Close stderr because indent prints stupid errors */
		close (2);
		if (!(pstream = popen ("indent -kr -st -l90", "w"))) {
			fprintf (stderr, "cannot find the 'indent' program."
				" output is one big line of tokens\n");
			pstream = stdout;
		}
	}
	if (debugflag.GENERAL)
		PRINTF ("+++++++++++++ PROGRAM +++++++++++++\n");
#endif
	for (/*k = 0,*/ p = o->first; p; p = p->next)
		for (i = 0; i < p->i; i++) {
			fputs (expand (p->data [i]), pstream);
			fputc (' ', pstream);
#ifdef DEBUG
			if (debugflag.OUTPUT_INDENTED)
			if (p->data [i] == '{' || p->data [i] == '}' || p->data [i] == ';'
			|| (p->data [i] == ',' && p->data [i+1] == '.'))
				fputc ('\n', pstream);
#endif
		}
	fputc ('\n', pstream);
	fflush (pstream);
#ifdef DEBUG
	if (pstream != stdout)
		pclose (pstream);
#endif
}
