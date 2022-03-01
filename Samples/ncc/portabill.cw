//
// C wrappers for extremely hairy GNU-Libc stuff
// which lwc (nor any other sane non-gnu prog) can
// parse
//
#include <sys/types.h>
#include <sys/wait.h>

int nohairy_WEXITSTATUS (int status)
{
	return WEXITSTATUS (status);
}
