/**
 * @file math.hpp
 * @brief Mathematical utilities for alignment, rounding, and value constraints.
 *
 * Provides template functions for common mathematical operations in systems
 * programming, including alignment operations, rounding, clamping, and
 * hexadecimal conversion utilities.
 */

#ifndef MATH_HPP_
#define MATH_HPP_

#include <math.h>
#include <stdint.h>

/**
 * @defgroup MathUtilities Mathematical Utilities
 * @brief Alignment, rounding, and conversion helper functions
 */

/**
 * @ingroup MathUtilities
 * @brief Aligns a value UP to the next multiple of alignment.
 *
 * Computes the smallest value >= input that is a multiple of alignment.
 * Useful for memory alignment operations in systems programming.
 *
 * @tparam T1 Type of the value to align (integral type).
 * @tparam T2 Type of the alignment (integral type).
 * @param value The value to align.
 * @param alignment The alignment boundary (must be > 0).
 * @return The smallest value >= input that is a multiple of alignment.
 *
 * Example:
 * @code
 * uint64_t addr = 1000;
 * uint64_t aligned = ceil_aligned(addr, 4096);  // Aligns to page boundary
 * assert(aligned % 4096 == 0);
 * assert(aligned >= 1000);
 * @endcode
 */
template <class T1, class T2>
T1 ceil_aligned(T1 value, T2 alignment) {
    return value + alignment - (((value + alignment - 1) % alignment) + 1);
}

/**
 * @ingroup MathUtilities
 * @brief Divides and rounds UP to the next integer.
 *
 * Computes ceil(dividend / divisor) without using floating point.
 * Equivalent to (dividend + divisor - 1) / divisor.
 *
 * @tparam T1 Type of the dividend.
 * @tparam T2 Type of the divisor.
 * @param dividend The value being divided.
 * @param alignment The divisor (must be > 0).
 * @return ceil(dividend / alignment).
 *
 * Example:
 * @code
 * size_t bytes = 1000;
 * size_t pages = ceil_divide(bytes, 4096);  // Pages needed for bytes
 * assert(pages * 4096 >= bytes);
 * @endcode
 */
template <class T1, class T2>
T1 ceil_divide(T1 dividend, T2 alignment) {
    return (dividend + alignment - 1) / alignment;
}

/**
 * @ingroup MathUtilities
 * @brief Aligns a value DOWN to the previous multiple of alignment.
 *
 * Computes the largest value <= input that is a multiple of alignment.
 *
 * @tparam T1 Type of the value to align.
 * @tparam T2 Type of the alignment.
 * @param value The value to align downward.
 * @param alignment The alignment boundary (must be > 0).
 * @return The largest value <= input that is a multiple of alignment.
 *
 * Example:
 * @code
 * uint64_t addr = 5000;
 * uint64_t aligned = floor_aligned(addr, 4096);  // Aligns down
 * assert(aligned % 4096 == 0);
 * assert(aligned <= 5000);
 * @endcode
 */
template <class T1, class T2>
T1 floor_aligned(T1 value, T2 alignment) {
    return value - (value % alignment);
}

/**
 * @ingroup MathUtilities
 * @brief Computes the rounding overhead to next alignment boundary.
 *
 * Returns (alignment - value % alignment) % alignment, which is the number
 * of units needed to round value UP to the next multiple of alignment.
 *
 * @tparam T1 Type of the value.
 * @tparam T2 Type of the alignment.
 * @param value The value to compute overhead for.
 * @param alignment The alignment boundary.
 * @return The number of units to add to value to reach next alignment.
 *
 * Example:
 * @code
 * size_t size = 1000;
 * size_t overhead = ceil_modulo(size, 8);  // Padding for 8-byte alignment
 * size_t aligned_size = size + overhead;
 * assert(aligned_size % 8 == 0);
 * @endcode
 */
template <class T1, class T2>
T1 ceil_modulo(T1 value, T2 alignment) {
    T1 x = (value + alignment - 1) % alignment;
    return value + alignment - ((x < 0 ? x + alignment : x) + 1);
}

/**
 * @ingroup MathUtilities
 * @brief Clamps a value to the range [lo, hi].
 *
 * Returns the value constrained to be within the specified bounds.
 *
 * @tparam T The type of the value and bounds.
 * @param x The value to clamp.
 * @param lo The lower bound (inclusive).
 * @param hi The upper bound (inclusive).
 * @return The clamped value: max(lo, min(x, hi)).
 *
 * Example:
 * @code
 * int score = clamp(score, 0, 100);  // Ensure score is in [0, 100]
 * @endcode
 */
template <class T>
T clamp(T x, T lo, T hi) {
    return x < lo ? lo : x > hi ? hi : x;
}

/**
 * @ingroup MathUtilities
 * @brief Checks if x divides y evenly.
 *
 * @param x The divisor.
 * @param y The dividend.
 * @return true if y % x == 0, false otherwise.
 *
 * Example:
 * @code
 * if (divides(4, 20)) {  // Is 20 divisible by 4?
 *     // ...
 * }
 * @endcode
 */
constexpr inline bool divides(int64_t x, int64_t y) {
    return y % x == 0;
}

/**
 * @ingroup MathUtilities
 * @brief Rounds an integer UP to the next power of two.
 *
 * @param x The input value.
 * @return The smallest power of two >= x.
 *
 * Example:
 * @code
 * int64_t size = int64_round_up_to_power_of_two(1000);  // Returns 1024
 * @endcode
 */
int64_t int64_round_up_to_power_of_two(int64_t x);

/**
 * @ingroup MathUtilities
 * @brief Rounds an unsigned 64-bit integer UP to the next power of two.
 *
 * @param x The input value.
 * @return The smallest power of two >= x.
 */
uint64_t uint64_round_up_to_power_of_two(uint64_t x);

/**
 * @ingroup MathUtilities
 * @brief Checks if a double is finite (not NaN, not infinity).
 *
 * Wrapper around std::isfinite that forwards to the isfinite macro or
 * std::isfinite depending on the implementation.
 *
 * @param value The double value to check.
 * @return true if value is finite, false if NaN or infinity.
 */
bool risfinite(double);

/**
 * @ingroup MathUtilities
 * @brief Converts a hexadecimal character to an integer.
 *
 * Converts characters '0'-'9' to 0-9, 'a'-'f' to 10-15, and
 * 'A'-'F' to 10-15.
 *
 * @param c The hexadecimal character.
 * @param out Pointer to output the converted value.
 * @return true if c is a valid hex digit, false otherwise.
 *
 * Example:
 * @code
 * int value;
 * if (hex_to_int('A', &value)) {
 *     // value == 10
 * }
 * @endcode
 */
bool hex_to_int(char c, int *out);

/**
 * @ingroup MathUtilities
 * @brief Converts an integer to its hexadecimal character representation.
 *
 * Converts 0-9 to '0'-'9' and 10-15 to 'A'-'F'.
 *
 * @param i An integer in [0, 15].
 * @return The uppercase hexadecimal character representation.
 *
 * Example:
 * @code
 * char hex = int_to_hex(10);  // Returns 'A'
 * @endcode
 */
char int_to_hex(int i);

#endif  // MATH_HPP_
