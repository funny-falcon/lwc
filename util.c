#include "global.h"

NormPtr skip_brackets (NormPtr p)
{
	NormPtr perr = p;
	int po = 1;
	while (CODE [p] != -1 && po)
		switch (CODE [p++]) {
		 case '[': po++;
		ncase RESERVED_oper_array:
		 case ']': po--;
		}
	if (CODE [p] == -1)
		parse_error (perr, "Unclosed '[' :[");
	return p;
}

NormPtr skip_braces (NormPtr p)
{
	NormPtr perr = p;
	int po = 1;
	while (CODE [p] != -1 && po)
		switch (CODE [p++]) {
		 case '{': po++;
		ncase '}': po--;
		}
	if (CODE [p] == -1)
		parse_error (perr, "Unclosed '{' :{");
	return p;
}

/* Start RIGHT AFTER the parenthesis. (never remember that) */
NormPtr skip_parenthesis (NormPtr p)
{
	NormPtr perr = p;
	int po = 1;
	while (CODE [p] != -1 && po)
		switch (CODE [p++]) {
		 case '(': po++;
		ncase ')': po--;
		}
	if (CODE [p] == -1)
		parse_error (perr, "Unclosed '(' :(");
	return p;
}

NormPtr skip_buffer_parenthesis (Token *C, NormPtr p)
{
	int po = 1;
	while (C [p] != -1 && po)
		switch (C [p++]) {
		 case '(': po++;
		ncase ')': po--;
		}
	if (C [p-1] == -1)
		parse_error_ll ("Unclosed '(' :(");
	return p;
}

NormPtr skip_buffer_brackets (Token *C, NormPtr p)
{
	int po = 1;
	while (C [p] != -1 && po)
		switch (C [p++]) {
		 case '[': po++;
		ncase ']': po--;
		}
	if (C [p-1] == -1)
		parse_error_ll ("Unclosed '[' :[");
	return p;
}

NormPtr skip_buffer_braces (Token *C, NormPtr p)
{
	int po = 1;
	while (C [p] != -1 && po)
		switch (C [p++]) {
		 case '{': po++;
		ncase '}': po--;
		}
	if (C [p-1] == -1)
		parse_error_ll ("Unclosed '{' :{");
	return p;
}

NormPtr skip_declaration (NormPtr p)
{
	bool obj_after_brace = isaggrspc (CODE [p]) || CODE [p] == RESERVED_specialize
		 || (CODE [p] == RESERVED_typedef
		 && (isaggrspc (CODE [p + 1]) || CODE [p+1] == RESERVED_specialize));

	if (CODE [p + 1] == ':'
	&& (CODE [p] == RESERVED_private || CODE [p] == RESERVED_public))
		return p + 2;
	for (;;) switch (CODE [p]) {
		 case ';': return p + 1;
		 case '{': p = skip_braces (p + 1);
			if (!obj_after_brace) return p;
		ncase '(': p = skip_parenthesis (p + 1);
		ncase '[': p = skip_brackets (p + 1);
		ndefault: ++p;
		}
}

bool intchr (int *s, int t)
{
	while (*s != -1)
		if (*s++ == t) return true;
	return false;
}

void intsubst (int *str, int t1, int t2)
{
	int i;
	for (i = 0; str [i] != -1; i++)
		if (str [i] == t1)
			str [i] = t2;
}

void intsubst1 (int *str, int t1, int t2)
{
	int i;
	for (i = 0; str [i] != -1; i++)
		if (str [i] == t1) {
			str [i] = t2;
			break;
		}
}

Token *intcpy (int *dest, int *src)
{
	Token *r = dest;
	do *dest++ = *src;
	while (*src++ != -1);
	return r;
}

void intextract (int *dest, int *src, int len)
{
	while (len--)
		*dest++ = *src++;
	*dest = -1;
}

int intlen (int *t)
{
	int i;
	for (i = 0; *t != -1; t++, i++);
	return i;
}

int *argtdup (typeID *src)
{
	int *dest, *r;
	int len;

	for (len = 0; src [len] != INTERNAL_ARGEND; len++)
		;
	r = dest = (int*) malloc ((len + 1) * sizeof (int));
	do *dest++ = *src;
	while (*src++ != INTERNAL_ARGEND);
	*(dest - 1) = -1;
	return r;
}

int *intdup1 (int *src)
{
	int *dest;
	int len;

	for (len = 0; src [len] != -1; len++)
		;
	dest = (int*) malloc ((len + 2) * sizeof (int));
	intcpy (dest, src);
	return dest;
}

int *intdup (int *src)
{
	int *dest;
	int len;

	for (len = 0; src [len] != -1; len++)
		;
	dest = (int*) malloc ((len + 1) * sizeof (int));
	intcpy (dest, src);
	return dest;
}

