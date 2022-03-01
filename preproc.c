/*

*/
#include "global.h"
#include "SYS.h"

static void RUN (char *outfile, char **argv)
{
#if 0
	int i;
	fprintf (stderr, "Running: ");
	for (i = 0; argv [i]; i++)
		fprintf (stderr, "%s ", argv [i]);
	fprintf (stderr, "\n");
#endif

	int pid = fork ();
	if (pid == 0) {
		if (outfile) if(!freopen (outfile, "w", stdout))
			exit (127);
		execvp (argv [0], argv);
		exit (127);
	}
	int status;
	waitpid (pid, &status, 0);
	if (WEXITSTATUS (status) != 0) {
		fprintf (stderr, "Preprocessor failed..\n");
		exit (1);
	}
}

static bool issource (char *f)
{
	char *e = f + strlen (f) - 1;
	if (*(e-1) == '.')
		return *e == 'c' || *e == 'h' || *e == 'C' || *e == 'i';
	if (*(e-2) == '.')
		return *(e-1) == 'c' && *e == '+';
	return *(e-3) == '.' && (*(e-2) == 'c' || *(e-2) == 'C');
}

const static char banner [] =
"Lightweight C++ preprocessor Version "LWC_VERSION" [compiled "__DATE__", adapted to: "COMPILER"]\n"
"Developed by Stelios Xanthakis and Carl Rosmann\n"
"Internet: http://students.ceid.upatras.gr/~sxanth/lwc/\n"
"This program is *Freeware*.  Save the trees\n\n";

char main_file [256];
bool sys_cpp =
#ifdef DO_CPP
 0
#else
 1
#endif
;

void preproc (int argc, char **argv)
{
	char **cppopt = (char**) alloca ((argc + 5) * sizeof (char*));
	int ncppopt, i;

#if 0
	/* The -gcc option seems to have been obsoleted and cpp reports
	   that it doesn't recognize it.  Still, it makes a difference */
	cppopt [0] = "cpp";
	cppopt [1] = "-D__LWC__";
#ifdef	__GNUC__
	cppopt [2] = "-x";
	cppopt [3] = "c";
	cppopt [4] = "-C";
	cppopt [5] = "-gcc";
	ncppopt = 6;
#else
	ncppopt = 2;
#endif
#else
	cppopt [0] = BASECC;
	cppopt [1] = "-D__LWC__";
	cppopt [2] = "-E";
	cppopt [3] = "-C";
	cppopt [4] = "-x";
	cppopt [5] = "c";
	ncppopt = 6;
#endif


	for (i = 0; i < argc; i++)
		if (issource (cppopt [ncppopt++] = argv [i]))
			if (current_file) fatal ("multiple files?");
			else current_file = strdup (argv [i]);
		else if (!strcmp (argv [i], "-D__REENTRANT"))
			Reentrant = true;
		else if (!strcmp (argv [i], "-sys_cpp"))
			sys_cpp = true;
	cppopt [ncppopt] = NULL;

	if (!current_file) {
		fputs (banner, stderr);
		exit (1);
	}

	if (sys_cpp) {
		snprintf (main_file, sizeof(main_file), "%s%i", PREPROCFILE, getpid ());
		RUN (main_file, cppopt);
	} else {
#ifdef	DO_CPP
		strncpy (main_file, current_file, sizeof(main_file));
		setup_cpp (argc, argv);
#endif
	}
}
