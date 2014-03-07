/* 
 * Copyright (c) 1993-1994 by Xerox Corporation.  All rights reserved.
 *
 * THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
 * OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.
 *
 * Permission is hereby granted to use or copy this program
 * for any purpose,  provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 */
/* Boehm, August 24, 1994 11:58 am PDT */
# include "gc.h"	/* For GC_INIT() only */
# include "cord.h"
# include <string.h>
# include <stdio.h>
# include <stdlib.h>
/* This is a very incomplete test of the cord package.  It knows about	*/
/* a few internals of the package (e.g. when C strings are returned)	*/
/* that real clients shouldn't rely on.					*/

# define ABORT(string) \
{ int x = 0; fprintf(stderr, "FAILED: %s\n", string); x = 1 / x; abort(); }

int count;

int test_fn(char c, void * client_data)
{
    if (client_data != (void *)13) ABORT("bad client data");
    if (count < 64*1024+1) {
        if ((count & 1) == 0) {
            if (c != 'b') ABORT("bad char");
        } else {
            if (c != 'a') ABORT("bad char");
        }
        count++;
        return(0);
    } else {
        if (c != 'c') ABORT("bad char");
        count++;
        return(1);
    }
}

char id_cord_fn(size_t i, void * client_data)
{
    return((char)i);
}

void test_basics()
{
    CORD x = CORD_from_char_star("ab");
    register int i;
    char c;
    CORD y;
    CORD_pos p;
    
    x = CORD_cat(x,x);
    if (!CORD_IS_STRING(x)) ABORT("short cord should usually be a string");
    if (strcmp(x, "abab") != 0) ABORT("bad CORD_cat result");
    
    for (i = 1; i < 16; i++) {
        x = CORD_cat(x,x);
    }
    x = CORD_cat(x,"c");
    if (CORD_len(x) != 128*1024+1) ABORT("bad length");
    
    count = 0;
    if (CORD_iter5(x, 64*1024-1, test_fn, CORD_NO_FN, (void *)13) == 0) {
        ABORT("CORD_iter5 failed");
    }
    if (count != 64*1024 + 2) ABORT("CORD_iter5 failed");
    
    count = 0;
    CORD_set_pos(p, x, 64*1024-1);
    while(CORD_pos_valid(p)) {
       	(void) test_fn(CORD_pos_fetch(p), (void *)13);
	CORD_next(p);
    }
    if (count != 64*1024 + 2) ABORT("Position based iteration failed");
    
    y = CORD_substr(x, 1023, 5);
    if (!CORD_IS_STRING(y)) ABORT("short cord should usually be a string");
    if (strcmp(y, "babab") != 0) ABORT("bad CORD_substr result");
    
    y = CORD_substr(x, 1024, 8);
    if (!CORD_IS_STRING(y)) ABORT("short cord should usually be a string");
    if (strcmp(y, "abababab") != 0) ABORT("bad CORD_substr result");
    
    y = CORD_substr(x, 128*1024-1, 8);
    if (!CORD_IS_STRING(y)) ABORT("short cord should usually be a string");
    if (strcmp(y, "bc") != 0) ABORT("bad CORD_substr result");
    
    x = CORD_balance(x);
    if (CORD_len(x) != 128*1024+1) ABORT("bad length");
    
    count = 0;
    if (CORD_iter5(x, 64*1024-1, test_fn, CORD_NO_FN, (void *)13) == 0) {
        ABORT("CORD_iter5 failed");
    }
    if (count != 64*1024 + 2) ABORT("CORD_iter5 failed");
    
    y = CORD_substr(x, 1023, 5);
    if (!CORD_IS_STRING(y)) ABORT("short cord should usually be a string");
    if (strcmp(y, "babab") != 0) ABORT("bad CORD_substr result");
    y = CORD_from_fn(id_cord_fn, 0, 13);
    i = 0;
    CORD_set_pos(p, y, i);
    while(CORD_pos_valid(p)) {
        c = CORD_pos_fetch(p);
       	if(c != i) ABORT("Traversal of function node failed");
	CORD_next(p); i++;
    }
    if (i != 13) ABORT("Bad apparent length for function node");
}

