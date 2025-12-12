// Copyright 2010-2013 RethinkDB, all rights reserved.

/// @file auth_key.hpp
/// @brief Authentication key management with length constraints
///
/// Provides a validated authentication key type that enforces maximum length
/// constraints and provides timing-safe comparison to prevent timing attacks.
///
/// @defgroup Authentication Authentication Infrastructure
/// Types and utilities for authentication and authorization
/// @{

#ifndef CONTAINERS_AUTH_KEY_HPP_
#define CONTAINERS_AUTH_KEY_HPP_

#include <string>

#include "rpc/serialize_macros.hpp"

/// @brief Authentication key with length validation and timing-safe comparison
///
/// `auth_key_t` represents a cluster authentication key. It enforces a maximum
/// length and provides timing-safe comparison to prevent timing-based attacks
/// that could reveal information about the key through measurement of comparison time.
///
/// The key is stored as a string but all equality operations use timing-safe
/// comparison to prevent side-channel attacks.
///
/// @example
/// @code
/// auth_key_t key;
/// if (key.assign_value("my-secret-key-12345")) {
///     // Key was valid (< 2048 characters)
///     if (key == other_key) {
///         // Keys match (timing-safe comparison)
///     }
/// } else {
///     // Key was too long
///     std::cerr << "Key must be <= " << auth_key_t::max_length << " characters\n";
/// }
/// @endcode
class auth_key_t {
public:
    /// @brief Default constructor initializes to empty string
    auth_key_t();

    /// @brief Sets the authentication key value with validation
    /// Validates that the new key doesn't exceed max_length.
    /// @param new_key The authentication key to assign
    /// @return true if the key was valid and assigned, false if too long
    MUST_USE bool assign_value(const std::string &new_key);

    /// @brief Gets the authentication key string
    /// @return Reference to the stored key string
    const std::string &str() const { return key; }

    /// @brief Maximum allowed length for authentication keys
    static const int32_t max_length = 2048;

    /// @brief Declares this class as serializable
    /// Allows auth_key_t to be serialized/deserialized in RPC messages
    RDB_DECLARE_ME_SERIALIZABLE(auth_key_t);

private:
    std::string key;  ///< The actual key string (max 2048 characters)
};

/// @brief Timing-safe equality comparison for authentication keys
/// Uses constant-time comparison to prevent timing attacks.
/// @param x First key to compare
/// @param y Second key to compare
/// @return true if both keys are identical, false otherwise
bool timing_sensitive_equals(const auth_key_t &x, const auth_key_t &y);

/// @brief Equality operator using timing-safe comparison
/// @param x First key
/// @param y Second key
/// @return true if keys are equal
inline bool operator==(const auth_key_t &x, const auth_key_t &y) {
    // Might as well use timing_sensitive_equals this in case of accidental misuse.
    return timing_sensitive_equals(x, y);
}

/// @brief Inequality operator using timing-safe comparison
/// @param x First key
/// @param y Second key
/// @return true if keys differ
inline bool operator!=(const auth_key_t &x, const auth_key_t &y) {
    return !(x == y);
}

/// @brief Less-than operator for use in containers
/// @param x First key
/// @param y Second key
/// @return true if x < y lexicographically
inline bool operator<(const auth_key_t &x, const auth_key_t &y) {
    return x.str() < y.str();
}

/// @}

#endif  // CONTAINERS_AUTH_KEY_HPP_
