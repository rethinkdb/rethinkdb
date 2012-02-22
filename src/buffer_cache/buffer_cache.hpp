#ifndef __BUFFER_CACHE_HPP__
#define __BUFFER_CACHE_HPP__

// Predeclarations are in here.
#include "buffer_cache/types.hpp"

/* Choose our cache */

// This file should be kept synced up with buffer_cache/types.hpp,
// which defines what buf_lock_t, cache_t, etc, are.

#ifndef MOCK_CACHE_CHECK

#include "buffer_cache/mirrored/mirrored.hpp"

#if !defined(VALGRIND) && !defined(NDEBUG)

#include "buffer_cache/semantic_checking.hpp"

#else  // !defined(VALGRIND) && !defined(NDEBUG)

// include nothing else

#endif  // !defined(VALGRIND) && !defined(NDEBUG)

#else  // MOCK_CACHE_CHECK

#include "buffer_cache/mock.hpp"

#if !defined(VALGRIND)
#include "buffer_cache/semantic_checking.hpp"
#endif  // !defined(VALGRIND)

#endif  // MOCK_CACHE_CHECK


#endif /* __BUFFER_CACHE_HPP__ */
