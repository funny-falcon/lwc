
//
// use cos(a)^2 for lighting of matte
// surfaces instead of cos(a).
//
#define MATTE_SQR

//
// HAVE_SSE activates some SIMD extensions.
// gcc uses SSE instructions instead of 387fpu
// instructions but only scalar ones (for example
// 'addss' and not 'addps'). With this flag we
// use packed ones for vector operations.
//
// [Unfortunatelly]: For some reason it is slower.
// I have studied the output assembly and there are
// fewer commands (3 scalar muls == 1 packed mul),
// but the program slows down. Dunno. Maybe for
// pentiums or future gcc versions it will be faster.
//
//#define HAVE_SSE

