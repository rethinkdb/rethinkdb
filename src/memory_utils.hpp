/**
 * @file memory_utils.hpp
 * @brief Memory allocation utilities with alignment and error handling.
 *
 * Provides safe memory allocation functions that handle alignment requirements
 * and check for allocation failures by crashing (fail-fast approach).
 */

#ifndef MEMORY_HPP_
#define MEMORY_HPP_

#include <stddef.h>

/**
 * @defgroup MemoryUtilities Memory Allocation Utilities
 * @brief Safe memory allocation with alignment and error checking
 */

/**
 * @ingroup MemoryUtilities
 * @brief Allocate memory aligned to a specific boundary.
 *
 * Allocates memory with the specified alignment requirement. Useful for
 * SIMD operations, cache line alignment, or page boundaries.
 *
 * @param size The number of bytes to allocate.
 * @param alignment The alignment requirement in bytes (must be power of 2).
 * @return A pointer to the allocated memory, aligned to the specified boundary.
 *
 * @note The returned memory must be freed with raw_free_aligned().
 * @note Crashes if allocation fails.
 *
 * Example:
 * @code
 * void *buf = raw_malloc_aligned(1024, 64);  // 64-byte aligned
 * // ... use buffer ...
 * raw_free_aligned(buf);
 * @endcode
 */
void *raw_malloc_aligned(size_t size, size_t alignment);

/**
 * @ingroup MemoryUtilities
 * @brief Free memory allocated with raw_malloc_aligned().
 *
 * @param ptr Pointer previously returned by raw_malloc_aligned().
 *
 * @note Safe to call with nullptr.
 */
void raw_free_aligned(void *ptr);

/**
 * @ingroup MemoryUtilities
 * @brief Allocate page-aligned memory (not available on Windows).
 *
 * Allocates memory aligned to the system page size (typically 4096 bytes).
 * Useful for operations requiring page-granule alignment.
 *
 * @param size The number of bytes to allocate.
 * @return A pointer to page-aligned memory.
 *
 * @note Only available on non-Windows platforms.
 * @note The returned memory must be freed with raw_free_aligned().
 * @note Crashes if allocation fails.
 *
 * Example:
 * @code
 * void *page = raw_malloc_page_aligned(4096);
 * // ... use page ...
 * raw_free_aligned(page);
 * @endcode
 */
#ifndef _WIN32
void *raw_malloc_page_aligned(size_t size);
#endif

/**
 * @ingroup MemoryUtilities
 * @brief Allocate memory with automatic error checking.
 *
 * Calls malloc() and crashes if the allocation fails. Use this for
 * allocations where failure is not acceptable.
 *
 * @param size The number of bytes to allocate.
 * @return A pointer to allocated memory (never nullptr).
 *
 * @note Crashes the program if allocation fails.
 *
 * Example:
 * @code
 * int *array = (int *)rmalloc(1000 * sizeof(int));
 * // Guaranteed to succeed or crash
 * @endcode
 */
void *rmalloc(size_t size);

/**
 * @ingroup MemoryUtilities
 * @brief Reallocate memory with automatic error checking.
 *
 * Calls realloc() and crashes if the allocation fails.
 *
 * @param ptr Pointer to previously allocated memory (or nullptr).
 * @param size The new size in bytes.
 * @return A pointer to the reallocated memory (never nullptr).
 *
 * @note Crashes the program if reallocation fails.
 *
 * Example:
 * @code
 * int *array = (int *)rmalloc(100);
 * array = (int *)rrealloc(array, 200);  // Crash on failure
 * @endcode
 */
void *rrealloc(void *ptr, size_t size);

#endif  // MEMORY_HPP_
