/// @file math.hpp
/// @brief Mathematical utility functions and algorithms
///
/// Provides common mathematical operations needed throughout RethinkDB,
/// including alignment calculations, rounding, clamping, and bit operations.
///
/// @defgroup MathUtilities Mathematical Utilities
/// Common math functions for alignment, rounding, and bit operations
/// @{

#ifndef MATH_HPP_
#define MATH_HPP_

#include <math.h>
#include <stdint.h>

/// @brief Aligns a value up to the nearest multiple of alignment
/// Rounds @p value up to the nearest multiple of @p alignment
/// @tparam T1 The type of the value
/// @tparam T2 The type of the alignment
/// @param value The value to align
/// @param alignment The alignment boundary
/// @return The aligned value rounded up
/// @example
/// @code
/// int aligned = ceil_aligned(17, 8);  // Returns 24 (next multiple of 8)
/// int aligned2 = ceil_aligned(16, 8);  // Returns 16 (already aligned)
/// @endcode
template <class T1, class T2>
T1 ceil_aligned(T1 value, T2 alignment) {
    return value + alignment - (((value + alignment - 1) % alignment) + 1);
}

/// @brief Divides with ceiling, rounding up the quotient
/// Computes the ceiling of @p dividend / @p alignment
/// Equivalent to `ceil(dividend / alignment)` using integer arithmetic
/// @tparam T1 The dividend type
/// @tparam T2 The divisor type
/// @param dividend The dividend
/// @param alignment The divisor (alignment value)
/// @return The quotient rounded up
/// @example
/// @code
/// int result = ceil_divide(17, 8);  // Returns 3 (ceil(17/8) = 3)
/// int result2 = ceil_divide(16, 8);  // Returns 2
/// int result3 = ceil_divide(1, 8);   // Returns 1
/// @endcode
template <class T1, class T2>
T1 ceil_divide(T1 dividend, T2 alignment) {
    return (dividend + alignment - 1) / alignment;
}

/// @brief Aligns a value down to the nearest multiple of alignment
/// Rounds @p value down to the nearest multiple of @p alignment
/// @tparam T1 The type of the value
/// @tparam T2 The type of the alignment
/// @param value The value to align
/// @param alignment The alignment boundary
/// @return The aligned value rounded down
/// @example
/// @code
/// int aligned = floor_aligned(17, 8);  // Returns 16 (previous multiple of 8)
/// int aligned2 = floor_aligned(16, 8);  // Returns 16 (already aligned)
/// @endcode
template <class T1, class T2>
T1 floor_aligned(T1 value, T2 alignment) {
    return value - (value % alignment);
}

/// @brief Computes the modulo needed to reach the next multiple of alignment
/// Returns the amount needed to add to @p value to reach a multiple of @p alignment
/// @tparam T1 The type of the value
/// @tparam T2 The type of the alignment
/// @param value The value to compute modulo for
/// @param alignment The alignment boundary
/// @return The modulo offset
/// @example
/// @code
/// int offset = ceil_modulo(17, 8);  // Returns 7 (add 7 to get to 24)
/// int offset2 = ceil_modulo(16, 8);  // Returns 0 (already aligned)
/// @endcode
template <class T1, class T2>
T1 ceil_modulo(T1 value, T2 alignment) {
    T1 x = (value + alignment - 1) % alignment;
    return value + alignment - ((x < 0 ? x + alignment : x) + 1);
}

/// @brief Clamps a value within a range [lo, hi]
/// Restricts @p x to be within the range [@p lo, @p hi]
/// @tparam T The numeric type
/// @param x The value to clamp
/// @param lo The lower bound (inclusive)
/// @param hi The upper bound (inclusive)
/// @return The clamped value
/// @example
/// @code
/// int value = clamp(150, 0, 100);  // Returns 100
/// int value2 = clamp(50, 0, 100);  // Returns 50
/// int value3 = clamp(-10, 0, 100);  // Returns 0
/// @endcode
template <class T>
T clamp(T x, T lo, T hi) {
    return x < lo ? lo : x > hi ? hi : x;
}

/// @brief Checks if one integer divides another
/// Determines whether @p x divides @p y evenly (y % x == 0)
/// @param x The potential divisor
/// @param y The dividend
/// @return true if y is divisible by x, false otherwise
/// @example
/// @code
/// bool result = divides(2, 10);  // Returns true (10 is even)
/// bool result2 = divides(3, 10);  // Returns false
/// bool result3 = divides(5, 25);  // Returns true
/// @endcode
constexpr inline bool divides(int64_t x, int64_t y) {
    return y % x == 0;
}

/// @brief Rounds a 64-bit signed integer up to the next power of 2
/// Finds the smallest power of 2 >= @p x
/// @param x The input value
/// @return The next power of 2, or x if already a power of 2
/// @example
/// @code
/// int64_t result = int64_round_up_to_power_of_two(17);  // Returns 32
/// int64_t result2 = int64_round_up_to_power_of_two(16);  // Returns 16
/// @endcode
int64_t int64_round_up_to_power_of_two(int64_t x);

/// @brief Rounds a 64-bit unsigned integer up to the next power of 2
/// Finds the smallest power of 2 >= @p x
/// @param x The input value
/// @return The next power of 2, or x if already a power of 2
/// @example
/// @code
/// uint64_t result = uint64_round_up_to_power_of_two(1025);  // Returns 2048
/// uint64_t result2 = uint64_round_up_to_power_of_two(1024);  // Returns 1024
/// @endcode
uint64_t uint64_round_up_to_power_of_two(uint64_t x);

/// @brief Safely checks if a double-precision float is finite
/// Forwards to isfinite macro or std::isfinite depending on platform
/// @param val The double value to check
/// @return true if val is finite (not NaN or infinite), false otherwise
/// @example
/// @code
/// bool result = risfinite(3.14);  // Returns true
/// bool result2 = risfinite(INFINITY);  // Returns false
/// bool result3 = risfinite(NAN);  // Returns false
/// @endcode
bool risfinite(double);

/// @brief Converts a hexadecimal character to its integer value
/// Parses '0'-'9', 'A'-'F', and 'a'-'f' to 0-15
/// @param c The hexadecimal character
/// @param out Pointer to store the result (0-15)
/// @return true if c is a valid hex digit, false otherwise
/// @example
/// @code
/// int value;
/// hex_to_int('F', &value);  // Sets value=15, returns true
/// hex_to_int('G', &value);  // Returns false
/// @endcode
bool hex_to_int(char c, int *out);

/// @brief Converts an integer 0-15 to its hexadecimal character
/// Converts 0-9 to '0'-'9' and 10-15 to 'A'-'F'
/// @param i The integer value (0-15)
/// @return The hexadecimal character
/// @example
/// @code
/// char c = int_to_hex(15);  // Returns 'F'
/// char c2 = int_to_hex(10);  // Returns 'A'
/// @endcode
char int_to_hex(int i);

/// @}

#endif  // MATH_HPP_
