/// @file memory_utils.hpp
/// @brief Memory allocation utilities with alignment and crash-on-failure semantics
///
/// Provides wrappers around standard malloc/realloc that automatically crash
/// if allocation fails, and specialized functions for aligned memory allocation.
/// This ensures no null-pointer checks are needed after allocation.
///
/// @defgroup MemoryAllocation Memory Allocation Utilities
/// Safe memory allocation with alignment and failure guarantees
/// @{

#ifndef MEMORY_HPP_
#define MEMORY_HPP_

#include <stddef.h>

/// @brief Allocates aligned memory with specified byte alignment
/// Allocates @p size bytes with alignment boundary of @p alignment
/// @param size The number of bytes to allocate
/// @param alignment The required alignment boundary in bytes (must be power of 2)
/// @return A pointer to allocated memory aligned to @p alignment boundary
/// @note Must be freed with raw_free_aligned()
/// @example
/// @code
/// // Allocate 1024 bytes aligned to 64-byte boundary
/// void *ptr = raw_malloc_aligned(1024, 64);
/// // ... use memory ...
/// raw_free_aligned(ptr);
/// @endcode
void *raw_malloc_aligned(size_t size, size_t alignment);

/// @brief Frees memory allocated by raw_malloc_aligned()
/// Deallocates aligned memory allocated by raw_malloc_aligned()
/// @param ptr Pointer to memory allocated by raw_malloc_aligned()
/// @note Only call with pointers from raw_malloc_aligned(), not malloc()
void raw_free_aligned(void *ptr);

/// @brief Allocates memory aligned to system page boundary
/// Allocates memory with alignment matching the system's page size (typically 4096 bytes)
/// @param size The number of bytes to allocate
/// @return A pointer to page-aligned allocated memory
/// @note Only available on POSIX systems (not Windows)
/// @example
/// @code
/// void *page_buffer = raw_malloc_page_aligned(8192);  // 2 pages
/// @endcode
#ifndef _WIN32
void *raw_malloc_page_aligned(size_t size);
#endif

/// @brief Safe malloc that crashes on allocation failure
/// Allocates @p size bytes and crashes the program if allocation fails
/// Useful for guaranteed memory acquisition without error checking
/// @param size The number of bytes to allocate
/// @return A pointer to allocated memory (never nullptr)
/// @example
/// @code
/// int *array = (int*)rmalloc(1000 * sizeof(int));  // Won't return if OOM
/// // Safe to use immediately without null-check
/// @endcode
void *rmalloc(size_t size);

/// @brief Safe realloc that crashes on allocation failure
/// Reallocates @p ptr to @p size bytes and crashes if reallocation fails
/// If ptr is nullptr, behaves like rmalloc()
/// @param ptr The pointer to reallocate (can be nullptr)
/// @param size The new size in bytes
/// @return A pointer to the reallocated memory (never nullptr)
/// @example
/// @code
/// int *array = (int*)rmalloc(100 * sizeof(int));
/// array = (int*)rrealloc(array, 200 * sizeof(int));  // Double capacity
/// @endcode
void *rrealloc(void *ptr, size_t size);

/// @}

#endif  // MEMORY_HPP_
