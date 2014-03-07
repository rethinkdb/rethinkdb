/* A Bison parser, made by GNU Bison 2.4.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006,
   2009, 2010 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 96 "jamgram.y"

#include "jam.h"

#include "lists.h"
#include "parse.h"
#include "scan.h"
#include "compile.h"
#include "object.h"
#include "rules.h"

# define YYMAXDEPTH 10000	/* for OSF and other less endowed yaccs */

# define F0 -1
# define P0 (PARSE *)0
# define S0 (OBJECT *)0

# define pappend( l,r )    	parse_make( PARSE_APPEND,l,r,P0,S0,S0,0 )
# define peval( c,l,r )	parse_make( PARSE_EVAL,l,r,P0,S0,S0,c )
# define pfor( s,l,r,x )    	parse_make( PARSE_FOREACH,l,r,P0,s,S0,x )
# define pif( l,r,t )	  	parse_make( PARSE_IF,l,r,t,S0,S0,0 )
# define pincl( l )       	parse_make( PARSE_INCLUDE,l,P0,P0,S0,S0,0 )
# define plist( s )	  	parse_make( PARSE_LIST,P0,P0,P0,s,S0,0 )
# define plocal( l,r,t )  	parse_make( PARSE_LOCAL,l,r,t,S0,S0,0 )
# define pmodule( l,r )	  	parse_make( PARSE_MODULE,l,r,P0,S0,S0,0 )
# define pclass( l,r )	  	parse_make( PARSE_CLASS,l,r,P0,S0,S0,0 )
# define pnull()	  	parse_make( PARSE_NULL,P0,P0,P0,S0,S0,0 )
# define pon( l,r )	  	parse_make( PARSE_ON,l,r,P0,S0,S0,0 )
# define prule( s,p )     	parse_make( PARSE_RULE,p,P0,P0,s,S0,0 )
# define prules( l,r )	  	parse_make( PARSE_RULES,l,r,P0,S0,S0,0 )
# define pset( l,r,a )          parse_make( PARSE_SET,l,r,P0,S0,S0,a )
# define pset1( l,r,t,a )	parse_make( PARSE_SETTINGS,l,r,t,S0,S0,a )
# define psetc( s,p,a,l )     	parse_make( PARSE_SETCOMP,p,a,P0,s,S0,l )
# define psete( s,l,s1,f ) 	parse_make( PARSE_SETEXEC,l,P0,P0,s,s1,f )
# define pswitch( l,r )   	parse_make( PARSE_SWITCH,l,r,P0,S0,S0,0 )
# define pwhile( l,r )   	parse_make( PARSE_WHILE,l,r,P0,S0,S0,0 )

# define pnode( l,r )    	parse_make( F0,l,r,P0,S0,S0,0 )
# define psnode( s,l )     	parse_make( F0,l,P0,P0,s,S0,0 )



/* Line 189 of yacc.c  */
#line 114 "y.tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     _BANG_t = 258,
     _BANG_EQUALS_t = 259,
     _AMPER_t = 260,
     _AMPERAMPER_t = 261,
     _LPAREN_t = 262,
     _RPAREN_t = 263,
     _PLUS_EQUALS_t = 264,
     _COLON_t = 265,
     _SEMIC_t = 266,
     _LANGLE_t = 267,
     _LANGLE_EQUALS_t = 268,
     _EQUALS_t = 269,
     _RANGLE_t = 270,
     _RANGLE_EQUALS_t = 271,
     _QUESTION_EQUALS_t = 272,
     _LBRACKET_t = 273,
     _RBRACKET_t = 274,
     ACTIONS_t = 275,
     BIND_t = 276,
     CASE_t = 277,
     CLASS_t = 278,
     DEFAULT_t = 279,
     ELSE_t = 280,
     EXISTING_t = 281,
     FOR_t = 282,
     IF_t = 283,
     IGNORE_t = 284,
     IN_t = 285,
     INCLUDE_t = 286,
     LOCAL_t = 287,
     MODULE_t = 288,
     ON_t = 289,
     PIECEMEAL_t = 290,
     QUIETLY_t = 291,
     RETURN_t = 292,
     RULE_t = 293,
     SWITCH_t = 294,
     TOGETHER_t = 295,
     UPDATED_t = 296,
     WHILE_t = 297,
     _LBRACE_t = 298,
     _BAR_t = 299,
     _BARBAR_t = 300,
     _RBRACE_t = 301,
     ARG = 302,
     STRING = 303
   };
