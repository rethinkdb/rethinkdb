// Copyright 2010-2014 RethinkDB, all rights reserved.

/**
 * @file stl_utils.hpp
 * @brief Template utilities for working with STL containers.
 *
 * This header provides convenient template functions for working with standard
 * library containers, including key extraction, membership testing, debug printing,
 * and container construction utilities.
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
 * @defgroup STLContainerUtils STL Container Utilities
 * @brief Template utilities for working with STL containers.
 */

/**
 * @ingroup STLContainerUtils
 * @brief Extracts all keys from a map into a set.
 *
 * Creates a new std::set containing all keys from the provided map.
 * This is useful for operations that need to work with the key set rather
 * than the entire map.
 *
 * @tparam K The key type of the map.
 * @tparam V The value type of the map.
 * @param m The source map to extract keys from.
 * @return A std::set containing all keys from the input map.
 *
 * @code
 * std::map<std::string, int> age;
 * age["Alice"] = 30;
 * age["Bob"] = 25;
 * age["Charlie"] = 35;
 *
 * auto names = keys(age);
 * // names contains {"Alice", "Bob", "Charlie"}
 * @endcode
 */
template <class K, class V>
std::set<K> keys(const std::map<K, V> &);

/**
 * @ingroup STLContainerUtils
 * @brief Tests whether a container contains a specific key.
 *
 * Provides a consistent interface for membership testing across different
 * STL container types that support the count() or find() method.
 *
 * @tparam container_t The type of container to search.
 * @param c The container to search.
 * @param key The key to search for (of type container_t::key_type).
 * @return true if the key exists in the container, false otherwise.
 *
 * @code
 * std::set<int> numbers = {1, 2, 3, 4, 5};
 * if (std_contains(numbers, 3)) {
 *     // 3 is in the set
 * }
 *
 * std::map<std::string, int> config;
 * config["debug"] = 1;
 * if (std_contains(config, "debug")) {
 *     // "debug" key exists
 * }
 * @endcode
 */
template <class container_t>
bool std_contains(const container_t &, const typename container_t::key_type &);

/**
 * @ingroup STLContainerUtils
 * @brief Pretty-prints a map to a printf_buffer_t.
 *
 * Formats a map in a human-readable way suitable for debugging output.
 * The output format displays key-value pairs in a convenient format.
 *
 * @tparam K The key type of the map.
 * @tparam V The value type of the map.
 * @tparam C The comparator type of the map.
 * @param buf The printf_buffer_t to write output to.
 * @param map The map to print.
 */
template <class K, class V, class C>
void debug_print(printf_buffer_t *buf, const std::map<K, V, C> &map);

/**
 * @ingroup STLContainerUtils
 * @brief Pretty-prints a set to a printf_buffer_t.
 *
 * @tparam T The element type of the set.
 * @param buf The printf_buffer_t to write output to.
 * @param map The set to print.
 */
template <class T>
void debug_print(printf_buffer_t *buf, const std::set<T> &map);

/**
 * @ingroup STLContainerUtils
 * @brief Pretty-prints a vector to a printf_buffer_t.
 *
 * Formats a vector in a human-readable way suitable for debugging output.
 *
 * @tparam T The element type of the vector.
 * @param buf The printf_buffer_t to write output to.
 * @param vec The vector to print.
 *
 * @code
 * std::vector<int> nums = {1, 2, 3, 4, 5};
 * debug_print(buf, nums);
 * // buf contains something like "[1, 2, 3, 4, 5]"
 * @endcode
 */
template <class T>
void debug_print(printf_buffer_t *buf, const std::vector<T> &vec);

/**
 * @ingroup STLContainerUtils
 * @brief Pretty-prints a deque to a printf_buffer_t.
 *
 * @tparam T The element type of the deque.
 * @param buf The printf_buffer_t to write output to.
 * @param vec The deque to print.
 */
template <class T>
void debug_print(printf_buffer_t *buf, const std::deque<T> &vec);

/**
 * @ingroup STLContainerUtils
 * @brief Pretty-prints a pair to a printf_buffer_t.
 *
 * @tparam T The type of the first element.
 * @tparam U The type of the second element.
 * @param buf The printf_buffer_t to write output to.
 * @param p The pair to print.
 */
template <class T, class U>
void debug_print(printf_buffer_t *buf, const std::pair<T, U> &p);

/**
 * @ingroup STLContainerUtils
 * @brief Constructs a vector from variadic arguments.
 *
 * A convenient helper function to create a vector with initial values
 * in a single expression. The type is inferred from the first argument.
 *
 * @tparam T The element type for the vector.
 * @tparam Args The types of the remaining arguments (must be compatible with T).
 * @param arg The first element to add to the vector.
 * @param args Additional elements to add to the vector.
 * @return A std::vector<T> containing all provided arguments.
 *
 * @code
 * auto nums = make_vector(1, 2, 3, 4, 5);
 * // nums is std::vector<int> with elements {1, 2, 3, 4, 5}
 *
 * auto strs = make_vector(std::string("hello"), std::string("world"));
 * // strs is std::vector<std::string> with elements {"hello", "world"}
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
 * @ingroup STLContainerUtils
 * @brief Constructs a map from variadic pair arguments.
 *
 * A convenient helper function to create a map with initial key-value pairs
 * in a single expression. The key and value types are inferred from the first pair.
 *
 * @tparam K The key type for the map.
 * @tparam V The value type for the map.
 * @tparam Args The types of the remaining pair arguments.
 * @param arg The first key-value pair to add to the map.
 * @param args Additional key-value pairs to add to the map.
 * @return A std::map<K, V> containing all provided pairs.
 *
 * @code
 * auto config = make_map(
 *     std::make_pair("debug", 1),
 *     std::make_pair("verbose", 0),
 *     std::make_pair("timeout", 30)
 * );
 * // config contains three key-value pairs
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
 * @ingroup STLContainerUtils
 * @brief Splits a string by a separator character.
 *
 * Partitions a string into substrings separated by a given character.
 * Empty substrings are included in the result.
 *
 * @param s The string to split.
 * @param sep The separator character.
 * @return A std::vector<std::string> containing the split substrings.
 *
 * @code
 * auto parts = split_string("hello,world,test", ',');
 * // parts contains {"hello", "world", "test"}
 *
 * auto paths = split_string("/usr/local/bin", '/');
 * // paths contains {"", "usr", "local", "bin"}
 * @endcode
 */
std::vector<std::string> split_string(const std::string &s, char sep);

#include "stl_utils.tcc"

#endif /* STL_UTILS_HPP_ */
