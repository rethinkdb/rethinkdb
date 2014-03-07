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
 *
 * Author: Hans-J. Boehm (boehm@parc.xerox.com)
 */
/* Boehm, October 3, 1994 5:19 pm PDT */
# include "gc.h"
# include "cord.h"
# include <stdlib.h>
# include <stdio.h>
# include <string.h>

/* An implementation of the cord primitives.  These are the only 	*/
/* Functions that understand the representation.  We perform only	*/
/* minimal checks on arguments to these functions.  Out of bounds	*/
/* arguments to the iteration functions may result in client functions	*/
/* invoked on garbage data.  In most cases, client functions should be	*/
/* programmed defensively enough that this does not result in memory	*/
/* smashes.								*/ 

typedef void (* oom_fn)(void);

oom_fn CORD_oom_fn = (oom_fn) 0;

# define OUT_OF_MEMORY {  if (CORD_oom_fn != (oom_fn) 0) (*CORD_oom_fn)(); \
			  ABORT("Out of memory\n"); }
# define ABORT(msg) { fprintf(stderr, "%s\n", msg); abort(); }

typedef unsigned long word;

typedef union {
    struct Concatenation {
    	char null;
	char header;
	char depth;	/* concatenation nesting depth. */
	unsigned char left_len;
			/* Length of left child if it is sufficiently	*/
			/* short; 0 otherwise.				*/
#	    define MAX_LEFT_LEN 255
	word len;
	CORD left;	/* length(left) > 0	*/
	CORD right;	/* length(right) > 0	*/
    } concatenation;
    struct Function {
	char null;
	char header;
	char depth;	/* always 0	*/
	char left_len;	/* always 0	*/
	word len;
	CORD_fn fn;
	void * client_data;
    } function;
    struct Generic {
    	char null;
	char header;
	char depth;
	char left_len;
	word len;
    } generic;
    char string[1];
} CordRep;

# define CONCAT_HDR 1
	
# define FN_HDR 4
# define SUBSTR_HDR 6
	/* Substring nodes are a special case of function nodes.  	*/
	/* The client_data field is known to point to a substr_args	*/
	/* structure, and the function is either CORD_apply_access_fn 	*/
	/* or CORD_index_access_fn.					*/

/* The following may be applied only to function and concatenation nodes: */
#define IS_CONCATENATION(s)  (((CordRep *)s)->generic.header == CONCAT_HDR)

#define IS_FUNCTION(s)  ((((CordRep *)s)->generic.header & FN_HDR) != 0)

#define IS_SUBSTR(s) (((CordRep *)s)->generic.header == SUBSTR_HDR)

#define LEN(s) (((CordRep *)s) -> generic.len)
#define DEPTH(s) (((CordRep *)s) -> generic.depth)
#define GEN_LEN(s) (CORD_IS_STRING(s) ? strlen(s) : LEN(s))

#define LEFT_LEN(c) ((c) -> left_len != 0? \
				(c) -> left_len \
				: (CORD_IS_STRING((c) -> left) ? \
					(c) -> len - GEN_LEN((c) -> right) \
					: LEN((c) -> left)))

#define SHORT_LIMIT (sizeof(CordRep) - 1)
	/* Cords shorter than this are C strings */


/* Dump the internal representation of x to stdout, with initial 	*/
/* indentation level n.							*/
void CORD_dump_inner(CORD x, unsigned n)
{
    register size_t i;
    
    for (i = 0; i < (size_t)n; i++) {
        fputs("  ", stdout);
    }
    if (x == 0) {
      	fputs("NIL\n", stdout);
    } else if (CORD_IS_STRING(x)) {
        for (i = 0; i <= SHORT_LIMIT; i++) {
            if (x[i] == '\0') break;
            putchar(x[i]);
        }
        if (x[i] != '\0') fputs("...", stdout);
        putchar('\n');
    } else if (IS_CONCATENATION(x)) {
        register struct Concatenation * conc =
        			&(((CordRep *)x) -> concatenation);
        printf("Concatenation: %p (len: %d, depth: %d)\n",
               x, (int)(conc -> len), (int)(conc -> depth));
        CORD_dump_inner(conc -> left, n+1);
        CORD_dump_inner(conc -> right, n+1);
    } else /* function */{
        register struct Function * func =
        			&(((CordRep *)x) -> function);
        if (IS_SUBSTR(x)) printf("(Substring) ");
        printf("Function: %p (len: %d): ", x, (int)(func -> len));
        for (i = 0; i < 20 && i < func -> len; i++) {
            putchar((*(func -> fn))(i, func -> client_data));
        }
        if (i < func -> len) fputs("...", stdout);
        putchar('\n');
    }
}