#endif
/* Tokens.  */
#define _BANG_t 258
#define _BANG_EQUALS_t 259
#define _AMPER_t 260
#define _AMPERAMPER_t 261
#define _LPAREN_t 262
#define _RPAREN_t 263
#define _PLUS_EQUALS_t 264
#define _COLON_t 265
#define _SEMIC_t 266
#define _LANGLE_t 267
#define _LANGLE_EQUALS_t 268
#define _EQUALS_t 269
#define _RANGLE_t 270
#define _RANGLE_EQUALS_t 271
#define _QUESTION_EQUALS_t 272
#define _LBRACKET_t 273
#define _RBRACKET_t 274
#define ACTIONS_t 275
#define BIND_t 276
#define CASE_t 277
#define CLASS_t 278
#define DEFAULT_t 279
#define ELSE_t 280
#define EXISTING_t 281
#define FOR_t 282
#define IF_t 283
#define IGNORE_t 284
#define IN_t 285
#define INCLUDE_t 286
#define LOCAL_t 287
#define MODULE_t 288
#define ON_t 289
#define PIECEMEAL_t 290
#define QUIETLY_t 291
#define RETURN_t 292
#define RULE_t 293
#define SWITCH_t 294
#define TOGETHER_t 295
#define UPDATED_t 296
#define WHILE_t 297
#define _LBRACE_t 298
#define _BAR_t 299
#define _BARBAR_t 300
#define _RBRACE_t 301
#define ARG 302
#define STRING 303




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 252 "y.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  43
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   243

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  49
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  24
/* YYNRULES -- Number of rules.  */
#define YYNRULES  75
/* YYNRULES -- Number of states.  */
#define YYNSTATES  159

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   303

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     4,     6,     8,    10,    12,    15,    21,
      22,    25,    27,    31,    32,    34,    35,    39,    43,    47,
      52,    59,    63,    72,    78,    84,    90,    96,   102,   110,
     116,   120,   121,   122,   132,   134,   136,   138,   141,   143,
     147,   151,   155,   159,   163,   167,   171,   175,   179,   183,
     187,   190,   194,   195,   198,   203,   205,   209,   211,   212,
     215,   217,   218,   223,   226,   231,   236,   237,   240,   242,
     244,   246,   248,   250,   252,   253
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      50,     0,    -1,    -1,    52,    -1,    53,    -1,    52,    -1,
      57,    -1,    57,    52,    -1,    32,    65,    54,    11,    51,
      -1,    -1,    14,    65,    -1,    53,    -1,     7,    64,     8,
      -1,    -1,    32,    -1,    -1,    43,    51,    46,    -1,    31,
      65,    11,    -1,    47,    64,    11,    -1,    67,    60,    65,
      11,    -1,    67,    34,    65,    60,    65,    11,    -1,    37,
      65,    11,    -1,    27,    56,    47,    30,    65,    43,    51,
      46,    -1,    39,    65,    43,    62,    46,    -1,    28,    61,
      43,    51,    46,    -1,    33,    65,    43,    51,    46,    -1,
      23,    64,    43,    51,    46,    -1,    42,    61,    43,    51,
      46,    -1,    28,    61,    43,    51,    46,    25,    57,    -1,
      56,    38,    47,    55,    57,    -1,    34,    67,    57,    -1,
      -1,    -1,    20,    70,    47,    72,    43,    58,    48,    59,
      46,    -1,    14,    -1,     9,    -1,    17,    -1,    24,    14,
      -1,    67,    -1,    61,    14,    61,    -1,    61,     4,    61,
      -1,    61,    12,    61,    -1,    61,    13,    61,    -1,    61,
      15,    61,    -1,    61,    16,    61,    -1,    61,     5,    61,
      -1,    61,     6,    61,    -1,    61,    44,    61,    -1,    61,
      45,    61,    -1,    67,    30,    65,    -1,     3,    61,    -1,
       7,    61,     8,    -1,    -1,    63,    62,    -1,    22,    47,
      10,    51,    -1,    65,    -1,    65,    10,    64,    -1,    66,
      -1,    -1,    66,    67,    -1,    47,    -1,    -1,    18,    68,
      69,    19,    -1,    47,    64,    -1,    34,    67,    47,    64,
      -1,    34,    67,    37,    65,    -1,    -1,    70,    71,    -1,
      41,    -1,    40,    -1,    29,    -1,    36,    -1,    35,    -1,
      26,    -1,    -1,    21,    65,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   139,   139,   141,   152,   154,   158,   160,   162,   167,
     170,   172,   176,   179,   182,   185,   188,   190,   192,   194,
     196,   198,   200,   202,   204,   206,   208,   210,   212,   214,
     216,   219,   221,   218,   230,   232,   234,   236,   243,   245,
     247,   249,   251,   253,   255,   257,   259,   261,   263,   265,
     267,   269,   281,   282,   286,   295,   297,   307,   312,   313,
     317,   319,   319,   328,   330,   332,   343,   344,   348,   350,
     352,   354,   356,   358,   368,   369
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "_BANG_t", "_BANG_EQUALS_t", "_AMPER_t",
  "_AMPERAMPER_t", "_LPAREN_t", "_RPAREN_t", "_PLUS_EQUALS_t", "_COLON_t",
  "_SEMIC_t", "_LANGLE_t", "_LANGLE_EQUALS_t", "_EQUALS_t", "_RANGLE_t",
  "_RANGLE_EQUALS_t", "_QUESTION_EQUALS_t", "_LBRACKET_t", "_RBRACKET_t",
  "ACTIONS_t", "BIND_t", "CASE_t", "CLASS_t", "DEFAULT_t", "ELSE_t",
  "EXISTING_t", "FOR_t", "IF_t", "IGNORE_t", "IN_t", "INCLUDE_t",
  "LOCAL_t", "MODULE_t", "ON_t", "PIECEMEAL_t", "QUIETLY_t", "RETURN_t",
  "RULE_t", "SWITCH_t", "TOGETHER_t", "UPDATED_t", "WHILE_t", "_LBRACE_t",
  "_BAR_t", "_BARBAR_t", "_RBRACE_t", "ARG", "STRING", "$accept", "run",
  "block", "rules", "null", "assign_list_opt", "arglist_opt", "local_opt",
  "rule", "$@1", "$@2", "assign", "expr", "cases", "case", "lol", "list",
  "listp", "arg", "$@3", "func", "eflags", "eflag", "bindlist", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    49,    50,    50,    51,    51,    52,    52,    52,    53,
      54,    54,    55,    55,    56,    56,    57,    57,    57,    57,
      57,    57,    57,    57,    57,    57,    57,    57,    57,    57,
      57,    58,    59,    57,    60,    60,    60,    60,    61,    61,
      61,    61,    61,    61,    61,    61,    61,    61,    61,    61,
      61,    61,    62,    62,    63,    64,    64,    65,    66,    66,
      67,    68,    67,    69,    69,    69,    70,    70,    71,    71,
      71,    71,    71,    71,    72,    72
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     1,     1,     1,     1,     2,     5,     0,
       2,     1,     3,     0,     1,     0,     3,     3,     3,     4,
       6,     3,     8,     5,     5,     5,     5,     5,     7,     5,
       3,     0,     0,     9,     1,     1,     1,     2,     1,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       2,     3,     0,     2,     4,     1,     3,     1,     0,     2,
       1,     0,     4,     2,     4,     4,     0,     2,     1,     1,
       1,     1,     1,     1,     0,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,    61,    66,    58,    15,     0,    58,    58,    58,     0,
      58,    58,     0,     9,    60,     0,     3,     0,     6,     0,
       0,     0,     0,    55,    57,    14,     0,     0,     0,    60,
       0,    38,     0,     9,     0,    15,     0,     0,     0,     0,
       5,     4,     0,     1,     0,     7,    35,    34,    36,     0,
      58,    58,     0,    58,     0,    73,    70,    72,    71,    69,
      68,    74,    67,     9,    58,    59,     0,    50,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     9,     0,     0,
      58,    17,    58,    11,     0,     9,    30,    21,    52,     9,
      16,    18,    13,    37,     0,     0,     0,    63,    62,    58,
       0,     0,    56,    58,    51,    40,    45,    46,    41,    42,
      39,    43,    44,     0,    47,    48,    49,    10,     9,     0,
       0,     0,    52,     0,    58,    15,    58,    19,    58,    58,
      75,    31,    26,     0,    24,     8,    25,     0,    23,    53,
      27,     0,    29,     0,    65,    64,     0,     9,    15,     9,
      12,    20,    32,     0,    28,    54,     0,    22,    33
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    15,    39,    40,    41,    84,   125,    17,    18,   146,
     156,    51,    30,   121,   122,    22,    23,    24,    31,    20,
      54,    21,    62,   100
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -48
static const yytype_int16 yypact[] =
{
     170,   -48,   -48,   -48,   -12,     7,   -48,   -17,   -48,    -3,
     -48,   -48,     7,   170,     1,    22,   -48,    -9,   170,    19,
      -2,    79,    -6,    29,    -3,   -48,     2,     7,     7,   -48,
     138,    20,    44,    45,    18,   196,    51,    26,   151,    24,
     -48,   -48,    62,   -48,    27,   -48,   -48,   -48,   -48,    61,
     -48,   -48,    -3,   -48,    67,   -48,   -48,   -48,   -48,   -48,
     -48,    58,   -48,   170,   -48,   -48,    50,   -48,    52,     7,
       7,     7,     7,     7,     7,     7,     7,   170,     7,     7,
     -48,   -48,   -48,   -48,    70,   170,   -48,   -48,    87,   170,
     -48,   -48,    94,   -48,    17,    99,   -20,   -48,   -48,   -48,
      69,    71,   -48,   -48,   -48,    91,   156,   156,   -48,   -48,
      91,   -48,   -48,    77,    78,    78,   -48,   -48,   170,    81,
      66,    82,    87,    95,   -48,   196,   -48,   -48,   -48,   -48,
     -48,   -48,   -48,    97,   112,   -48,   -48,   135,   -48,   -48,
     -48,   150,   -48,   148,   -48,   -48,    98,   170,   196,   170,
     -48,   -48,   -48,   115,   -48,   -48,   116,   -48,   -48
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -48,   -48,   -47,     5,   140,   -48,   -48,   171,   -27,   -48,
     -48,    80,    60,    54,   -48,   -13,    -4,   -48,     0,   -48,
     -48,   -48,   -48,   -48
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -59
static const yytype_int16 yytable[] =
{
      19,    42,    32,    33,    34,    16,    36,    37,    86,    35,
      27,   -58,   -58,    19,    28,     1,   101,   128,    19,   -58,
      25,   -14,    43,    45,    65,     1,    46,   129,    46,    44,
     113,    47,    52,    47,    48,    19,    48,    63,   119,    64,
      97,    49,   123,    49,    29,    53,    94,    95,   -58,    66,
      80,   102,    96,    50,    29,    81,    69,    70,    71,    82,
     104,    85,    87,    19,    72,    73,    74,    75,    76,    88,
      90,   135,    38,    91,    92,    93,   116,    19,   117,    99,
     103,   118,    69,    70,    71,    19,    98,    67,    68,    19,
      72,    73,    74,    75,    76,   130,    78,    79,   142,   133,
     153,   124,   155,    72,    73,    55,    75,    76,    56,   120,
     127,   141,   131,   137,    57,    58,   145,   132,    19,    59,
      60,   154,   143,   134,   144,    19,    61,   136,   138,   105,
     106,   107,   108,   109,   110,   111,   112,   148,   114,   115,
     147,   140,    69,    70,    71,   149,   152,    19,    19,    19,
      72,    73,    74,    75,    76,    69,    70,    71,   150,   151,
      69,   157,   158,    72,    73,    74,    75,    76,    72,    73,
      74,    75,    76,    83,   126,    26,   139,     0,     0,     0,
       0,    77,    78,    79,     0,     0,     0,     0,     1,     0,
       2,     0,     0,     3,    89,    78,    79,     4,     5,     0,
       0,     6,     7,     8,     9,     0,     0,    10,   -15,    11,
       0,     0,    12,    13,     1,     0,     2,    14,     0,     3,
       0,     0,     0,     4,     5,     0,     0,     6,    25,     8,
       9,     0,     0,    10,     0,    11,     0,     0,    12,    13,
       0,     0,     0,    14
};

static const yytype_int16 yycheck[] =
{
       0,    14,     6,     7,     8,     0,    10,    11,    35,     9,
       3,    10,    11,    13,     7,    18,    63,    37,    18,    18,
      32,    38,     0,    18,    24,    18,     9,    47,     9,    38,
      77,    14,    34,    14,    17,    35,    17,    43,    85,    10,
      53,    24,    89,    24,    47,    47,    50,    51,    47,    47,
      30,    64,    52,    34,    47,    11,     4,     5,     6,    14,
       8,    43,    11,    63,    12,    13,    14,    15,    16,    43,
      46,   118,    12,    11,    47,    14,    80,    77,    82,    21,
      30,    11,     4,     5,     6,    85,    19,    27,    28,    89,
      12,    13,    14,    15,    16,    99,    44,    45,   125,   103,
     147,     7,   149,    12,    13,    26,    15,    16,    29,    22,
      11,   124,    43,    47,    35,    36,   129,    46,   118,    40,
      41,   148,   126,    46,   128,   125,    47,    46,    46,    69,
      70,    71,    72,    73,    74,    75,    76,    25,    78,    79,
      43,    46,     4,     5,     6,    10,    48,   147,   148,   149,
      12,    13,    14,    15,    16,     4,     5,     6,     8,    11,
       4,    46,    46,    12,    13,    14,    15,    16,    12,    13,
      14,    15,    16,    33,    94,     4,   122,    -1,    -1,    -1,
      -1,    43,    44,    45,    -1,    -1,    -1,    -1,    18,    -1,
      20,    -1,    -1,    23,    43,    44,    45,    27,    28,    -1,
      -1,    31,    32,    33,    34,    -1,    -1,    37,    38,    39,
      -1,    -1,    42,    43,    18,    -1,    20,    47,    -1,    23,
      -1,    -1,    -1,    27,    28,    -1,    -1,    31,    32,    33,
      34,    -1,    -1,    37,    -1,    39,    -1,    -1,    42,    43,
      -1,    -1,    -1,    47
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    18,    20,    23,    27,    28,    31,    32,    33,    34,
      37,    39,    42,    43,    47,    50,    52,    56,    57,    67,
      68,    70,    64,    65,    66,    32,    56,     3,     7,    47,
      61,    67,    65,    65,    65,    67,    65,    65,    61,    51,
      52,    53,    64,     0,    38,    52,     9,    14,    17,    24,
      34,    60,    34,    47,    69,    26,    29,    35,    36,    40,
      41,    47,    71,    43,    10,    67,    47,    61,    61,     4,
       5,     6,    12,    13,    14,    15,    16,    43,    44,    45,
      30,    11,    14,    53,    54,    43,    57,    11,    43,    43,
      46,    11,    47,    14,    65,    65,    67,    64,    19,    21,
      72,    51,    64,    30,     8,    61,    61,    61,    61,    61,
      61,    61,    61,    51,    61,    61,    65,    65,    11,    51,
      22,    62,    63,    51,     7,    55,    60,    11,    37,    47,
      65,    43,    46,    65,    46,    51,    46,    47,    46,    62,
      46,    64,    57,    65,    65,    64,    58,    43,    25,    10,
       8,    11,    48,    51,    57,    51,    59,    46,    46
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:

/* Line 1464 of yacc.c  */
#line 142 "jamgram.y"
    { parse_save( (yyvsp[(1) - (1)]).parse ); }
    break;

  case 4:

/* Line 1464 of yacc.c  */
#line 153 "jamgram.y"
    { (yyval).parse = (yyvsp[(1) - (1)]).parse; }
    break;

  case 5:

/* Line 1464 of yacc.c  */
#line 155 "jamgram.y"
    { (yyval).parse = (yyvsp[(1) - (1)]).parse; }
    break;

  case 6:

/* Line 1464 of yacc.c  */
#line 159 "jamgram.y"
    { (yyval).parse = (yyvsp[(1) - (1)]).parse; }
    break;

  case 7:

/* Line 1464 of yacc.c  */
#line 161 "jamgram.y"
    { (yyval).parse = prules( (yyvsp[(1) - (2)]).parse, (yyvsp[(2) - (2)]).parse ); }
    break;

  case 8:

/* Line 1464 of yacc.c  */
#line 163 "jamgram.y"
    { (yyval).parse = plocal( (yyvsp[(2) - (5)]).parse, (yyvsp[(3) - (5)]).parse, (yyvsp[(5) - (5)]).parse ); }
    break;

  case 9:

/* Line 1464 of yacc.c  */
#line 167 "jamgram.y"
    { (yyval).parse = pnull(); }
    break;

  case 10:

/* Line 1464 of yacc.c  */
#line 171 "jamgram.y"
    { (yyval).parse = (yyvsp[(2) - (2)]).parse; (yyval).number = ASSIGN_SET; }
    break;

  case 11:

/* Line 1464 of yacc.c  */
#line 173 "jamgram.y"
    { (yyval).parse = (yyvsp[(1) - (1)]).parse; (yyval).number = ASSIGN_APPEND; }
    break;

  case 12:

/* Line 1464 of yacc.c  */
#line 177 "jamgram.y"
    { (yyval).parse = (yyvsp[(2) - (3)]).parse; }
    break;

  case 13:

/* Line 1464 of yacc.c  */
#line 179 "jamgram.y"
    { (yyval).parse = P0; }
    break;

  case 14:

/* Line 1464 of yacc.c  */
#line 183 "jamgram.y"
    { (yyval).number = 1; }
    break;

  case 15:

/* Line 1464 of yacc.c  */
#line 185 "jamgram.y"
    { (yyval).number = 0; }
    break;

  case 16:

/* Line 1464 of yacc.c  */
#line 189 "jamgram.y"
    { (yyval).parse = (yyvsp[(2) - (3)]).parse; }
    break;

  case 17:

/* Line 1464 of yacc.c  */
#line 191 "jamgram.y"
    { (yyval).parse = pincl( (yyvsp[(2) - (3)]).parse ); }
    break;

  case 18:

/* Line 1464 of yacc.c  */
#line 193 "jamgram.y"
    { (yyval).parse = prule( (yyvsp[(1) - (3)]).string, (yyvsp[(2) - (3)]).parse ); }
    break;

  case 19:

/* Line 1464 of yacc.c  */
#line 195 "jamgram.y"
    { (yyval).parse = pset( (yyvsp[(1) - (4)]).parse, (yyvsp[(3) - (4)]).parse, (yyvsp[(2) - (4)]).number ); }
    break;

  case 20:

/* Line 1464 of yacc.c  */
#line 197 "jamgram.y"
    { (yyval).parse = pset1( (yyvsp[(1) - (6)]).parse, (yyvsp[(3) - (6)]).parse, (yyvsp[(5) - (6)]).parse, (yyvsp[(4) - (6)]).number ); }
    break;

  case 21:

/* Line 1464 of yacc.c  */
#line 199 "jamgram.y"
    { (yyval).parse = (yyvsp[(2) - (3)]).parse; }
    break;

  case 22:

/* Line 1464 of yacc.c  */
#line 201 "jamgram.y"
    { (yyval).parse = pfor( (yyvsp[(3) - (8)]).string, (yyvsp[(5) - (8)]).parse, (yyvsp[(7) - (8)]).parse, (yyvsp[(2) - (8)]).number ); }
    break;

  case 23:

/* Line 1464 of yacc.c  */
#line 203 "jamgram.y"
    { (yyval).parse = pswitch( (yyvsp[(2) - (5)]).parse, (yyvsp[(4) - (5)]).parse ); }
    break;

  case 24:

/* Line 1464 of yacc.c  */
#line 205 "jamgram.y"
    { (yyval).parse = pif( (yyvsp[(2) - (5)]).parse, (yyvsp[(4) - (5)]).parse, pnull() ); }
    break;

  case 25:

/* Line 1464 of yacc.c  */
#line 207 "jamgram.y"
    { (yyval).parse = pmodule( (yyvsp[(2) - (5)]).parse, (yyvsp[(4) - (5)]).parse ); }
    break;

  case 26:

/* Line 1464 of yacc.c  */
#line 209 "jamgram.y"
    { (yyval).parse = pclass( (yyvsp[(2) - (5)]).parse, (yyvsp[(4) - (5)]).parse ); }
    break;

  case 27:

/* Line 1464 of yacc.c  */
#line 211 "jamgram.y"
    { (yyval).parse = pwhile( (yyvsp[(2) - (5)]).parse, (yyvsp[(4) - (5)]).parse ); }
    break;

  case 28:

/* Line 1464 of yacc.c  */
#line 213 "jamgram.y"
    { (yyval).parse = pif( (yyvsp[(2) - (7)]).parse, (yyvsp[(4) - (7)]).parse, (yyvsp[(7) - (7)]).parse ); }
    break;

  case 29:

/* Line 1464 of yacc.c  */
#line 215 "jamgram.y"
    { (yyval).parse = psetc( (yyvsp[(3) - (5)]).string, (yyvsp[(5) - (5)]).parse, (yyvsp[(4) - (5)]).parse, (yyvsp[(1) - (5)]).number ); }
    break;

  case 30:

/* Line 1464 of yacc.c  */
#line 217 "jamgram.y"
    { (yyval).parse = pon( (yyvsp[(2) - (3)]).parse, (yyvsp[(3) - (3)]).parse ); }
    break;

  case 31:

/* Line 1464 of yacc.c  */
#line 219 "jamgram.y"
    { yymode( SCAN_STRING ); }
    break;

  case 32:

/* Line 1464 of yacc.c  */
#line 221 "jamgram.y"
    { yymode( SCAN_NORMAL ); }
    break;

  case 33:

/* Line 1464 of yacc.c  */
#line 223 "jamgram.y"
    { (yyval).parse = psete( (yyvsp[(3) - (9)]).string,(yyvsp[(4) - (9)]).parse,(yyvsp[(7) - (9)]).string,(yyvsp[(2) - (9)]).number ); }
    break;

  case 34:

/* Line 1464 of yacc.c  */
#line 231 "jamgram.y"
    { (yyval).number = ASSIGN_SET; }
    break;

  case 35:

/* Line 1464 of yacc.c  */
#line 233 "jamgram.y"
    { (yyval).number = ASSIGN_APPEND; }
    break;

  case 36:

/* Line 1464 of yacc.c  */
#line 235 "jamgram.y"
    { (yyval).number = ASSIGN_DEFAULT; }
    break;

  case 37:

/* Line 1464 of yacc.c  */
#line 237 "jamgram.y"
    { (yyval).number = ASSIGN_DEFAULT; }
    break;

  case 38:

/* Line 1464 of yacc.c  */
#line 244 "jamgram.y"
    { (yyval).parse = peval( EXPR_EXISTS, (yyvsp[(1) - (1)]).parse, pnull() ); }
    break;

  case 39:

/* Line 1464 of yacc.c  */
#line 246 "jamgram.y"
    { (yyval).parse = peval( EXPR_EQUALS, (yyvsp[(1) - (3)]).parse, (yyvsp[(3) - (3)]).parse ); }
    break;

  case 40:

/* Line 1464 of yacc.c  */
#line 248 "jamgram.y"
    { (yyval).parse = peval( EXPR_NOTEQ, (yyvsp[(1) - (3)]).parse, (yyvsp[(3) - (3)]).parse ); }
    break;

  case 41:

/* Line 1464 of yacc.c  */
#line 250 "jamgram.y"
    { (yyval).parse = peval( EXPR_LESS, (yyvsp[(1) - (3)]).parse, (yyvsp[(3) - (3)]).parse ); }
    break;

  case 42:

/* Line 1464 of yacc.c  */
#line 252 "jamgram.y"
    { (yyval).parse = peval( EXPR_LESSEQ, (yyvsp[(1) - (3)]).parse, (yyvsp[(3) - (3)]).parse ); }
    break;

  case 43:

/* Line 1464 of yacc.c  */
#line 254 "jamgram.y"
    { (yyval).parse = peval( EXPR_MORE, (yyvsp[(1) - (3)]).parse, (yyvsp[(3) - (3)]).parse ); }
    break;

  case 44:

/* Line 1464 of yacc.c  */
#line 256 "jamgram.y"
    { (yyval).parse = peval( EXPR_MOREEQ, (yyvsp[(1) - (3)]).parse, (yyvsp[(3) - (3)]).parse ); }
    break;

  case 45:

/* Line 1464 of yacc.c  */
#line 258 "jamgram.y"
    { (yyval).parse = peval( EXPR_AND, (yyvsp[(1) - (3)]).parse, (yyvsp[(3) - (3)]).parse ); }
    break;

  case 46:

/* Line 1464 of yacc.c  */
#line 260 "jamgram.y"
    { (yyval).parse = peval( EXPR_AND, (yyvsp[(1) - (3)]).parse, (yyvsp[(3) - (3)]).parse ); }
    break;

  case 47:

/* Line 1464 of yacc.c  */
#line 262 "jamgram.y"
    { (yyval).parse = peval( EXPR_OR, (yyvsp[(1) - (3)]).parse, (yyvsp[(3) - (3)]).parse ); }
    break;

  case 48:

/* Line 1464 of yacc.c  */
#line 264 "jamgram.y"
    { (yyval).parse = peval( EXPR_OR, (yyvsp[(1) - (3)]).parse, (yyvsp[(3) - (3)]).parse ); }
    break;

  case 49:

/* Line 1464 of yacc.c  */
#line 266 "jamgram.y"
    { (yyval).parse = peval( EXPR_IN, (yyvsp[(1) - (3)]).parse, (yyvsp[(3) - (3)]).parse ); }
    break;

  case 50:

/* Line 1464 of yacc.c  */
#line 268 "jamgram.y"
    { (yyval).parse = peval( EXPR_NOT, (yyvsp[(2) - (2)]).parse, pnull() ); }
    break;

  case 51:

/* Line 1464 of yacc.c  */
#line 270 "jamgram.y"
    { (yyval).parse = (yyvsp[(2) - (3)]).parse; }
    break;

  case 52:

/* Line 1464 of yacc.c  */
#line 281 "jamgram.y"
    { (yyval).parse = P0; }
    break;

  case 53:

/* Line 1464 of yacc.c  */
#line 283 "jamgram.y"
    { (yyval).parse = pnode( (yyvsp[(1) - (2)]).parse, (yyvsp[(2) - (2)]).parse ); }
    break;

  case 54:

/* Line 1464 of yacc.c  */
#line 287 "jamgram.y"
    { (yyval).parse = psnode( (yyvsp[(2) - (4)]).string, (yyvsp[(4) - (4)]).parse ); }
    break;

  case 55:

/* Line 1464 of yacc.c  */
#line 296 "jamgram.y"
    { (yyval).parse = pnode( P0, (yyvsp[(1) - (1)]).parse ); }
    break;

  case 56:

/* Line 1464 of yacc.c  */
#line 298 "jamgram.y"
    { (yyval).parse = pnode( (yyvsp[(3) - (3)]).parse, (yyvsp[(1) - (3)]).parse ); }
    break;

  case 57:

/* Line 1464 of yacc.c  */
#line 308 "jamgram.y"
    { (yyval).parse = (yyvsp[(1) - (1)]).parse; yymode( SCAN_NORMAL ); }
    break;

  case 58:

/* Line 1464 of yacc.c  */
#line 312 "jamgram.y"
    { (yyval).parse = pnull(); yymode( SCAN_PUNCT ); }
    break;

  case 59:

/* Line 1464 of yacc.c  */
#line 314 "jamgram.y"
    { (yyval).parse = pappend( (yyvsp[(1) - (2)]).parse, (yyvsp[(2) - (2)]).parse ); }
    break;

  case 60:

/* Line 1464 of yacc.c  */
#line 318 "jamgram.y"
    { (yyval).parse = plist( (yyvsp[(1) - (1)]).string ); }
    break;

  case 61:

/* Line 1464 of yacc.c  */
#line 319 "jamgram.y"
    { yymode( SCAN_NORMAL ); }
    break;

  case 62:

/* Line 1464 of yacc.c  */
#line 320 "jamgram.y"
    { (yyval).parse = (yyvsp[(3) - (4)]).parse; }
    break;

  case 63:

/* Line 1464 of yacc.c  */
#line 329 "jamgram.y"
    { (yyval).parse = prule( (yyvsp[(1) - (2)]).string, (yyvsp[(2) - (2)]).parse ); }
    break;

  case 64:

/* Line 1464 of yacc.c  */
#line 331 "jamgram.y"
    { (yyval).parse = pon( (yyvsp[(2) - (4)]).parse, prule( (yyvsp[(3) - (4)]).string, (yyvsp[(4) - (4)]).parse ) ); }
    break;

  case 65:

/* Line 1464 of yacc.c  */
#line 333 "jamgram.y"
    { (yyval).parse = pon( (yyvsp[(2) - (4)]).parse, (yyvsp[(4) - (4)]).parse ); }
    break;

  case 66:

/* Line 1464 of yacc.c  */
#line 343 "jamgram.y"
    { (yyval).number = 0; }
    break;

  case 67:

/* Line 1464 of yacc.c  */
#line 345 "jamgram.y"
    { (yyval).number = (yyvsp[(1) - (2)]).number | (yyvsp[(2) - (2)]).number; }
    break;

  case 68:

/* Line 1464 of yacc.c  */
#line 349 "jamgram.y"
    { (yyval).number = EXEC_UPDATED; }
    break;

  case 69:

/* Line 1464 of yacc.c  */
#line 351 "jamgram.y"
    { (yyval).number = EXEC_TOGETHER; }
    break;

  case 70:

/* Line 1464 of yacc.c  */
#line 353 "jamgram.y"
    { (yyval).number = EXEC_IGNORE; }
    break;

  case 71:

/* Line 1464 of yacc.c  */
#line 355 "jamgram.y"
    { (yyval).number = EXEC_QUIETLY; }
    break;

  case 72:

/* Line 1464 of yacc.c  */
#line 357 "jamgram.y"
    { (yyval).number = EXEC_PIECEMEAL; }
    break;

  case 73:

/* Line 1464 of yacc.c  */
#line 359 "jamgram.y"
    { (yyval).number = EXEC_EXISTING; }
    break;

  case 74:

/* Line 1464 of yacc.c  */
#line 368 "jamgram.y"
    { (yyval).parse = pnull(); }
    break;

  case 75:

/* Line 1464 of yacc.c  */
#line 370 "jamgram.y"
    { (yyval).parse = (yyvsp[(2) - (2)]).parse; }
    break;



/* Line 1464 of yacc.c  */
#line 2118 "y.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



