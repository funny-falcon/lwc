//
// The CODE[] table may include the things below, or
// ascii characters for simple operators
//

#define THE_END		0		// actually it should be infty..

#define CALL_AGAIN	256
#define IDENT_DUMMY	257

// Intermediate reserved
#define FCONSTANT	258
#define CCONSTANT	259
#define CONSTANT	300
#define STRING		301
#define FORCEERROR	302

// Special reserved
#define CPP_CONCAT	302
#define CPP_DIRECTIVE	303
#define MEGATOKEN	304
#define ANONYMOUS	305

// C/C++ operators
#define DOTSTAR		350
#define ELLIPSIS	351
#define POINTSAT_STAR	352
#define POINTSAT	353
#define MINUSMINUS	354
#define PLUSPLUS	355
#define SCOPE		356
#define GEQCMP		357
#define LSH		358
#define OROR		359
#define ANDAND		360
#define EQCMP		361
#define NEQCMP		362
#define RSH		363
#define LEQCMP		364
// '='
#define ASSIGNA		367
#define ASSIGNS		368
#define ASSIGNM		369
#define ASSIGND		370
#define ASSIGNR		371
#define ASSIGNBA	372
#define ASSIGNBX	373
#define ASSIGNBO	374
#define ASSIGNRS	375
#define ASSIGNLS	376

// Indexed symbols
#define NSTRINGS	100000
#define NSYMBOLS	1000000
#define BASE		2000
#define STRINGBASE	BASE

// bases for multiplexed quantities
#define SYMBASE		(BASE + NSTRINGS)
#define NUMBASE 	(SYMBASE + NSYMBOLS)
#define INT8BASE	(NUMBASE)
#define INT16BASE	(INT8BASE + 1000000)
#define INT32BASE	(INT8BASE + 2000000)
#define UINT32BASE	(INT8BASE + 3000000)
#define INT64BASE	(INT8BASE + 4000000)
#define UINT64BASE	(INT8BASE + 5000000)
#define FLOATBASE	(INT8BASE + 6000000)
#define INUMBER		(INT8BASE + 7000000)

#define ISSTRING(x)	(x >= BASE && x < SYMBASE)
#define ISSYMBOL(x)	(x >= SYMBASE && x < NUMBASE)
#define ISNUMBER(x)	(x >= NUMBASE)

// Helper routines
#define ISOPERATOR(x)	(x <= '~' || (x >= DOTSTAR && x <= ASSIGNLS))
#define ISASSIGNMENT(x)	(x == '=' || (x >= ASSIGNA && x <= ASSIGNLS))
#define ISRESERVED(x)	(x >= RESERVED_auto && x <= RESERVED_END)
#define ISDCLFLAG(x)	(x >= RESERVED_auto && x <= RESERVED_volatile)
#define ISBASETYPE(x)	(x >= RESERVED_void && x <= RESERVED_bool)
#define ISSTRUCTSPC(x)	(x >= RESERVED_class && x <= RESERVED_union)
#define ISHBASETYPE(x)	(x >= RESERVED_long && x <= RESERVED_unsigned)
#define ISDCLSTRT(x)	(x >= RESERVED_long && x <= RESERVED_bool)
#define SYMBOLID(x)	(x - SYMBASE)

// C and C++ declaration flags
#define RESERVED_auto			700
#define RESERVED_const			701
#define RESERVED_extern			702
#define RESERVED_inline			703
#define RESERVED_register		704
#define RESERVED_static			705
#define RESERVED_typedef		706
#define RESERVED_volatile		707

// these can define a type
#define RESERVED_long			708
#define RESERVED_short			709
#define RESERVED_signed			710
#define RESERVED_unsigned		711

// C and C++ base types
#define RESERVED_void			720
#define RESERVED_char			721
#define RESERVED_int			722
#define RESERVED_float			723
#define RESERVED_double			724
#define RESERVED_bool			725

// C and C++ aggregate specifiers
#define RESERVED_class			731
#define RESERVED_struct			732
#define RESERVED_union			733

// C and C++ reserved words -- not including basic types
#define RESERVED_break			741
#define RESERVED_case			742
#define RESERVED_catch			743
#define RESERVED_const_cast		745
#define RESERVED_continue		746
#define RESERVED_default		747
#define RESERVED_delete			748
#define RESERVED_do			749
#define RESERVED_dynamic_cast		750
#define RESERVED_else			751
#define RESERVED_enum			752
#define RESERVED_explict		753
#define RESERVED_export			754
#define RESERVED_false			755	// -Dfalse=0
#define RESERVED_for			756
#define RESERVED_friend			757
#define RESERVED_goto			758
#define RESERVED_if			759
#define RESERVED_mutable		760
#define RESERVED_RESERVED		761
#define RESERVED_namespace		762
#define RESERVED_new			763
#define RESERVED_operator		764
#define RESERVED_private		765
#define RESERVED_protected		766
#define RESERVED_public			767
#define RESERVED_reinterpret_cast	768
#define RESERVED_return			769
#define RESERVED_sizeof			770
#define RESERVED_static_cast		771
#define RESERVED_switch			773
#define RESERVED_template		774
#define RESERVED_this			775
#define RESERVED_throw			776
#define RESERVED_true			777	// -Dtrue=1
#define RESERVED_try			778
#define RESERVED_typeid			779
#define RESERVED_typename		780
#define RESERVED_using			781
#define RESERVED_virtual		782
#define RESERVED_while			783
#define RESERVED___asm__		784
#ifdef GNU_VIOLATIONS
#define RESERVED___typeof__		800
#define RESERVED___label__		801
#endif
#define RESERVED_END			888