/* Dump the internal representation of x to stdout	*/
void CORD_dump(CORD x)
{
    CORD_dump_inner(x, 0);
    fflush(stdout);
}

CORD CORD_cat_char_star(CORD x, const char * y, size_t leny)
{
    register size_t result_len;
    register size_t lenx;
    register int depth;
    
    if (x == CORD_EMPTY) return(y);
    if (leny == 0) return(x);
    if (CORD_IS_STRING(x)) {
        lenx = strlen(x);
        result_len = lenx + leny;
        if (result_len <= SHORT_LIMIT) {
            register char * result = GC_MALLOC_ATOMIC(result_len+1);
        
            if (result == 0) OUT_OF_MEMORY;
            memcpy(result, x, lenx);
            memcpy(result + lenx, y, leny);
            result[result_len] = '\0';
            return((CORD) result);
        } else {
            depth = 1;
        }
    } else {
    	register CORD right;
    	register CORD left;
    	register char * new_right;
    	register size_t right_len;
    	
    	lenx = LEN(x);
    	
        if (leny <= SHORT_LIMIT/2
    	    && IS_CONCATENATION(x)
            && CORD_IS_STRING(right = ((CordRep *)x) -> concatenation.right)) {
            /* Merge y into right part of x. */
            if (!CORD_IS_STRING(left = ((CordRep *)x) -> concatenation.left)) {
            	right_len = lenx - LEN(left);
            } else if (((CordRep *)x) -> concatenation.left_len != 0) {
                right_len = lenx - ((CordRep *)x) -> concatenation.left_len;
            } else {
            	right_len = strlen(right);
            }
            result_len = right_len + leny;  /* length of new_right */
            if (result_len <= SHORT_LIMIT) {
            	new_right = GC_MALLOC_ATOMIC(result_len + 1);
            	memcpy(new_right, right, right_len);
            	memcpy(new_right + right_len, y, leny);
            	new_right[result_len] = '\0';
            	y = new_right;
            	leny = result_len;
            	x = left;
            	lenx -= right_len;
            	/* Now fall through to concatenate the two pieces: */
            }
            if (CORD_IS_STRING(x)) {
                depth = 1;
            } else {
                depth = DEPTH(x) + 1;
            }
        } else {
            depth = DEPTH(x) + 1;
        }
        result_len = lenx + leny;
    }
    {
      /* The general case; lenx, result_len is known: */
    	register struct Concatenation * result;
    	
    	result = GC_NEW(struct Concatenation);
    	if (result == 0) OUT_OF_MEMORY;
    	result->header = CONCAT_HDR;
    	result->depth = depth;
    	if (lenx <= MAX_LEFT_LEN) result->left_len = lenx;
    	result->len = result_len;
    	result->left = x;
    	result->right = y;
    	if (depth >= MAX_DEPTH) {
    	    return(CORD_balance((CORD)result));
    	} else {
    	    return((CORD) result);
    	}
    }
}


CORD CORD_cat(CORD x, CORD y)
{
    register size_t result_len;
    register int depth;
    register size_t lenx;
    
    if (x == CORD_EMPTY) return(y);
    if (y == CORD_EMPTY) return(x);
    if (CORD_IS_STRING(y)) {
        return(CORD_cat_char_star(x, y, strlen(y)));
    } else if (CORD_IS_STRING(x)) {
        lenx = strlen(x);
        depth = DEPTH(y) + 1;
    } else {
        register int depthy = DEPTH(y);
        
        lenx = LEN(x);
        depth = DEPTH(x) + 1;
        if (depthy >= depth) depth = depthy + 1;
    }
    result_len = lenx + LEN(y);
    {
    	register struct Concatenation * result;
    	
    	result = GC_NEW(struct Concatenation);
    	if (result == 0) OUT_OF_MEMORY;
    	result->header = CONCAT_HDR;
    	result->depth = depth;
    	if (lenx <= MAX_LEFT_LEN) result->left_len = lenx;
    	result->len = result_len;
    	result->left = x;
    	result->right = y;
    	if (depth >= MAX_DEPTH) {
    	    return(CORD_balance((CORD)result));
    	} else {
    	    return((CORD) result);
    	}
    }
}



