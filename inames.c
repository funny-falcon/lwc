#include "global.h"

#define ANONYMOUS_POST	"AnOnYmOuS"
#define OVERLOAD_POST	"OvErlOaD"
#define MEMBER_POST	""
#define TDEF_POST	"TyPeDeF"
#define UNNAMED_POST	"ObJeCt"
#define INHERIT_POST	"InHeRiTeD"
#define INHSTOR_POST	"iNhErItEd"
#define DOWNCAST_POST	"DoWnCaSt"
#define UPCAST_POST	"UpCaSt"
#define ARGV_PRE	"__ArGv"
#define VIRTUAL_POST	"virtual"
#define VIRTUALI_POST	"ViRtUaLTaBlE"
#define VTABLEREC_POST	"ViRtUaLTaBlE_StRuCt"
#define VIRTFUNC_POST	"ViRtUaLLity"
#define VVAR_POST	"ViRtUaLVar"
#define RTTI_POST	"DcAsTRttI"
#define INCTOR_POST	"__ICoNsTRuCTion"
#define INDTOR_POST	"__IDeStRuCtIoN"
#define INVDTOR_POST	"__vIDeStRuCtIoN"
#define VTDER_POST	"DeRRiVe"
#define NANONU_POST	"anonNeSt"
#define NONULL_POST	"NoNuLL"
#define IREGEXP_PRE	"rEgExP"
#define TYPEID_POST	"TyPeId"
#define ARRDTOR_POST	"aRrDtOr"
#define OBJCTOR		"ObJFiLe_ctor"
#define INITF_POST	"InItObJ"

#define NAME_TRUNCATED	"_TrNC"

#define INTERN_IDENT1	"lwcUniQUe"
#define INTERN_IDENT2	"lwcUniQUe2"
#define INTERN_IDENT3	"lwcUniQUe3"

int max_symbol_len = 512;

static char *name_not_too_long (char *n)
{
	char tmp [1024];
	int i, j;

	if (strlen (n) <= max_symbol_len) return n;
	i = j = 0;
	for (;;) {
		if (!(tmp [i++] = n [j++])) break;
		if (!n [j++]) break;
	}
	tmp [i] = 0;
	return name_not_too_long (strcpy (n, strcat (tmp, NAME_TRUNCATED)));
}

