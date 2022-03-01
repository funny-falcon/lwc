/***************************************************************************

	Array implementation.

This file provides the template for a generic array for objects
of type 'X'. The array allows us to:

	insert(X,i)
	append(X)
	remove(i)

elements can be accessed with:

	X &operator [] (int);
	int len;

If 'len' is exceeded in [], the program will naturally crash!
Enable DEBUG_ARRAY for check.

The array can be dynamically expanding or of fixed size.
The options are:

	- array_static:		fixed size array -- no allocations
	- array_f:		dynamic array of linear expansion
	- array_e:		dynamic array of exponential expansion

USAGE: look at the bottom of this file for sample implementation/testing
***************************************************************************/

	
extern "stdlib.h" {
#include <stdlib.h>
}
extern "string.h" {
#include <string.h>
}
#define DEBUG_ARRAY
#ifdef DEBUG_ARRAY
extern "assert.h" {
#include <assert.h>
}
#endif

template class array
{
	int len;

	ctor (array);

	X &operator [] (int i)	{ 
#ifdef DEBUG_ARRAY
	assert (i < len);
#endif
		return &v [i]; }
	void append (X);
	void append (array);
	void insert (X, int);
	void insert (array, int);
	void remove (int);
	void operator += (array A)	{ append (A); }
};

array.ctor (array A)
{
	ctor ();
	append (A);
}

void array.append (X x)
{
	if (max == len) expand ();
	v [len++] = x;
}

void array.append (array A)
{
	for (int i = 0; i < A.len; i++)
		append (A [i]);
}

void array.insert (X x, int p)
{
	int i;
	X *v = v;

	if (max == len) expand ();
	for (i = len++; i > p; i--)
		v [i] = v [i - 1];
	v [p] = x;
}

void array.insert (array A, int p)
{
	for (int i = 0; i < A.len; i++)
		insert (A [i], p++);
}

void array.remove (int p)
{
	int i;
	X *v = v;

	for (i = len--; p < i; p++)
		v [i] = v [i + 1];
}

// ********* Dynamic v *************
template class array_dyn : array {
	X *v;
	int max;
	void expand ();
	ctor (int);
	dtor ();
};

void array_dyn.expand ()
{
	v = (X*) realloc (v, sizeof * v * new_max ());
}


array_dyn.ctor (int i)
{
	v = (X*) malloc (sizeof * v * i);
	max = i;
	len = 0;
}


array_dyn.dtor ()
{
	if (v) free (v);
}

// ********** static v **********
template class array_static : array {
	const max 10;
	X v [max];
	ctor ()			{ len = 0; }
	void expand ()		{}
};

// ********************************

template class array_f : array_dyn {
	const increment 8;
	int new_max ();
	ctor () { v = 0; len = max = 0; }
};

int array_f.new_max ()
{
	return max += increment;
}

template class array_e : array_dyn {
	int new_max ();
	ctor () { v = 0; max = 4; expand (); len = 0; }
};

int array_e.new_max ()
{
	return max *= 2;
}

//************************ USAGE *************************
#if 0

typedef specialize array_e {
	typedef int X;
} array_int_expn;

typedef specialize array_f {
	const increment 16;
	typedef int X;
} array_int_fix16;

typedef specialize array_static {
	const max 123;
	typedef int X;
} array_int_static123;

int main ()
{
	array_int_expn A;
	array_int_fix16 B;
	array_int_static123 C;

	A.append (1);
}
#endif
