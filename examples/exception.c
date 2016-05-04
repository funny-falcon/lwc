/* lightweight c++ 2.0 */

/*[01;37m************* system headers *************[0m*/

#include <setjmp.h>

#include <unwind.h>

/*[01;37m************* global scope *************[0m*/

/*[01;37m************* Structures *************[0m*/
struct longjmp_StaCk
{
  jmp_buf *x;
  struct longjmp_StaCk *y;
  void *X;
  void *i;
};
/*[01;37m************* Virtua Table declarations *************[0m*/

/*[01;37m************ Function Prototypes  ***********[0m*/
int printf (const char *__ArGv0, ...);
void recur (int x);
void *malloc (unsigned int);
void free ();
/*[01;37m************* Global variables *************[0m*/
struct longjmp_StaCk longjmp_iNiTObjFaKe, *__restrict longjmp_StaCkTop = &longjmp_iNiTObjFaKe;	/*[01;37m*** gcc unwind internals ***[0m */
static _Unwind_Reason_Code
ThRoW_sToP ()
{
  return _URC_NO_REASON;
}

void __lwc_unwind (void *) __attribute__ ((noreturn, noinline));
void
__lwc_unwind (void *i)
{
  struct _Unwind_Exception *exc = malloc (sizeof *exc);
  exc->exception_class = 0;
  exc->exception_cleanup = 0;
  longjmp_StaCkTop->X = i;
  longjmp_StaCkTop->i = exc;
  _Unwind_ForcedUnwind (exc, ThRoW_sToP, 0);
/* did you forget -fexceptions ? */ *(int *) 0 = 0;
  __lwc_unwind (i);
}

void
__lwc_landingpad (int *i)
{
  if (!*i)
    longjmp (*longjmp_StaCkTop->x, 1);
}

/*[01;37m************* regular expressions *************[0m*/

/*[01;37m************ Internal Functions  ************[0m*/

/*[01;37m******** auto-function instantiations ********[0m*/

/*[01;37m******** Program function definitions ********[0m*/
void
recur (int x)
{
  printf ("Entered %i\n", x);
  if (x == 10)
    __lwc_unwind (13);
  recur (x + 1);
}

int
main ()
{
  {
    struct longjmp_StaCk longjmp_CoNtExT;
    jmp_buf lwcUniQUe2;
    longjmp_CoNtExT.x = &lwcUniQUe2;
    longjmp_CoNtExT.y = longjmp_StaCkTop;
    longjmp_StaCkTop = &longjmp_CoNtExT;
    longjmp_CoNtExT.X = 0;
    if (!(setjmp (lwcUniQUe2)))
      {
	int lwcUniQUe3 __attribute__ ((cleanup (__lwc_landingpad))) = 0;
	{
	  printf ("try\n");
	  recur (0);
	  printf ("This is never reached.");
	} lwcUniQUe3 = 1;
	longjmp_StaCkTop = longjmp_CoNtExT.y;
      }
    else
      {
	free (longjmp_CoNtExT.i);
	longjmp_StaCkTop = longjmp_CoNtExT.y;
	printf ("caught exception\n");
      }
  }
  {
    struct longjmp_StaCk longjmp_CoNtExT;
    int retcode;
    jmp_buf lwcUniQUe2;
    longjmp_CoNtExT.x = &lwcUniQUe2;
    longjmp_CoNtExT.y = longjmp_StaCkTop;
    longjmp_StaCkTop = &longjmp_CoNtExT;
    longjmp_CoNtExT.X = 0;
    if (!(setjmp (lwcUniQUe2)))
      {
	int lwcUniQUe3 __attribute__ ((cleanup (__lwc_landingpad))) = 0;
	recur (0);
	lwcUniQUe3 = 1;
	longjmp_StaCkTop = longjmp_CoNtExT.y;
      }
    else
      {
	retcode = longjmp_CoNtExT.X;
	free (longjmp_CoNtExT.i);
	longjmp_StaCkTop = longjmp_CoNtExT.y;
	printf ("caught exception retcode=%i\n", retcode);
      }
  }
  {
    struct longjmp_StaCk longjmp_CoNtExT;
    jmp_buf lwcUniQUe2;
    longjmp_CoNtExT.x = &lwcUniQUe2;
    longjmp_CoNtExT.y = longjmp_StaCkTop;
    longjmp_StaCkTop = &longjmp_CoNtExT;
    longjmp_CoNtExT.X = 0;
    if (!(setjmp (lwcUniQUe2)))
      {
	int lwcUniQUe3 __attribute__ ((cleanup (__lwc_landingpad))) = 0;
	recur (0);
	lwcUniQUe3 = 1;
	longjmp_StaCkTop = longjmp_CoNtExT.y;
      }
    else
      {
	free (longjmp_CoNtExT.i);
	longjmp_StaCkTop = longjmp_CoNtExT.y;
      }
  }
  return 0;
}

/*[01;37m*************** Virtual tables ***************[0m*/