CORD CORD_from_fn(CORD_fn fn, void * client_data, size_t len)
{
    if (len <= 0) return(0);
    if (len <= SHORT_LIMIT) {
        register char * result;
        register size_t i;
        char buf[SHORT_LIMIT+1];
        register char c;
        
        for (i = 0; i < len; i++) {
            c = (*fn)(i, client_data);
            if (c == '\0') goto gen_case;
            buf[i] = c;
        }
        buf[i] = '\0';
        result = GC_MALLOC_ATOMIC(len+1);
        if (result == 0) OUT_OF_MEMORY;
        strcpy(result, buf);
        result[len] = '\0';
        return((CORD) result);
    }
  gen_case:
    {
    	register struct Function * result;
    	
    	result = GC_NEW(struct Function);
    	if (result == 0) OUT_OF_MEMORY;
    	result->header = FN_HDR;
    	/* depth is already 0 */
    	result->len = len;
    	result->fn = fn;
    	result->client_data = client_data;
    	return((CORD) result);
    }
}

size_t CORD_len(CORD x)
{
    if (x == 0) {
     	return(0);
    } else {
	return(GEN_LEN(x));
    }
}

struct substr_args {
    CordRep * sa_cord;
    size_t sa_index;
};

char CORD_index_access_fn(size_t i, void * client_data)
{
    register struct substr_args *descr = (struct substr_args *)client_data;
    
    return(((char *)(descr->sa_cord))[i + descr->sa_index]);
}

char CORD_apply_access_fn(size_t i, void * client_data)
{
    register struct substr_args *descr = (struct substr_args *)client_data;
    register struct Function * fn_cord = &(descr->sa_cord->function);
    
    return((*(fn_cord->fn))(i + descr->sa_index, fn_cord->client_data));
}

/* A version of CORD_substr that simply returns a function node, thus	*/
/* postponing its work.	The fourth argument is a function that may	*/
/* be used for efficient access to the ith character.			*/
/* Assumes i >= 0 and i + n < length(x).				*/
CORD CORD_substr_closure(CORD x, size_t i, size_t n, CORD_fn f)
{
    register struct substr_args * sa = GC_NEW(struct substr_args);
    CORD result;
    
    if (sa == 0) OUT_OF_MEMORY;
    sa->sa_cord = (CordRep *)x;
    sa->sa_index = i;
    result = CORD_from_fn(f, (void *)sa, n);
    ((CordRep *)result) -> function.header = SUBSTR_HDR;
    return (result);
}

# define SUBSTR_LIMIT (10 * SHORT_LIMIT)
	/* Substrings of function nodes and flat strings shorter than 	*/
	/* this are flat strings.  Othewise we use a functional 	*/
	/* representation, which is significantly slower to access.	*/

