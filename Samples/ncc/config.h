#define NCC_VERSION "1.4 -- [*lwc version*]"
#define GNU_VIOLATIONS
#define LABEL_VALUES
#define OUTPUT_EXT ".nccout"
#define NOGNU_MACROS "/usr/include/nognu"
#define NCC_INFILE_KEY "ncc-key"
#define FAKE_VARIABLE_ARRAYS
#define NCC_ISOC99
//#define PARSE_ARRAY_INITIALIZERS

// If you have linux with /dev/shm mounted
// try this one -- performance boost
//#define PREPROCESSOR_OUTPUT "/dev/shm/NCC.i"
#define PREPROCESSOR_OUTPUT "NCC.i"

#ifdef __GNUC__		// ------- gcc near 3.2 ------
unsigned int __builtin_bswap32(unsigned int);
#define __uint64_t __UINT64_TYPE__
__uint64_t __builtin_bswap64(__uint64_t);
#define COMPILER "gcc"
#define INTERN_memcpy RESERVED___builtin_memcpy
#define INTERN_alloca RESERVED___builtin_alloca
#define INTERN_strncmp RESERVED___builtin_strncmp
#define INTERN_strncasecmp RESERVED_strncasecmp
#define INTERN_bswap32 RESERVED_bswap32
#define CASE_RANGERS
#define HAVE_GNUC_LOCAL_LABELS
#define HAVE_GNUC_ATTR_NORETURN
#ifdef alloca
#undef alloca
#endif
#define alloca __builtin_alloca
#else			// ------ generic ------
#include <alloca.h>
#define COMPILER "generic"
#define INTERN_memcpy RESERVED_memcpy
#define INTERN_alloca RESERVED_alloca
#define INTERN_strncmp RESERVED_strncmp
#define INTERN_strncasecmp RESERVED_strncasecmp
#define INTERN_bswap32 RESERVED_bswap32
#undef HAVE_GNUC_LOCAL_LABELS
#undef HAVE_GNUC_ATTR_NORETURN
#endif
