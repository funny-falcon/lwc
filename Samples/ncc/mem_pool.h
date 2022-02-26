//--------------------------------------------------------------------------
//
//	arrays of automatically increasing size.
//
//	earray<X>	for few elements, reallocation + memcpy every 64
//	mem_pool<X>	for up to a LOT of elements (64*128*256)
//
//-------------------------------------------------------------------------


#ifndef mem_pool_INCLUDED
#define mem_pool_INCLUDED
#include <string.h>
#include "global.h"
#include "dbstree.h"

template class mem_pool
{
const	TSEG 64;
const	NSEG 128;
const	PSEG 64;
	X ***p;
	int sp;
   public:
	mem_pool ();
	int	alloc ();
inline	X	&operator [] (int);
	void	pop ();
	int	nr ()	{ return sp; }
	void	copy (X**);
	void	destroy ();
};


#define I1A(x) (x / (PSEG*NSEG))
#define I2A(x) ((x % (PSEG*NSEG)) / PSEG)
#define I3A(x) ((x % (PSEG*NSEG)) % PSEG)

mem_pool.mem_pool ()
{
	sp = 0;
	p = (X***) malloc (TSEG * sizeof (X**));
}

int mem_pool.alloc ()
{
	if (sp % (PSEG*NSEG) == 0)
		p [I1A (sp)] = (X**) malloc (NSEG * sizeof (X*));
	if (sp % PSEG == 0) {
		p [I1A (sp)] [I2A (sp)] = (X*) malloc (PSEG * sizeof (X));
		memset (p [I1A (sp)] [I2A (sp)], 0, PSEG * sizeof (X));
	}
	return sp++;
}

X &mem_pool.operator [] (int i)
{
	return &p [I1A (i)] [I2A (i)] [I3A (i)];
}

void mem_pool.pop ()
{
	if (sp) sp--;
}

void mem_pool.copy (X **di)
{
	int i;
	X *d;
	if (sp == 0) return;
	if (*di == NULL) *di = (X*) malloc (sp * sizeof (X));
	d = *di;
	for (i = 0; i + PSEG < sp; i += PSEG)
		memcpy (&d [i], p [I1A(i)][I2A(i)], PSEG * sizeof (X));
	memcpy (&d [i], p [I1A(i)][I2A(i)], (sp - i) * sizeof (X));
}

void mem_pool.destroy ()
{
	int i;
	for (i = 0; i < sp; i += PSEG)
		free (p [I1A (i)] [I2A (i)]);
	for (i = 0; i < sp; i += PSEG*NSEG)
		free (p [I1A (i)]);
	free (p);
}

template class earray
{
#define	SSEG 64
	int ms;
	X *Realloc (int);
   public:
	int nr;
	X *x;
	earray ();
inline	int alloc ();
	void freeze ();
	void erase ();
};

earray.earray ()
{
	ms = nr = 0;
	x = NULL;
}

X *earray.Realloc (int s)
{
	return x = (X*) realloc (x, s * sizeof (X));
}

int earray.alloc ()
{
	if (ms == nr)
		x = Realloc (ms += SSEG);
	return nr++;
}

void earray.freeze ()
{
	if (nr) x = Realloc (ms = nr);
}

void earray.erase ()
{
	if (x) free (x);
	x = NULL;
	ms = nr = 0;
}


// *******************************************************
// We're gonna need those
// *******************************************************

//earray_template (earray_cf, cfile_i)
//earray_template (earray_types, type)
//earray_template (earray_typeID, typeID*)
//mem_pool_template (mem_pool_cl, clines_i)
//mem_pool_template (mem_pool_int, int)
//mem_pool_template (mem_pool_char, signed char)
//mem_pool_template (mem_pool_short, signed short int)
//mem_pool_template (mem_pool_long, signed long int)
//mem_pool_template (mem_pool_ulong, unsigned long int)
//mem_pool_template (mem_pool_float, double)

struct memb_li { Symbol s; memb_li *next; };
enum REGION { GLOBAL, CCODE, FUNCTIONAL, RECORD_S, RECORD_U };
struct region {
	RegionPtr parent;
	REGION kind;
	int aligned_top, nn;
	char bits, incomplete;
	memb_li *first, *last;
	int add_object (typeID);
	int add_field (typeID, typeID*);
	void add_member (Symbol);
};

//mem_pool_template (mem_pool_region, region)

struct cfunc { Symbol name; NormPtr args, body, ends; };
//earray_template (earray_cfunc, cfunc)

struct fptrassign { Symbol s1, s2, m1, m2; bool sptr; };
//earray_template (earray_fptrassign, fptrassign)

#endif