/* A version of CORD_substr that assumes i >= 0, n > 0, and i + n < length(x).*/
CORD CORD_substr_checked(CORD x, size_t i, size_t n)
{
    if (CORD_IS_STRING(x)) {
        if (n > SUBSTR_LIMIT) {
            return(CORD_substr_closure(x, i, n, CORD_index_access_fn));
        } else {
            register char * result = GC_MALLOC_ATOMIC(n+1);
            
            if (result == 0) OUT_OF_MEMORY;
            strncpy(result, x+i, n);
            result[n] = '\0';
            return(result);
        }
    } else if (IS_CONCATENATION(x)) {
    	register struct Concatenation * conc
    			= &(((CordRep *)x) -> concatenation);
    	register size_t left_len;
    	register size_t right_len;
    	
    	left_len = LEFT_LEN(conc);
    	right_len = conc -> len - left_len;
    	if (i >= left_len) {
    	    if (n == right_len) return(conc -> right);
    	    return(CORD_substr_checked(conc -> right, i - left_len, n));
    	} else if (i+n <= left_len) {
    	    if (n == left_len) return(conc -> left);
    	    return(CORD_substr_checked(conc -> left, i, n));
    	} else {
    	    /* Need at least one character from each side. */
    	    register CORD left_part;
    	    register CORD right_part;
    	    register size_t left_part_len = left_len - i;
     	
    	    if (i == 0) {
    	        left_part = conc -> left;
    	    } else {
    	        left_part = CORD_substr_checked(conc -> left, i, left_part_len);
    	    }
    	    if (i + n == right_len + left_len) {
    	         right_part = conc -> right;
    	    } else {
    	         right_part = CORD_substr_checked(conc -> right, 0,
    	    				          n - left_part_len);
    	    }
    	    return(CORD_cat(left_part, right_part));
    	}
    } else /* function */ {
        if (n > SUBSTR_LIMIT) {
            if (IS_SUBSTR(x)) {
            	/* Avoid nesting substring nodes.	*/
            	register struct Function * f = &(((CordRep *)x) -> function);
            	register struct substr_args *descr =
            			(struct substr_args *)(f -> client_data);
            	
            	return(CORD_substr_closure((CORD)descr->sa_cord,
            				   i + descr->sa_index,
            				   n, f -> fn));
            } else {
                return(CORD_substr_closure(x, i, n, CORD_apply_access_fn));
            }
        } else {
            char * result;
            register struct Function * f = &(((CordRep *)x) -> function);
            char buf[SUBSTR_LIMIT+1];
            register char * p = buf;
            register char c;
            register int j;
            register int lim = i + n;
            
            for (j = i; j < lim; j++) {
            	c = (*(f -> fn))(j, f -> client_data);
            	if (c == '\0') {
            	    return(CORD_substr_closure(x, i, n, CORD_apply_access_fn));
            	}
            	*p++ = c;
            }
            *p = '\0';
            result = GC_MALLOC_ATOMIC(n+1);
            if (result == 0) OUT_OF_MEMORY;
            strcpy(result, buf);
            return(result);
        }
    }
}

CORD CORD_substr(CORD x, size_t i, size_t n)
{
    register size_t len = CORD_len(x);
    
    if (i >= len || n <= 0) return(0);
    	/* n < 0 is impossible in a correct C implementation, but	*/
    	/* quite possible  under SunOS 4.X.				*/
    if (i + n > len) n = len - i;
#   ifndef __STDC__
      if (i < 0) ABORT("CORD_substr: second arg. negative");
    	/* Possible only if both client and C implementation are buggy.	*/
    	/* But empirically this happens frequently.			*/
#   endif
    return(CORD_substr_checked(x, i, n));
}

/* See cord.h for definition.  We assume i is in range.	*/
int CORD_iter5(CORD x, size_t i, CORD_iter_fn f1,
			 CORD_batched_iter_fn f2, void * client_data)
{
    if (x == 0) return(0);
    if (CORD_IS_STRING(x)) {
    	register const char *p = x+i;
    	
    	if (*p == '\0') ABORT("2nd arg to CORD_iter5 too big");
        if (f2 != CORD_NO_FN) {
            return((*f2)(p, client_data));
        } else {
	    while (*p) {
                if ((*f1)(*p, client_data)) return(1);
                p++;
	    }
	    return(0);
        }
    } else if (IS_CONCATENATION(x)) {
    	register struct Concatenation * conc
    			= &(((CordRep *)x) -> concatenation);
    	
    	
    	if (i > 0) {
    	    register size_t left_len = LEFT_LEN(conc);
    	    
    	    if (i >= left_len) {
    	        return(CORD_iter5(conc -> right, i - left_len, f1, f2,
    	        		  client_data));
    	    }
    	}
    	if (CORD_iter5(conc -> left, i, f1, f2, client_data)) {
    	    return(1);
    	}
    	return(CORD_iter5(conc -> right, 0, f1, f2, client_data));
    } else /* function */ {
        register struct Function * f = &(((CordRep *)x) -> function);
        register size_t j;
        register size_t lim = f -> len;
        
        for (j = i; j < lim; j++) {
            if ((*f1)((*(f -> fn))(j, f -> client_data), client_data)) {
                return(1);
            }
        }
        return(0);
    }
}
			
#undef CORD_iter
int CORD_iter(CORD x, CORD_iter_fn f1, void * client_data)
{
    return(CORD_iter5(x, 0, f1, CORD_NO_FN, client_data));
}

