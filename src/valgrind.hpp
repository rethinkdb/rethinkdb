#ifndef VALGRIND_HPP_
#define VALGRIND_HPP_

/// @file valgrind.hpp
/// @brief Integration utilities for Valgrind memory debugging tool
///
/// Provides macros and functions to communicate with Valgrind about memory state,
/// allowing the debugger to track undefined values and other memory issues.
///
/// @defgroup ValgrindIntegration Valgrind Integration
/// Memory debugging and tracking utilities for Valgrind
/// @{

#ifdef VALGRIND
#include <valgrind/memcheck.h>
#endif

/// @brief Marks memory as undefined for Valgrind tracking
///
/// When running under Valgrind, this function marks the specified memory region
/// as containing undefined data. This is useful when you intentionally want Valgrind
/// to treat a value as uninitialized for testing error detection.
///
/// When not running under Valgrind, this function simply returns the value unchanged.
///
/// @tparam T The type of value
/// @param value The value to mark as undefined
/// @return The same value, now marked as undefined (under Valgrind) or unchanged
/// @note This is typically used for testing Valgrind's uninitialized value detection
/// @example
/// @code
/// // Test that code properly validates uninitialized input
/// int test_value = valgrind_undefined(0);
/// // Valgrind will now report any use of test_value as undefined
/// @endcode
template <class T>
T valgrind_undefined(T value) {
#ifdef VALGRIND
    UNUSED auto x = VALGRIND_MAKE_MEM_UNDEFINED(&value, sizeof(value));
#endif
    return value;
}

/// @}

#endif  // VALGRIND_HPP_
