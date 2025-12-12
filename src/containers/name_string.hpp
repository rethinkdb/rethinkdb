// Copyright 2010-2013 RethinkDB, all rights reserved.

/// @file name_string.hpp
/// @brief Validated string type for identifiers and names
///
/// Provides a string type that enforces semantic constraints - only valid
/// non-empty names are allowed. Used throughout RethinkDB for database names,
/// table names, and other identifiers where only valid naming conventions
/// are acceptable.
///
/// @defgroup NameTypes Name and Identifier Types
/// Validated string types for identifiers and names
/// @{

#ifndef CONTAINERS_NAME_STRING_HPP_
#define CONTAINERS_NAME_STRING_HPP_

#include <string>

#include "rpc/serialize_macros.hpp"

class datum_string_t;
class printf_buffer_t;

/// @brief Validated string type for database and table names
///
/// `name_string_t` represents a validated name that:
/// - Can only be a non-empty string (empty assignment fails)
/// - Conforms to valid naming conventions for identifiers
/// - Can be serialized for RPC communication
///
/// The empty string is represented as an "unset" name. Use assign_value()
/// to validate and assign a name, or guarantee_valid() for hard-coded compile-time names.
///
/// @example
/// @code
/// name_string_t db_name;
/// if (db_name.assign_value("my_database")) {
///     // Valid name assigned
///     std::cout << "Database: " << db_name.str() << "\n";
/// } else {
///     // Invalid name (empty or contains invalid characters)
///     std::cerr << "Invalid database name\n";
/// }
///
/// // For compile-time constant names, use guarantee_valid()
/// auto table_name = name_string_t::guarantee_valid("users");
/// @endcode
///
/// @note The empty string represents "unset" state and is always valid internally
/// @note To check if a name is set, use !str().empty()
class name_string_t {
public:
    /// @brief Default constructor initializes to empty string
    /// Represents an unset name
    name_string_t();

    /// @brief Assigns a value with validation
    /// Validates that the string is a valid identifier.
    /// Empty strings fail validation (but empty() state is still valid internally).
    /// @param s The string value to assign
    /// @return true if assignment succeeded, false if invalid
    MUST_USE bool assign_value(const std::string &s);

    /// @brief Assigns from a datum_string_t with validation
    /// Converts and validates a datum_string_t value.
    /// @param s The datum_string_t to assign from
    /// @return true if assignment succeeded, false if invalid
    MUST_USE bool assign_value(const datum_string_t &s);

    /// @brief Creates a guaranteed-valid name from a compile-time constant
    /// Use this for hard-coded names that are known to be valid at compile time.
    /// Bypasses validation - use only for literal strings.
    /// @param name A C-string literal that is known to be valid
    /// @return A name_string_t with the given value
    /// @note Only use this with string literals, not user input
    static name_string_t guarantee_valid(const char *name);

    /// @brief Gets the string value
    /// @return Reference to the underlying string
    const std::string& str() const { return str_; }

    /// @brief Checks if the name is empty
    /// @return true if this represents an unset name
    /// @note Consider using str().empty() instead (marked as untrustworthy)
    bool empty() const { return str_.empty(); }

    /// @brief Gets C-string pointer for compatibility
    /// @return C-string pointer to the underlying string
    const char *c_str() const { return str_.c_str(); }

    /// @brief Declares this class as serializable
    /// Allows name_string_t to be serialized/deserialized in RPC messages
    RDB_DECLARE_ME_SERIALIZABLE(name_string_t);

    /// @brief Message describing valid character requirements
    /// Explains which characters are allowed in names
    static const char *const valid_char_msg;

private:
    std::string str_;  ///< The underlying string value
};

/// @brief Equality operator for names
/// @param x First name
/// @param y Second name
/// @return true if both names are identical
inline bool operator==(const name_string_t& x, const name_string_t& y) {
    return x.str() == y.str();
}

/// @brief Inequality operator for names
/// @param x First name
/// @param y Second name
/// @return true if names differ
inline bool operator!=(const name_string_t& x, const name_string_t& y) {
    return !(x == y);
}

/// @brief Less-than operator for use in containers
/// @param x First name
/// @param y Second name
/// @return true if x < y lexicographically
inline bool operator<(const name_string_t& x, const name_string_t& y) {
    return x.str() < y.str();
}

/// @brief Writes name representation to a printf buffer
/// Formats the name in human-readable form
/// @param buf The output buffer to write to
/// @param s The name_string_t to format
void debug_print(printf_buffer_t *buf, const name_string_t& s);

/// @}

#endif  // CONTAINERS_NAME_STRING_HPP_
