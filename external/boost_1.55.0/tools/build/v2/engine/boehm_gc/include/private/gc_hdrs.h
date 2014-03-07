/* 
 * Copyright 1988, 1989 Hans-J. Boehm, Alan J. Demers
 * Copyright (c) 1991-1994 by Xerox Corporation.  All rights reserved.
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
/* Boehm, July 11, 1995 11:54 am PDT */
# ifndef GC_HEADERS_H
# define GC_HEADERS_H
typedef struct hblkhdr hdr;

# if CPP_WORDSZ != 32 && CPP_WORDSZ < 36
	--> Get a real machine.
# endif

/*
 * The 2 level tree data structure that is used to find block headers.
 * If there are more than 32 bits in a pointer, the top level is a hash
 * table.
 *
 * This defines HDR, GET_HDR, and SET_HDR, the main macros used to
 * retrieve and set object headers.
 *
 * We take advantage of a header lookup
 * cache.  This is a locally declared direct mapped cache, used inside
 * the marker.  The HC_GET_HDR macro uses and maintains this
 * cache.  Assuming we get reasonable hit rates, this shaves a few
 * memory references from each pointer validation.
 */

# if CPP_WORDSZ > 32
#   define HASH_TL
# endif

/* Define appropriate out-degrees for each of the two tree levels	*/
# ifdef SMALL_CONFIG
#   define LOG_BOTTOM_SZ 11
	/* Keep top index size reasonable with smaller blocks. */
# else
#   define LOG_BOTTOM_SZ 10
# endif
# ifndef HASH_TL
#   define LOG_TOP_SZ (WORDSZ - LOG_BOTTOM_SZ - LOG_HBLKSIZE)
# else
#   define LOG_TOP_SZ 11
# endif
# define TOP_SZ (1 << LOG_TOP_SZ)
# define BOTTOM_SZ (1 << LOG_BOTTOM_SZ)

#ifndef SMALL_CONFIG
# define USE_HDR_CACHE
#endif

/* #define COUNT_HDR_CACHE_HITS  */

# ifdef COUNT_HDR_CACHE_HITS
    extern word GC_hdr_cache_hits;
    extern word GC_hdr_cache_misses;
#   define HC_HIT() ++GC_hdr_cache_hits
#   define HC_MISS() ++GC_hdr_cache_misses
# else
#   define HC_HIT()
#   define HC_MISS()
# endif

  typedef struct hce {
    word block_addr;  	/* right shifted by LOG_HBLKSIZE */
    hdr * hce_hdr;
  } hdr_cache_entry;

# define HDR_CACHE_SIZE 8  /* power of 2 */

# define DECLARE_HDR_CACHE \
	hdr_cache_entry hdr_cache[HDR_CACHE_SIZE]

# define INIT_HDR_CACHE BZERO(hdr_cache, sizeof(hdr_cache))

# define HCE(h) hdr_cache + (((word)(h) >> LOG_HBLKSIZE) & (HDR_CACHE_SIZE-1))

# define HCE_VALID_FOR(hce,h) ((hce) -> block_addr == \
				((word)(h) >> LOG_HBLKSIZE))

# define HCE_HDR(h) ((hce) -> hce_hdr)

#ifdef PRINT_BLACK_LIST
  hdr * GC_header_cache_miss(ptr_t p, hdr_cache_entry *hce, ptr_t source);
# define HEADER_CACHE_MISS(p, hce, source) \
	  GC_header_cache_miss(p, hce, source)
#else
  hdr * GC_header_cache_miss(ptr_t p, hdr_cache_entry *hce);
# define HEADER_CACHE_MISS(p, hce, source) GC_header_cache_miss(p, hce)
#endif

/* Set hhdr to the header for p.  Analogous to GET_HDR below,		*/
/* except that in the case of large objects, it				*/
/* gets the header for the object beginning, if GC_all_interior_ptrs	*/
/* is set.								*/
/* Returns zero if p points to somewhere other than the first page	*/
/* of an object, and it is not a valid pointer to the object.		*/
# define HC_GET_HDR(p, hhdr, source, exit_label) \
	{ \
	  hdr_cache_entry * hce = HCE(p); \
	  if (EXPECT(HCE_VALID_FOR(hce, p), 1)) { \
	    HC_HIT(); \
	    hhdr = hce -> hce_hdr; \
	  } else { \
	    hhdr = HEADER_CACHE_MISS(p, hce, source); \
	    if (0 == hhdr) goto exit_label; \
	  } \
	}