void test_extras()
{
#   if defined(__OS2__) || defined(__DJGPP__)
#	define FNAME1 "tmp1"
#	define FNAME2 "tmp2"
#   elif defined(AMIGA)
#	define FNAME1 "T:tmp1"
#	define FNAME2 "T:tmp2"
#   else
#	define FNAME1 "/tmp/cord_test"
#	define FNAME2 "/tmp/cord_test2"
#   endif
    register int i;
    CORD y = "abcdefghijklmnopqrstuvwxyz0123456789";
    CORD x = "{}";
    CORD w, z;
    FILE *f;
    FILE *f1a, *f1b, *f2;
    
    w = CORD_cat(CORD_cat(y,y),y);
    z = CORD_catn(3,y,y,y);
    if (CORD_cmp(w,z) != 0) ABORT("CORD_catn comparison wrong");
    for (i = 1; i < 100; i++) {
        x = CORD_cat(x, y);
    }
    z = CORD_balance(x);
    if (CORD_cmp(x,z) != 0) ABORT("balanced string comparison wrong");
    if (CORD_cmp(x,CORD_cat(z, CORD_nul(13))) >= 0) ABORT("comparison 2");
    if (CORD_cmp(CORD_cat(x, CORD_nul(13)), z) <= 0) ABORT("comparison 3");
    if (CORD_cmp(x,CORD_cat(z, "13")) >= 0) ABORT("comparison 4");
    if ((f = fopen(FNAME1, "w")) == 0) ABORT("open failed");
    if (CORD_put(z,f) == EOF) ABORT("CORD_put failed");
    if (fclose(f) == EOF) ABORT("fclose failed");
    w = CORD_from_file(f1a = fopen(FNAME1, "rb"));
    if (CORD_len(w) != CORD_len(z)) ABORT("file length wrong");
    if (CORD_cmp(w,z) != 0) ABORT("file comparison wrong");
    if (CORD_cmp(CORD_substr(w, 50*36+2, 36), y) != 0)
    	ABORT("file substr wrong");
    z = CORD_from_file_lazy(f1b = fopen(FNAME1, "rb"));
    if (CORD_cmp(w,z) != 0) ABORT("File conversions differ");
    if (CORD_chr(w, 0, '9') != 37) ABORT("CORD_chr failed 1");
    if (CORD_chr(w, 3, 'a') != 38) ABORT("CORD_chr failed 2");
    if (CORD_rchr(w, CORD_len(w) - 1, '}') != 1) ABORT("CORD_rchr failed");
    x = y;
    for (i = 1; i < 14; i++) {
        x = CORD_cat(x,x);
    }
    if ((f = fopen(FNAME2, "w")) == 0) ABORT("2nd open failed");
#   ifdef __DJGPP__
      /* FIXME: DJGPP workaround.  Why does this help? */
      if (fflush(f) != 0) ABORT("fflush failed");
#   endif
    if (CORD_put(x,f) == EOF) ABORT("CORD_put failed");
    if (fclose(f) == EOF) ABORT("fclose failed");
    w = CORD_from_file(f2 = fopen(FNAME2, "rb"));
    if (CORD_len(w) != CORD_len(x)) ABORT("file length wrong");
    if (CORD_cmp(w,x) != 0) ABORT("file comparison wrong");
    if (CORD_cmp(CORD_substr(w, 1000*36, 36), y) != 0)
    	ABORT("file substr wrong");
    if (strcmp(CORD_to_char_star(CORD_substr(w, 1000*36, 36)), y) != 0)
    	ABORT("char * file substr wrong");
    if (strcmp(CORD_substr(w, 1000*36, 2), "ab") != 0)
    	ABORT("short file substr wrong");
    if (CORD_str(x,1,"9a") != 35) ABORT("CORD_str failed 1");
    if (CORD_str(x,0,"9abcdefghijk") != 35) ABORT("CORD_str failed 2");
    if (CORD_str(x,0,"9abcdefghijx") != CORD_NOT_FOUND)
    	ABORT("CORD_str failed 3");
    if (CORD_str(x,0,"9>") != CORD_NOT_FOUND) ABORT("CORD_str failed 4");
    if (remove(FNAME1) != 0) {
    	/* On some systems, e.g. OS2, this may fail if f1 is still open. */
    	if ((fclose(f1a) == EOF) & (fclose(f1b) == EOF))
    		ABORT("fclose(f1) failed");
    	if (remove(FNAME1) != 0) ABORT("remove 1 failed");
    }
    if (remove(FNAME2) != 0) {
    	if (fclose(f2) == EOF) ABORT("fclose(f2) failed");
    	if (remove(FNAME2) != 0) ABORT("remove 2 failed");
    }
}

void test_printf()
{
    CORD result;
    char result2[200];
    long l;
    short s;
    CORD x;
    
    if (CORD_sprintf(&result, "%7.2f%ln", 3.14159F, &l) != 7)
    	ABORT("CORD_sprintf failed 1");
    if (CORD_cmp(result, "   3.14") != 0)ABORT("CORD_sprintf goofed 1");
    if (l != 7) ABORT("CORD_sprintf goofed 2");
    if (CORD_sprintf(&result, "%-7.2s%hn%c%s", "abcd", &s, 'x', "yz") != 10)
    	ABORT("CORD_sprintf failed 2");
    if (CORD_cmp(result, "ab     xyz") != 0)ABORT("CORD_sprintf goofed 3");
    if (s != 7) ABORT("CORD_sprintf goofed 4");
    x = "abcdefghij";
    x = CORD_cat(x,x);
    x = CORD_cat(x,x);
    x = CORD_cat(x,x);
    if (CORD_sprintf(&result, "->%-120.78r!\n", x) != 124)
    	ABORT("CORD_sprintf failed 3");
    (void) sprintf(result2, "->%-120.78s!\n", CORD_to_char_star(x));
    if (CORD_cmp(result, result2) != 0)ABORT("CORD_sprintf goofed 5");
}

int main()
{
#   ifdef THINK_C
        printf("cordtest:\n");
#   endif
    GC_INIT();
    test_basics();
    test_extras();
    test_printf();
    CORD_fprintf(stderr, "SUCCEEDED\n");
    return(0);
}
