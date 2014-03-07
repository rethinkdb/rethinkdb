//  Copyright (C) 2009 Tim Blechmann
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_LOCKFREE_PREFIX_HPP_INCLUDED
#define BOOST_LOCKFREE_PREFIX_HPP_INCLUDED

/* this file defines the following macros:
   BOOST_LOCKFREE_CACHELINE_BYTES: size of a cache line
   BOOST_LOCKFREE_PTR_COMPRESSION: use tag/pointer compression to utilize parts
                                   of the virtual address space as tag (at least 16bit)
   BOOST_LOCKFREE_DCAS_ALIGNMENT:  symbol used for aligning structs at cache line
                                   boundaries
*/

#define BOOST_LOCKFREE_CACHELINE_BYTES 64

#ifdef _MSC_VER

#define BOOST_LOCKFREE_CACHELINE_ALIGNMENT __declspec(align(BOOST_LOCKFREE_CACHELINE_BYTES))

#if defined(_M_IX86)
    #define BOOST_LOCKFREE_DCAS_ALIGNMENT
#elif defined(_M_X64) || defined(_M_IA64)
    #define BOOST_LOCKFREE_PTR_COMPRESSION 1
    #define BOOST_LOCKFREE_DCAS_ALIGNMENT __declspec(align(16))
#endif

#endif /* _MSC_VER */

#ifdef __GNUC__

#define BOOST_LOCKFREE_CACHELINE_ALIGNMENT __attribute__((aligned(BOOST_LOCKFREE_CACHELINE_BYTES)))

#if defined(__i386__) || defined(__ppc__)
    #define BOOST_LOCKFREE_DCAS_ALIGNMENT
#elif defined(__x86_64__)
    #define BOOST_LOCKFREE_PTR_COMPRESSION 1
    #define BOOST_LOCKFREE_DCAS_ALIGNMENT __attribute__((aligned(16)))
#elif defined(__alpha__)
    // LATER: alpha may benefit from pointer compression. but what is the maximum size of the address space?
    #define BOOST_LOCKFREE_DCAS_ALIGNMENT
#endif
#endif /* __GNUC__ */

#ifndef BOOST_LOCKFREE_DCAS_ALIGNMENT
#define BOOST_LOCKFREE_DCAS_ALIGNMENT /*BOOST_LOCKFREE_DCAS_ALIGNMENT*/
#endif

#ifndef BOOST_LOCKFREE_CACHELINE_ALIGNMENT
#define BOOST_LOCKFREE_CACHELINE_ALIGNMENT /*BOOST_LOCKFREE_CACHELINE_ALIGNMENT*/
#endif

#endif /* BOOST_LOCKFREE_PREFIX_HPP_INCLUDED */
