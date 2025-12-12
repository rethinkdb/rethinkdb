// Copyright 2010-2014 RethinkDB, all rights reserved.

/**
 * @file stl_utils.hpp
 * @brief Utility functions for working with STL containers.
 *
 * Provides helper functions for maps, sets, vectors, and other STL containers
 * to simplify common operations like extracting keys, checking membership,
 * debug printing, and constructing containers from variadic arguments.
 */

#ifndef STL_UTILS_HPP_
#define STL_UTILS_HPP_

#include <deque>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "errors.hpp"

class printf_buffer_t;

/**
 * @defgroup STLUtilities STL Container Utilities
 * @brief Helper functions for standard library containers
 */

/**
 * @ingroup STLUtilities
 * @brief Extract all keys from a map.
 *
 * @tparam K The key type of the map.
 * @tparam V The value type of the map.
 * @param map The map to extract keys from.
 * @return A set containing all keys from the map.
 *
 * Example:
 * @code
 * std::map<int, std::string> data = {{1, "a"}, {2, "b"}};
 * std::set<int> all_keys = keys(data);  // Contains {1, 2}
 * @endcode
 */
template <class K, class V>
std::set<K> keys(const std::map<K, V> &);

/**
 * @ingroup STLUtilities
 * @brief Check if a container contains a specific key.
 *
 * Works with associative containers (map, set, unordered_map, etc.)
 * that have a key_type typedef.
 *
 * @tparam container_t The container type.
 * @param container The container to search.
 * @param key The key to search for.
 * @return true if the key exists in the container, false otherwise.
 *
 * Example:
 * @code
 * std::set<std::string> options = {"verbose", "debug"};
 * if (std_contains(options, "verbose")) { // true
 *     std::cout << "Verbose mode enabled\n";
 * }
 * @endcode
 */
template <class container_t>
bool std_contains(const container_t &, const typename container_t::key_type &);

/**
 * @ingroup STLUtilities
 * @brief Debug print a map to a printf buffer.
 *
 * @tparam K The key type.
 * @tparam V The value type.
 * @tparam C The comparator type.
 * @param buf The printf buffer to write to.
 * @param map The map to print.
 */
template <class K, class V, class C>
void debug_print(printf_buffer_t *buf, const std::map<K, V, C> &map);

/**
 * @ingroup STLUtilities
 * @brief Debug print a set to a printf buffer.
 *
 * @tparam T The element type.
 * @param buf The printf buffer to write to.
 * @param map The set to print.
 */
template <class T>
void debug_print(printf_buffer_t *buf, const std::set<T> &map);

/**
 * @ingroup STLUtilities
 * @brief Debug print a vector to a printf buffer.
 *
 * @tparam T The element type.
 * @param buf The printf buffer to write to.
 * @param vec The vector to print.
 */
template <class T>
void debug_print(printf_buffer_t *buf, const std::vector<T> &vec);

/**
 * @ingroup STLUtilities
 * @brief Debug print a deque to a printf buffer.
 *
 * @tparam T The element type.
 * @param buf The printf buffer to write to.
 * @param vec The deque to print.
 */
template <class T>
void debug_print(printf_buffer_t *buf, const std::deque<T> &vec);

/**
 * @ingroup STLUtilities
 * @brief Debug print a pair to a printf buffer.
 *
 * @tparam T The first element type.
 * @tparam U The second element type.
 * @param buf The printf buffer to write to.
 * @param p The pair to print.
 */
template <class T, class U>
void debug_print(printf_buffer_t *buf, const std::pair<T, U> &p);

/**
 * @ingroup STLUtilities
 * @brief Construct a vector from variadic arguments.
 *
 * Creates a vector containing all provided arguments. The first argument
 * type determines the vector element type.
 *
 * @tparam T The element type (inferred from first argument).
 * @tparam Args The types of remaining arguments (must be convertible to T).
 * @param arg The first element.
 * @param args Remaining elements.
 * @return A vector containing all arguments in order.
 *
 * Example:
 * @code
 * auto vec = make_vector(1, 2, 3, 4, 5);
 * // Returns std::vector<int>{1, 2, 3, 4, 5}
 * @endcode
 */
template <class T, class... Args>
std::vector<T> make_vector(const T &arg, Args... args) {
    std::vector<T> ret;
    ret.reserve(sizeof...(args) + 1);
    ret.emplace_back(std::move(arg));
    UNUSED int dummy[] = { 0, (ret.emplace_back(std::move(args)), 1)... };
    return ret;
}

/**
 * @ingroup STLUtilities
 * @brief Construct a map from variadic pair arguments.
 *
 * Creates a map from key-value pairs provided as variadic arguments.
 *
 * @tparam K The key type.
 * @tparam V The value type.
 * @tparam Args The types of remaining pair arguments.
 * @param arg The first key-value pair.
 * @param args Remaining key-value pairs.
 * @return A map containing all provided pairs.
 *
 * Example:
 * @code
 * auto m = make_map(std::make_pair("a", 1),
 *                   std::make_pair("b", 2));
 * // Returns std::map<std::string, int>{{"a", 1}, {"b", 2}}
 * @endcode
 */
template <class K, class V, class... Args>
std::map<K, V> make_map(const std::pair<K, V> &arg, Args... args) {
    std::map<K, V> ret;
    ret.emplace(std::move(arg));
    UNUSED int dummy[] = { (ret.emplace(std::move(args)), 1)... };
    return ret;
}

/**
 * @ingroup STLUtilities
 * @brief Split a string by a separator character.
 *
 * Splits the input string on all occurrences of the separator and returns
 * a vector of substrings. Empty strings are included (adjacent separators
 * produce empty parts).
 *
 * @param s The string to split.
 * @param sep The separator character.
 * @return A vector of string parts.
 *
 * Example:
 * @code
 * auto parts = split_string("hello,world,test", ',');
 * // Returns {"hello", "world", "test"}
 * @endcode
 */
std::vector<std::string> split_string(const std::string &s, char sep);

#include "stl_utils.tcc"

#endif /* STL_UTILS_HPP_ */