int *intndup (int *src, int len)
{
	int *dest = (int*) malloc ((len + 1) * sizeof (int));
	intextract (dest, src, len);
	return dest;
}

int intcmp (int *p1, int *p2)
{
	while (*p1 == *p2 && *p1 != -1)
		p1++, p2++;
	return *p1 == *p2 ? 0 : *p1 < *p2 ? -1 : 1;
}

void intcat (int *dest, int *src)
{
	while (*dest != -1) dest++;
	intcpy (dest, src);
}

void intncat (int *dest, int *src, int len)
{
	while (*dest != -1) dest++;
	intextract (dest, src, len);
}

void intcatc (int *dest, int t)
{
	while (*dest != -1) dest++;
	*dest++ = t;
	*dest = -1;
}

void fintprint (FILE *f, int *s)
{
	while (*s != -1)
		fprintf (f, "%s ", expand (*s++));
}

/* escape double quotes */
char *escape_q_string (char *str, int len)
{
	int i, slen;
	char *ret;

	for (i = 0, slen = 4; i < len; i++, slen++)
		if (str [i] == '"')
			++slen;

	ret = (char*) malloc (slen);

	ret [0] = '"';
	for (i = 0, slen = 1; i < len; i++) {
		if (str [i] == '"') ret [slen++] = '\\';
		ret [slen++] = str [i];
	}
	ret [slen++] = '"';
	ret [slen] = 0;

	return ret;
}

/* load an entire text file into a string literal */
char *loadtext (char *file)
{
	struct load_file L;
	ctor_load_file_ (&L, file);
	if (!L.success) return 0;
	file = escape_c_string (L.data, L.len);
	dtor_load_file_ (&L);
	return file;
}

/* load a file with mmap */
/* How did this got here ??? bootstrap?? */

void ctor_load_file_(struct load_file *const this, char *file)
{
	this->data = 0;
	this->success = 0;
	this->fd = -1;
	struct stat statbuf;

	if (stat(file, &statbuf) == -1)
		return;
	if ((this->len = statbuf.st_size) == -1)
		return;
	if ((this->fd = open(file, O_RDONLY)) == -1)
		return;
#ifdef	SYS_HAS_MMAP
	this->data = (char *) mmap(0, this->len, PROT_READ, MAP_PRIVATE, this->fd, 0);
	this->success = this->data != MAP_FAILED;
#else
	this->data = (char*) malloc(this->len);
	this->success = read(this->fd, this->data, this->len) == this->len;
	close(this->fd);
#endif
}

void dtor_load_file_(struct load_file *const this)
{
#ifdef	SYS_HAS_MMAP
	if (this->success)
		munmap(this->data, this->len);
	if (this->fd != -1)
		close(this->fd);
#else
	free(this->data);
#endif
}

/* integer trees in C */
intnode *intfind (intnode *n, int key)
{
	int cbit;

	if (!n) return NULL;

	for (cbit = 1;; cbit <<= 1)
		if (n->key == key) return n;
		else if ((cbit & key))
			if (n->less) n = n->less;
			else return NULL;
		else
			if (n->more) n = n->more;
			else return NULL;
	return NULL;
}

void intadd (intnode **r, int key, union ival v)
{
	int cbit;
	intnode *n, *m = (intnode*) malloc (sizeof * m);
	m->less = m->more = NULL;
	m->key = key;
	m->v = v;

	if (!(n = *r)) {
		*r = m;
		return;
	}

	for (cbit = 1;; cbit <<= 1)
		if ((cbit & key))
			if (n->less) n = n->less;
			else {
				n->less = m;
				break;
			}
		else
			if (n->more) n = n->more;
			else {
				n->more = m;
				break;
			}
}

void intremove (intnode **root, intnode *i)
{
	unsigned int isroot, bt = 0;
	unsigned int key = i->key;
	intnode *n = *root;

	if (!((isroot = n == i))) {
		for (bt = 1; bt; bt *= 2)
			if (key & bt)	// avoid braces like hell
				if (n->less != i) n = n->less;
				else break;
			else		// yes but why?
				if (n->more != i) n = n->more;
				else break;
        }
	if (!i->less && !i->more)
		if (isroot) *root = 0;
		else
			if (key & bt) n->less = 0;
			else n->more = 0;
	else {
		intnode *r = i, *rp = 0;
		while (r->more || r->less) {
			rp = r;
			r = (r->more) ? r->more : r->less;
		}
		if (isroot) *root = r;
		else
			if (key & bt) n->less = r;
			else n->more = r;
		if (rp->more == r) rp->more = 0;
		else rp->less = 0;
		r->more = i->more;
		r->less = i->less;
	}
}
