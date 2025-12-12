// Copyright 2010-2013 RethinkDB, all rights reserved.

/// @file uuid.hpp
/// @brief Universally unique identifier type and utilities
///
/// Provides a standard UUID implementation used throughout RethinkDB for
/// uniquely identifying namespaces, databases, clusters, and other entities.
/// All UUIDs are stored in network byte order.
///
/// @defgroup UUIDTypes UUID and Unique Identifier Management
/// Utilities for generating and managing unique identifiers
/// @{

#ifndef CONTAINERS_UUID_HPP_
#define CONTAINERS_UUID_HPP_

#include <string.h>
#include <stdint.h>

#include <string>

#include "errors.hpp"

class printf_buffer_t;

/// @brief Universally unique identifier (UUID) type
///
/// `uuid_u` represents a 128-bit universally unique identifier stored in
/// network byte order. UUIDs are used throughout RethinkDB to uniquely identify
/// namespaces, databases, tables, clusters, and other entities.
///
/// The name `uuid_u` is used to avoid conflicts with `uuid_t` defined on Darwin.
///
/// @note All UUIDs are maintained in network byte order internally
/// @example
/// @code
/// // Generate a random UUID
/// uuid_u my_id = generate_uuid();
///
/// // Check if valid
/// if (!my_id.is_nil() && !my_id.is_unset()) {
///     // Use the UUID...
/// }
///
/// // Convert to/from string representation
/// std::string id_string = uuid_to_str(my_id);
/// uuid_u parsed_id = str_to_uuid(id_string);
/// @endcode
class uuid_u {
public:
    /// @brief Default constructor creates an uninitialized UUID
    uuid_u();

    /// @brief Checks if this UUID is unset (default-constructed)
    /// @return true if this is an uninitialized UUID
    bool is_unset() const;

    /// @brief Checks if this UUID is the nil UUID (all zeros)
    /// @return true if this is the nil UUID
    bool is_nil() const;

    /// @brief Size of a UUID in bytes
    static const size_t kStaticSize = 16;
    /// @brief Size of a UUID as hex string with hyphens
    /// Two hex chars per byte (16 bytes = 32 chars) + 4 hyphens = 36 chars
    static const size_t kStringSize = 2 * kStaticSize + 4;

    /// @brief Gets the static size of a UUID
    /// Used for compile-time validation
    /// @return The size of uuid_u in bytes (always 16)
    static size_t static_size() {
        CT_ASSERT(sizeof(uuid_u) == kStaticSize);
        return kStaticSize;
    }

    /// @brief Gets mutable access to the underlying UUID bytes
    /// @return Pointer to the 16-byte UUID data
    uint8_t *data() { return data_; }

    /// @brief Gets const access to the underlying UUID bytes
    /// @return Const pointer to the 16-byte UUID data
    const uint8_t *data() const { return data_; }

    /// @brief Generates a UUID derived from a base UUID and name
    /// Creates a deterministic UUID based on a namespace and name,
    /// useful for creating consistent identifiers within a system.
    /// @param base The base/namespace UUID
    /// @param name The name to derive from
    /// @return A new UUID derived from the namespace and name
    static uuid_u from_hash(const uuid_u &base, const std::string &name);

private:
    uint8_t data_[kStaticSize];  ///< 128-bit UUID stored in network byte order
};

/// @brief Equality operator for UUIDs
/// @param x First UUID to compare
/// @param y Second UUID to compare
/// @return true if both UUIDs are identical
bool operator==(const uuid_u& x, const uuid_u& y);

/// @brief Inequality operator for UUIDs
/// @param x First UUID to compare
/// @param y Second UUID to compare
/// @return true if UUIDs differ
inline bool operator!=(const uuid_u& x, const uuid_u& y) { return !(x == y); }

/// @brief Less-than operator for UUIDs
/// Provides ordering for use in containers like std::map
/// @param x First UUID
/// @param y Second UUID
/// @return true if x < y in byte order
bool operator<(const uuid_u& x, const uuid_u& y);

/// @brief Generates a random UUID
/// Creates a cryptographically suitable random UUID, similar to
/// boost::uuids::random_generator()() but without Valgrind warnings.
/// @return A newly generated random UUID
uuid_u generate_uuid();

/// @brief Returns the nil UUID (all zeros)
/// @return The nil UUID (128 zero bits)
uuid_u nil_uuid();

/// @brief Writes UUID representation to a printf buffer
/// Formats the UUID in human-readable hex-with-hyphens format
/// @param buf The output buffer to write to
/// @param id The UUID to format
void debug_print(printf_buffer_t *buf, const uuid_u& id);

/// @brief Converts a UUID to its string representation
/// Converts to hexadecimal format with hyphens: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
/// @param id The UUID to convert
/// @return String representation of the UUID
std::string uuid_to_str(uuid_u id);

/// @brief Converts a string to a UUID (throwing version)
/// Parses a string in hexadecimal-with-hyphens format into a UUID.
/// @param str The string to parse
/// @return The parsed UUID
/// @throw May throw if string format is invalid
uuid_u str_to_uuid(const std::string &str);

/// @brief Converts a string to a UUID (non-throwing version)
/// Parses a string in hexadecimal-with-hyphens format into a UUID.
/// @param str The string to parse
/// @param out Pointer to receive the parsed UUID
/// @return false if parsing failed, true on success
MUST_USE bool str_to_uuid(const std::string &str, uuid_u *out);

/// @defgroup UUIDTypeAliases UUID Type Aliases
/// Semantic type aliases for UUID used in different contexts
/// @{
typedef uuid_u namespace_id_t;      ///< Identifier for a namespace
typedef uuid_u database_id_t;       ///< Identifier for a database
typedef uuid_u backfill_session_id_t;  ///< Identifier for a backfill session
typedef uuid_u branch_id_t;         ///< Identifier for a version branch
typedef uuid_u issue_id_t;          ///< Identifier for an issue
/// @}

/// @}

#endif  // CONTAINERS_UUID_HPP_