Token name_anon_regexp ()
{
static	int i;
	char tmp [128];
	sprintf (tmp, IREGEXP_PRE"%i", i++);
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_local_typedef (Token c, Token n)
{
	char tmp [512];
	sprintf (tmp, TDEF_POST"_%s_"MAGIC_DIGIT"%s", expand (n), expand (c));
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_anon_union (int n, Token r)
{
	char tmp [40];
	sprintf (tmp, "%s_"NANONU_POST"%i", expand (r), n);
	return new_symbol (strdup (tmp));
}

Token name_anonymous_struct ()
{
static	int i;
	char tmp [32];
	sprintf (tmp, "_struct%i_" ANONYMOUS_POST, i++);
	return new_symbol (strdup (tmp));
}

Token name_anonymous_union ()
{
static	int i;
	char tmp [32];
	sprintf (tmp, "_union%i_" ANONYMOUS_POST, i++);
	return new_symbol (strdup (tmp));
}

Token name_anonymous_enum ()
{
static	int i;
	char tmp [32];
	sprintf (tmp, "_enum%i_" ANONYMOUS_POST, i++);
	return new_symbol (strdup (tmp));
}

Token name_overload_fun (Token n, char *astr)
{
	char tmp [512];
	sprintf (tmp, "%s_"OVERLOAD_POST"_%s", expand (n), astr);
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_member_function (recID r, Token n)
{
	Token c = name_of_struct (r);
static	intnode *mfn;
	int bdi = c + 10000 * n;
	intnode *N;

	if ((N = intfind (mfn, bdi)))
		return N->v.i;

	char tmp [512];
	sprintf (tmp, "%s_"MAGIC_DIGIT"%s_"MEMBER_POST, expand (c), expand (n));
	c = new_symbol (strdup (name_not_too_long (tmp)));
	union ival u = { .i = c };
	intadd (&mfn, bdi, u);
	return c;
}

Token name_internal_object ()
{
static	int i;
	char tmp [32];
	sprintf (tmp, "tmp_%i"UNNAMED_POST, i++);
	return new_symbol (strdup (tmp));
}

Token name_uniq_var (int i)
{
static	Token cache [32];
	if (i < 32 && cache [i])
		return cache [i];
	char tmp [32];
	sprintf (tmp, "UnIqE_%i", i);
	return i < 32 ? cache [i] = new_symbol (strdup (tmp)) : new_symbol (strdup (tmp));
}

Token internal_identifier1 ()
{
static	Token ret = -1;
	if (ret != -1) return ret;
	if (!(ret = Lookup_Symbol (INTERN_IDENT1)))
		ret = new_symbol (INTERN_IDENT1);
	return ret;
}

Token internal_identifier2 ()
{
static	Token ret = -1;
	if (ret != -1) return ret;
	if (!(ret = Lookup_Symbol (INTERN_IDENT2)))
		ret = new_symbol (INTERN_IDENT2);
	return ret;
}

Token internal_identifier3 ()
{
static	Token ret = -1;
	if (ret != -1) return ret;
	if (!(ret = Lookup_Symbol (INTERN_IDENT3)))
		ret = new_symbol (INTERN_IDENT3);
	return ret;
}

Token internal_identifiern (int n)
{
static	Token ns [160];	// cache first 16
	char tmp [32];
	if (n < 160 && ns [n]) return ns [n];
	sprintf (tmp, ARGV_PRE"%i", n);
	if (n < 160)
		return ns [n] = new_symbol (strdup (tmp));
	return new_symbol (strdup (tmp));
}

Token name_inherited (Token p)
{
static	intnode *mfn;
	intnode *N;

	if ((N = intfind (mfn, p)))
		return N->v.i;

	char tmp [512];
	sprintf (tmp, "%s_"INHERIT_POST, expand (p));
	Token c = new_symbol (strdup (name_not_too_long (tmp)));
	union ival u = { .i = c };
	intadd (&mfn, p, u);
	return c;
}

Token name_typeid_var (Token o)
{
	char tmp [512];
	sprintf (tmp, "%s_"TYPEID_POST, expand (o));
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_glob_static_local (Token o)
{
	char tmp [512];
	sprintf (tmp, "%s_%s_sTaTiC", expand (in_function), expand (o));
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_unwind_var (Token o)
{
	char tmp [512];
	sprintf (tmp, "%s_UnWiNdER", expand (o));
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_arrdto_var (Token o)
{
	char tmp [512];
	sprintf (tmp, "%s_aRRdToR", expand (o));
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_downcast (Token b, Token d)
{
	char tmp [512];
	sprintf (tmp, "%s_%s_"DOWNCAST_POST, expand (b), expand (d));
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_downcast_safe (Token b, Token d)
{
	char tmp [512];
	sprintf (tmp, "%s_%s_"DOWNCAST_POST NONULL_POST, expand (b), expand (d));
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_upcast_safe (Token b, Token d)
{
	char tmp [512];
	sprintf (tmp, "%s_%s_"UPCAST_POST NONULL_POST, expand (b), expand (d));
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_virtual_slot (recID r, Token f, typeID t)
{
	char ts [256];
	char tmp [512];

	type_string (ts, t);
	sprintf (tmp, "%s_%s%s_"VIRTUAL_POST, expand (f), expand (name_of_struct (r)), ts);
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_virtual_variable (recID r, Token m)
{
	char tmp [512];
	sprintf (tmp, "%s_%s_"VVAR_POST, expand (m), expand (name_of_struct (r)));
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_instance (recID ra, recID rc)
{
	char tmp [512];
	sprintf (tmp, "%s_%s_"VIRTUALI_POST, expand (name_of_struct (ra)), expand (name_of_struct (rc)));
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_virtual_table (recID r)
{
	char tmp [512];
	sprintf (tmp, "%s_"VTABLEREC_POST, expand (name_of_struct (r)));
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_arrdtor (recID r)
{
	char tmp [512];
	sprintf (tmp, "%s_"ARRDTOR_POST, expand (name_of_struct (r)));
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_virtual_inner (recID r, recID r2, Token f, typeID t)
{
	char ts [256];
	char tmp [512];

	type_string (ts, t);
	sprintf (tmp, "%s%s_%s%s_"VIRTFUNC_POST, expand (name_of_struct (r)),
			 expand (name_of_struct (r2)), expand (f), ts);
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_name_enumerate (Token n, int i)
{
	char *tmp = (char*) alloca (strlen (expand (n)) + 5);
	sprintf (tmp, "%s%i", expand (n), i);
	return new_symbol (strdup (tmp));
}

Token name_rtti_slot (recID r1, Token t2)
{
	char tmp [512];
	sprintf (tmp, "%s_%s_"RTTI_POST, expand (name_of_struct (r1)), expand (t2));
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_derrive_memb (recID r)
{
	char tmp [512];
	sprintf (tmp, "%s_"VTDER_POST, expand (name_of_struct (r)));
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_intern_ctor (recID r)
{
	char tmp [512];
	sprintf (tmp, "%s_"INCTOR_POST, expand (name_of_struct (r)));
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_init_func (recID r)
{
	char tmp [512];
	sprintf (tmp, "%s_"INITF_POST, expand (name_of_struct (r)));
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_intern_vdtor (recID r)
{
	char tmp [512];
	sprintf (tmp, "%s_"INVDTOR_POST, expand (name_of_struct (r)));
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_intern_dtor (recID r)
{
	char tmp [512];
	sprintf (tmp, "%s_"INDTOR_POST, expand (name_of_struct (r)));
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_storage_inherit (recID r)
{
	char tmp [512];
	sprintf (tmp, "%s_"INHSTOR_POST, expand (name_of_struct (r)));
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_global_ctor (int i)
{
	char tmp [512];
	sprintf (tmp, OBJCTOR"%i_", i);
	return new_symbol (strdup (name_not_too_long (tmp)));
}

Token name_derrive_union;

Token tokstrcat (Token t, char *s)
{
	char tmp [512];
	sprintf (tmp, "%s%s\n", expand (t), s);
	return new_symbol (strdup (tmp));
}

Token toktokcat (Token t1, Token t2)
{
	char tmp [512];
	sprintf (tmp, "%s%s\n", expand (t1), expand (t2));
	return new_symbol (strdup (tmp));
}

Token name_ebn_func (Token e)
{
	char tmp [512];
	sprintf (tmp, "%s_By_NaMe", expand (e));
	return new_symbol (strdup (tmp));
}

Token name_longbreak ()
{
static	int i;
	char tmp [512];
	sprintf (tmp, "LoNgBrEaK%i", i++);
	return new_symbol (strdup (tmp));
}

Token name_longcontinue ()
{
static	int i;
	char tmp [512];
	sprintf (tmp, "LoNgCoNtInUe%i", i++);
	return new_symbol (strdup (tmp));
}