int CORD_riter4(CORD x, size_t i, CORD_iter_fn f1, void * client_data)
{
    if (x == 0) return(0);
    if (CORD_IS_STRING(x)) {
	register const char *p = x + i;
	register char c;
               
	for(;;) {
	    c = *p;
	    if (c == '\0') ABORT("2nd arg to CORD_riter4 too big");
            if ((*f1)(c, client_data)) return(1);
	    if (p == x) break;
            p--;
	}
	return(0);
    } else if (IS_CONCATENATION(x)) {
    	register struct Concatenation * conc
    			= &(((CordRep *)x) -> concatenation);
    	register CORD left_part = conc -> left;
    	register size_t left_len;
    	
    	left_len = LEFT_LEN(conc);
    	if (i >= left_len) {
    	    if (CORD_riter4(conc -> right, i - left_len, f1, client_data)) {
    	    	return(1);
    	    }
    	    return(CORD_riter4(left_part, left_len - 1, f1, client_data));
    	} else {
    	    return(CORD_riter4(left_part, i, f1, client_data));
    	}
    } else /* function */ {
        register struct Function * f = &(((CordRep *)x) -> function);
        register size_t j;
        
        for (j = i; ; j--) {
            if ((*f1)((*(f -> fn))(j, f -> client_data), client_data)) {
                return(1);
            }
            if (j == 0) return(0);
        }
    }
}

int CORD_riter(CORD x, CORD_iter_fn f1, void * client_data)
{
    return(CORD_riter4(x, CORD_len(x) - 1, f1, client_data));
}

/*
 * The following functions are concerned with balancing cords.
 * Strategy:
 * Scan the cord from left to right, keeping the cord scanned so far
 * as a forest of balanced trees of exponentialy decreasing length.
 * When a new subtree needs to be added to the forest, we concatenate all
 * shorter ones to the new tree in the appropriate order, and then insert
 * the result into the forest.
 * Crucial invariants:
 * 1. The concatenation of the forest (in decreasing order) with the
 *     unscanned part of the rope is equal to the rope being balanced.
 * 2. All trees in the forest are balanced.
 * 3. forest[i] has depth at most i.
 */

typedef struct {
    CORD c;
    size_t len;		/* Actual length of c 	*/
} ForestElement;

static size_t min_len [ MAX_DEPTH ];

static int min_len_init = 0;

int CORD_max_len;

typedef ForestElement Forest [ MAX_DEPTH ];
			/* forest[i].len >= fib(i+1)	        */
			/* The string is the concatenation	*/
			/* of the forest in order of DECREASING */
			/* indices.				*/

void CORD_init_min_len()
{
    register int i;
    register size_t last, previous, current;
        
    min_len[0] = previous = 1;
    min_len[1] = last = 2;
    for (i = 2; i < MAX_DEPTH; i++) {
    	current = last + previous;
    	if (current < last) /* overflow */ current = last;
    	min_len[i] = current;
    	previous = last;
    	last = current;
    }
    CORD_max_len = last - 1;
    min_len_init = 1;
}


void CORD_init_forest(ForestElement * forest, size_t max_len)
{
    register int i;
    
    for (i = 0; i < MAX_DEPTH; i++) {
    	forest[i].c = 0;
    	if (min_len[i] > max_len) return;
    }
    ABORT("Cord too long");
}

/* Add a leaf to the appropriate level in the forest, cleaning		*/
/* out lower levels as necessary.					*/
/* Also works if x is a balanced tree of concatenations; however	*/
/* in this case an extra concatenation node may be inserted above x;	*/
/* This node should not be counted in the statement of the invariants.	*/
void CORD_add_forest(ForestElement * forest, CORD x, size_t len)
{
    register int i = 0;
    register CORD sum = CORD_EMPTY;
    register size_t sum_len = 0;
    
    while (len > min_len[i + 1]) {
    	if (forest[i].c != 0) {
    	    sum = CORD_cat(forest[i].c, sum);
    	    sum_len += forest[i].len;
    	    forest[i].c = 0;
    	}
        i++;
    }
    /* Sum has depth at most 1 greter than what would be required 	*/
    /* for balance.							*/
    sum = CORD_cat(sum, x);
    sum_len += len;
    /* If x was a leaf, then sum is now balanced.  To see this		*/
    /* consider the two cases in which forest[i-1] either is or is 	*/
    /* not empty.							*/
    while (sum_len >= min_len[i]) {
    	if (forest[i].c != 0) {
    	    sum = CORD_cat(forest[i].c, sum);
    	    sum_len += forest[i].len;
    	    /* This is again balanced, since sum was balanced, and has	*/
    	    /* allowable depth that differs from i by at most 1.	*/
    	    forest[i].c = 0;
    	}
        i++;
    }
    i--;
    forest[i].c = sum;
    forest[i].len = sum_len;
}

