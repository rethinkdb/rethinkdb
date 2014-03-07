/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   malloc_aligned.hpp
 * \author Andrey Semashev
 * \date   12.07.2013
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_DETAIL_MALLOC_ALIGNED_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_MALLOC_ALIGNED_HPP_INCLUDED_

#include <cstddef>
#include <cstdlib>
#include <boost/assert.hpp>
#include <boost/cstdint.hpp>
#include <boost/log/detail/config.hpp>

// MSVC has its own _aligned_malloc and _aligned_free functions.
// But MinGW doesn't declare these aligned memory allocation routines for MSVC 6 runtime.
#if defined(BOOST_WINDOWS) && !(defined(__MINGW32__) && __MSVCRT_VERSION__ < 0x0700)
#include <malloc.h>
#define BOOST_LOG_HAS_MSVC_ALIGNED_MALLOC 1
#endif

#if defined(BOOST_HAS_UNISTD_H)
#include <unistd.h> // _POSIX_VERSION
#endif

#if defined(__APPLE__) || defined(__APPLE_CC__) || defined(macintosh)
#include <AvailabilityMacros.h>
#if defined(MAC_OS_X_VERSION_MIN_REQUIRED) && (MAC_OS_X_VERSION_MIN_REQUIRED >= 1060)
// Mac OS X 10.6 and later have posix_memalign
#define BOOST_LOG_HAS_POSIX_MEMALIGN 1
#endif
#elif defined(__ANDROID__)
// Android NDK (up to release 8e, at least) doesn't have posix_memalign despite it defines POSIX macros as if it does.
// But we can use memalign() with free() on this platform.
#include <malloc.h>
#define BOOST_LOG_HAS_FREEABLE_MEMALIGN 1
#elif (defined(_POSIX_VERSION) && (_POSIX_VERSION >= 200112L)) || (defined(_XOPEN_SOURCE) && (_XOPEN_SOURCE >= 600))
// Solaris 10 does not have posix_memalign. Solaris 11 and later seem to have it.
#if !(defined(sun) || defined(__sun)) || defined(__SunOS_5_11) || defined(__SunOS_5_12)
#define BOOST_LOG_HAS_POSIX_MEMALIGN 1
#endif
#endif

#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#ifndef BOOST_LOG_CPU_CACHE_LINE_SIZE
//! The macro defines the CPU cache line size for the target architecture. This is mostly used for optimization.
#define BOOST_LOG_CPU_CACHE_LINE_SIZE 64
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

/*!
 * Allocates uninitialized aligned memory. Memory alignment must be a power of 2 and a multiple of sizeof(void*).
 * The implementation may impose an upper bound of the alignment as well.
 */
inline void* malloc_aligned(std::size_t size, uint32_t alignment)
{
#if defined(BOOST_LOG_HAS_POSIX_MEMALIGN)
    void* p = NULL;
    if (posix_memalign(&p, alignment, size) != 0)
        return NULL;
    return p;
#elif defined(BOOST_LOG_HAS_FREEABLE_MEMALIGN)
    return memalign(alignment, size);
#elif defined(BOOST_LOG_HAS_MSVC_ALIGNED_MALLOC)
    return _aligned_malloc(size, alignment);
#else
    BOOST_ASSERT(alignment >= sizeof(void*));
    void* p = std::malloc(size + alignment);
    if (p)
    {
        unsigned char* q = static_cast< unsigned char* >(p) + alignment;
#if defined(BOOST_HAS_INTPTR_T)
        q = (unsigned char*)((uintptr_t)q & (~(uintptr_t)(alignment - 1u)));
#else
        q -= ((std::size_t)q & (std::size_t)(alignment - 1u));
#endif
        // Here we assume that the system allocator aligns to 4 bytes at the very least.
        // Therefore we will always have at least 4 bytes before the aligned pointer.
        const uint32_t diff = q - static_cast< unsigned char* >(p);
        p = q;
        *reinterpret_cast< uint32_t* >(q - 4u) = diff;
    }
    return p;
#endif
}

/*!
 * Frees memory allocated with \c malloc_aligned.
 */
inline void free_aligned(void* p)
{
#if defined(BOOST_LOG_HAS_POSIX_MEMALIGN) || defined(BOOST_LOG_HAS_FREEABLE_MEMALIGN)
    free(p);
#elif defined(BOOST_LOG_HAS_MSVC_ALIGNED_MALLOC)
    _aligned_free(p);
#else
    if (p)
    {
        unsigned char* const q = static_cast< unsigned char* >(p);
        const uint32_t diff = *reinterpret_cast< uint32_t* >(q - 4u);
        std::free(q - diff);
    }
#endif
}

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_DETAIL_MALLOC_ALIGNED_HPP_INCLUDED_
