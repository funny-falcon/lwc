
#define SYS_HAS_MMAP
#define COLOR_IN_COMMENTS
#define COMPILER_GCC
#define CASE_RANGERS
//#define DO_CPP

#if 0
#if defined __GNUC__ && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))
#define EHDEFAULT true
#else
#define EHDEFAULT false
#endif
#else
#define EHDEFAULT false
#endif
