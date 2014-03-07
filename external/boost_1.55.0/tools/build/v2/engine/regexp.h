/*
 * Definitions etc. for regexp(3) routines.
 *
 * Caveat:  this is V8 regexp(3) [actually, a reimplementation thereof],
 * not the System V one.
 */
#ifndef REGEXP_DWA20011023_H
#define REGEXP_DWA20011023_H

#define NSUBEXP  10
typedef struct regexp {
    char const * startp[ NSUBEXP ];
    char const * endp[ NSUBEXP ];
    char regstart;      /* Internal use only. */
    char reganch;       /* Internal use only. */
    char * regmust;     /* Internal use only. */
    int regmlen;        /* Internal use only. */
    char program[ 1 ];  /* Unwarranted chumminess with compiler. */
} regexp;


regexp * regcomp( char const * exp );
int regexec( regexp * prog, char const * string );
void regerror( char const * s );


/*
 * The first byte of the regexp internal "program" is actually this magic
 * number; the start node begins in the second byte.
 */
#define MAGIC  0234

#endif

