#include "global.h"

#ifdef DEBUG

FILE *logstream;

struct debugflag_s debugflag;

#define DEBUGDIR "/tmp/LWCD_"

void enable_debugs ()
{
	logstream = stderr;

#define TEST(x)	access (DEBUGDIR #x, F_OK) == 0;
//#define TEST(x)	1;
#define ENABLE(x) debugflag.x = TEST(x)
#define ON(x) debugflag.x = 1;

	/* general purpose. testing new features */
	ENABLE (GENERAL);

	/* report stages progress */
	ENABLE (PROGRESS);

	/* debug information on the expression rewritting */
	ENABLE (PEXPR);

	/* enable this if you're absolutely sure you really
	   know exactly what you're doing */
	ENABLE (PEXPR_EXTREME);

	/* report which function is being compiled */
	ENABLE (FUNCPROGRESS);

	/* indented output code but not indented debug msgs */
	ENABLE (OUTPUT_INDENTED);

	/* show all the program tokens after template expansion
	   and bogus() */
	ENABLE (SHOWPROG);

	/* report all declarations */
	ENABLE (DCL_TRACE);

	/* enable declared typedefs, useful for tracing libc headers */
	ENABLE (TDEF_TRACE);

	/* virtualtable info data */
	ENABLE (VIRTUALTABLES);

	/* because this is complicated */
	ENABLE (VIRTUALBASE);

	/* this is in virtual table combination in virtual inheritance */
	ENABLE (VIRTUALCOMBINE);

	/* auto function */
	ENABLE (AUTOF);

	/* errors in expressions terminate compilation */
	ENABLE (EXPR_ERRORS_FATAL);

	/* parse/expr errors cause an intentional segmentation
	   violation. This way we can use gdb to backtrace
	   and see which specific part of the program got
	   us to the parse error  */
	ENABLE (PARSE_ERRORS_SEGFAULT);

	/* do not print the generated output program.
	   that's useful if we just want to see the other
	   debugs above */
	ENABLE (NOSTDOUT);

	/* put the preprocessed source in a file (preproc.i) */
	ENABLE (SHOWCPP);

	/* preprocessor debugging */
	ENABLE (CPP);

	/* debug regular expression generation */
	ENABLE (REGEXP_DEBUG)
}

/* How does this work:
   If, for example, a file `/tmp/LWCD_PEXPR' exists, the program will
   enable the PEXPR flag and print debugging information which relies
   on this flag.   */

#endif