typedef struct bi {
    hdr * index[BOTTOM_SZ];
	/*
 	 * The bottom level index contains one of three kinds of values:
	 * 0 means we're not responsible for this block,
	 *   or this is a block other than the first one in a free block.
	 * 1 < (long)X <= MAX_JUMP means the block starts at least
	 *        X * HBLKSIZE bytes before the current address.
	 * A valid pointer points to a hdr structure. (The above can't be
	 * valid pointers due to the GET_MEM return convention.)
	 */
    struct bi * asc_link;	/* All indices are linked in	*/
    				/* ascending order...		*/
    struct bi * desc_link;	/* ... and in descending order.	*/
    word key;			/* high order address bits.	*/
# ifdef HASH_TL
    struct bi * hash_link;	/* Hash chain link.		*/
# endif
} bottom_index;

/* extern bottom_index GC_all_nils; - really part of GC_arrays */

/* extern bottom_index * GC_top_index []; - really part of GC_arrays */
				/* Each entry points to a bottom_index.	*/
				/* On a 32 bit machine, it points to 	*/
				/* the index for a set of high order	*/
				/* bits equal to the index.  For longer	*/
				/* addresses, we hash the high order	*/
				/* bits to compute the index in 	*/
				/* GC_top_index, and each entry points	*/
				/* to a hash chain.			*/
				/* The last entry in each chain is	*/
				/* GC_all_nils.				*/


# define MAX_JUMP (HBLKSIZE - 1)

# define HDR_FROM_BI(bi, p) \
		((bi)->index[((word)(p) >> LOG_HBLKSIZE) & (BOTTOM_SZ - 1)])
# ifndef HASH_TL
#   define BI(p) (GC_top_index \
		[(word)(p) >> (LOG_BOTTOM_SZ + LOG_HBLKSIZE)])
#   define HDR_INNER(p) HDR_FROM_BI(BI(p),p)
#   ifdef SMALL_CONFIG
#	define HDR(p) GC_find_header((ptr_t)(p))
#   else
#	define HDR(p) HDR_INNER(p)
#   endif
#   define GET_BI(p, bottom_indx) (bottom_indx) = BI(p)
#   define GET_HDR(p, hhdr) (hhdr) = HDR(p)
#   define SET_HDR(p, hhdr) HDR_INNER(p) = (hhdr)
#   define GET_HDR_ADDR(p, ha) (ha) = &(HDR_INNER(p))
# else /* hash */
/*  Hash function for tree top level */
#   define TL_HASH(hi) ((hi) & (TOP_SZ - 1))
/*  Set bottom_indx to point to the bottom index for address p */
#   define GET_BI(p, bottom_indx) \
	{ \
	    register word hi = \
	        (word)(p) >> (LOG_BOTTOM_SZ + LOG_HBLKSIZE); \
	    register bottom_index * _bi = GC_top_index[TL_HASH(hi)]; \
	    \
	    while (_bi -> key != hi && _bi != GC_all_nils) \
	    	_bi = _bi -> hash_link; \
	    (bottom_indx) = _bi; \
	}
#   define GET_HDR_ADDR(p, ha) \
	{ \
	    register bottom_index * bi; \
	    \
	    GET_BI(p, bi);	\
	    (ha) = &(HDR_FROM_BI(bi, p)); \
	}
#   define GET_HDR(p, hhdr) { register hdr ** _ha; GET_HDR_ADDR(p, _ha); \
			      (hhdr) = *_ha; }
#   define SET_HDR(p, hhdr) { register hdr ** _ha; GET_HDR_ADDR(p, _ha); \
			      *_ha = (hhdr); }
#   define HDR(p) GC_find_header((ptr_t)(p))
# endif
			    
/* Is the result a forwarding address to someplace closer to the	*/
/* beginning of the block or NIL?					*/
# define IS_FORWARDING_ADDR_OR_NIL(hhdr) ((size_t) (hhdr) <= MAX_JUMP)

/* Get an HBLKSIZE aligned address closer to the beginning of the block */
/* h.  Assumes hhdr == HDR(h) and IS_FORWARDING_ADDR(hhdr).		*/
# define FORWARDED_ADDR(h, hhdr) ((struct hblk *)(h) - (size_t)(hhdr))
# endif /*  GC_HEADERS_H */