CORD CORD_concat_forest(ForestElement * forest, size_t expected_len)
{
    register int i = 0;
    CORD sum = 0;
    size_t sum_len = 0;
    
    while (sum_len != expected_len) {
    	if (forest[i].c != 0) {
    	    sum = CORD_cat(forest[i].c, sum);
    	    sum_len += forest[i].len;
    	}
        i++;
    }
    return(sum);
}

/* Insert the frontier of x into forest.  Balanced subtrees are	*/
/* treated as leaves.  This potentially adds one to the depth	*/
/* of the final tree.						*/
void CORD_balance_insert(CORD x, size_t len, ForestElement * forest)
{
    register int depth;
    
    if (CORD_IS_STRING(x)) {
        CORD_add_forest(forest, x, len);
    } else if (IS_CONCATENATION(x)
               && ((depth = DEPTH(x)) >= MAX_DEPTH
                   || len < min_len[depth])) {
    	register struct Concatenation * conc
    			= &(((CordRep *)x) -> concatenation);
    	size_t left_len = LEFT_LEN(conc);
    	
    	CORD_balance_insert(conc -> left, left_len, forest);
    	CORD_balance_insert(conc -> right, len - left_len, forest);
    } else /* function or balanced */ {
    	CORD_add_forest(forest, x, len);
    }
}


CORD CORD_balance(CORD x)
{
    Forest forest;
    register size_t len;
    
    if (x == 0) return(0);
    if (CORD_IS_STRING(x)) return(x);
    if (!min_len_init) CORD_init_min_len();
    len = LEN(x);
    CORD_init_forest(forest, len);
    CORD_balance_insert(x, len, forest);
    return(CORD_concat_forest(forest, len));
}


/* Position primitives	*/

/* Private routines to deal with the hard cases only: */

/* P contains a prefix of the  path to cur_pos.	Extend it to a full	*/
/* path and set up leaf info.						*/
/* Return 0 if past the end of cord, 1 o.w.				*/
void CORD__extend_path(register CORD_pos p)
{
     register struct CORD_pe * current_pe = &(p[0].path[p[0].path_len]);
     register CORD top = current_pe -> pe_cord;
     register size_t pos = p[0].cur_pos;
     register size_t top_pos = current_pe -> pe_start_pos;
     register size_t top_len = GEN_LEN(top);
     
     /* Fill in the rest of the path. */
       while(!CORD_IS_STRING(top) && IS_CONCATENATION(top)) {
     	 register struct Concatenation * conc =
     	 		&(((CordRep *)top) -> concatenation);
     	 register size_t left_len;
     	 
     	 left_len = LEFT_LEN(conc);
     	 current_pe++;
     	 if (pos >= top_pos + left_len) {
     	     current_pe -> pe_cord = top = conc -> right;
     	     current_pe -> pe_start_pos = top_pos = top_pos + left_len;
     	     top_len -= left_len;
     	 } else {
     	     current_pe -> pe_cord = top = conc -> left;
     	     current_pe -> pe_start_pos = top_pos;
     	     top_len = left_len;
     	 }
     	 p[0].path_len++;
       }
     /* Fill in leaf description for fast access. */
       if (CORD_IS_STRING(top)) {
         p[0].cur_leaf = top;
         p[0].cur_start = top_pos;
         p[0].cur_end = top_pos + top_len;
       } else {
         p[0].cur_end = 0;
       }
       if (pos >= top_pos + top_len) p[0].path_len = CORD_POS_INVALID;
}

char CORD__pos_fetch(register CORD_pos p)
{
    /* Leaf is a function node */
    struct CORD_pe * pe = &((p)[0].path[(p)[0].path_len]);
    CORD leaf = pe -> pe_cord;
    register struct Function * f = &(((CordRep *)leaf) -> function);
    
    if (!IS_FUNCTION(leaf)) ABORT("CORD_pos_fetch: bad leaf");
    return ((*(f -> fn))(p[0].cur_pos - pe -> pe_start_pos, f -> client_data));
}

