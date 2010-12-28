#include "global.h"

/* escape backslashes and double-quotes */
char *escape_c_string (char *str, int len)
{
	int i, slen;
	char *ret;

	for (i = 0, slen = 4; i < len; i++, slen++)
		if (str [i] == '\n' || str [i] == '\t'
		||  str [i] == '"' || str [i] == '\\')
			++slen;

	ret = (char*) malloc (slen);

	ret [0] = '"';
	for (i = 0, slen = 1; i < len; i++)
		switch (str [i]) {
		case '\n':  ret [slen++] = '\\';
			    ret [slen++] = 'n';
		ncase '\t': ret [slen++] = '\\';
			    ret [slen++] = 't';
		ncase '"':
		 case '\\': ret [slen++] = '\\';
		default: ret [slen++] = str [i];
		}
	ret [slen++] = '"';
	ret [slen] = 0;

	return ret;
}

static char *r_processor (char *str, int len)
{
	return escape_c_string (str, len);
}

static char *invalid (char *str, int len)
{
	fprintf (stderr, "No processor defined for %c(%i)\n", processor, processor);
	parse_error_ll ("");
	return 0;
}

text_processor TP [128];

void init_processors ()
{
	int i;
	for (i = 0; i < 128; i++)
		TP [i] = invalid;
	TP ['r'] = r_processor;
}