void CORD__next(register CORD_pos p)
{
    register size_t cur_pos = p[0].cur_pos + 1;
    register struct CORD_pe * current_pe = &((p)[0].path[(p)[0].path_len]);
    register CORD leaf = current_pe -> pe_cord;
    
    /* Leaf is not a string or we're at end of leaf */
    p[0].cur_pos = cur_pos;
    if (!CORD_IS_STRING(leaf)) {
    	/* Function leaf	*/
    	register struct Function * f = &(((CordRep *)leaf) -> function);
    	register size_t start_pos = current_pe -> pe_start_pos;
    	register size_t end_pos = start_pos + f -> len;
    	
    	if (cur_pos < end_pos) {
    	  /* Fill cache and return. */
    	    register size_t i;
    	    register size_t limit = cur_pos + FUNCTION_BUF_SZ;
    	    register CORD_fn fn = f -> fn;
    	    register void * client_data = f -> client_data;
    	    
    	    if (limit > end_pos) {
    	        limit = end_pos;
    	    }
    	    for (i = cur_pos; i < limit; i++) {
    	        p[0].function_buf[i - cur_pos] =
    	        	(*fn)(i - start_pos, client_data);
    	    }
    	    p[0].cur_start = cur_pos;
    	    p[0].cur_leaf = p[0].function_buf;
    	    p[0].cur_end = limit;
    	    return;
    	}
    }
    /* End of leaf	*/
    /* Pop the stack until we find two concatenation nodes with the 	*/
    /* same start position: this implies we were in left part.		*/
    {
    	while (p[0].path_len > 0
    	       && current_pe[0].pe_start_pos != current_pe[-1].pe_start_pos) {
    	    p[0].path_len--;
    	    current_pe--;
    	}
    	if (p[0].path_len == 0) {
	    p[0].path_len = CORD_POS_INVALID;
            return;
	}
    }
    p[0].path_len--;
    CORD__extend_path(p);
}

void CORD__prev(register CORD_pos p)
{
    register struct CORD_pe * pe = &(p[0].path[p[0].path_len]);
    
    if (p[0].cur_pos == 0) {
        p[0].path_len = CORD_POS_INVALID;
        return;
    }
    p[0].cur_pos--;
    if (p[0].cur_pos >= pe -> pe_start_pos) return;
    
    /* Beginning of leaf	*/
    
    /* Pop the stack until we find two concatenation nodes with the 	*/
    /* different start position: this implies we were in right part.	*/
    {
    	register struct CORD_pe * current_pe = &((p)[0].path[(p)[0].path_len]);
    	
    	while (p[0].path_len > 0
    	       && current_pe[0].pe_start_pos == current_pe[-1].pe_start_pos) {
    	    p[0].path_len--;
    	    current_pe--;
    	}
    }
    p[0].path_len--;
    CORD__extend_path(p);
}

#undef CORD_pos_fetch
#undef CORD_next
#undef CORD_prev
#undef CORD_pos_to_index
#undef CORD_pos_to_cord
#undef CORD_pos_valid

char CORD_pos_fetch(register CORD_pos p)
{
    if (p[0].cur_start <= p[0].cur_pos && p[0].cur_pos < p[0].cur_end) {
    	return(p[0].cur_leaf[p[0].cur_pos - p[0].cur_start]);
    } else {
        return(CORD__pos_fetch(p));
    }
}

void CORD_next(CORD_pos p)
{
    if (p[0].cur_pos < p[0].cur_end - 1) {
    	p[0].cur_pos++;
    } else {
    	CORD__next(p);
    }
}

void CORD_prev(CORD_pos p)
{
    if (p[0].cur_end != 0 && p[0].cur_pos > p[0].cur_start) {
    	p[0].cur_pos--;
    } else {
    	CORD__prev(p);
    }
}

size_t CORD_pos_to_index(CORD_pos p)
{
    return(p[0].cur_pos);
}

CORD CORD_pos_to_cord(CORD_pos p)
{
    return(p[0].path[0].pe_cord);
}

int CORD_pos_valid(CORD_pos p)
{
    return(p[0].path_len != CORD_POS_INVALID);
}

void CORD_set_pos(CORD_pos p, CORD x, size_t i)
{
    if (x == CORD_EMPTY) {
    	p[0].path_len = CORD_POS_INVALID;
    	return;
    }
    p[0].path[0].pe_cord = x;
    p[0].path[0].pe_start_pos = 0;
    p[0].path_len = 0;
    p[0].cur_pos = i;
    CORD__extend_path(p);
}
