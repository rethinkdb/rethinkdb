// Copyright 2010-2016 RethinkDB, all rights reserved.

/// @file server_id.hpp
/// @brief Server identification with proxy vs. regular server distinction
///
/// Provides unique identifiers for RethinkDB servers, distinguishing between
/// proxy servers (clients that don't store data) and regular servers (that store data).
/// Server IDs are based on UUIDs with an additional flag for proxy designation.
///
/// @defgroup ServerIdentification Server and Peer Identification
/// Types for identifying servers and peers in the cluster
/// @{

#ifndef RPC_CONNECTIVITY_SERVER_ID_HPP_
#define RPC_CONNECTIVITY_SERVER_ID_HPP_

#include <string>

#include "containers/uuid.hpp"
#include "rpc/serialize_macros.hpp"

/// @brief Unique identifier for a server with proxy flag
///
/// `server_id_t` represents a unique server in the RethinkDB cluster. It combines
/// a UUID with a flag indicating whether the server is a proxy (client that doesn't
/// store data) or a regular server (stores data and participates in sharding).
///
/// For backwards compatibility, server_id_t serializes as just a uuid_u, with the
/// proxy flag determined by some other means (e.g., server configuration).
///
/// @example
/// @code
/// // Generate a new regular server ID
/// server_id_t storage_server = server_id_t::generate_server_id();
///
/// // Generate a new proxy server ID
/// server_id_t proxy_server = server_id_t::generate_proxy_id();
///
/// // Check server type
/// if (server.is_proxy()) {
///     std::cout << "Proxy server: " << server.print() << "\n";
/// } else {
///     std::cout << "Storage server: " << server.print() << "\n";
/// }
/// @endcode
///
/// @note For backwards compatibility, only the UUID portion is serialized
/// @note Ordering puts proxies before regular servers for consistent sorting
class server_id_t {
public:
    /// @brief Generates a unique ID for a proxy server
    /// Creates a new server_id_t with proxy_flag = true and random UUID
    /// @return A newly generated proxy server ID
    static server_id_t generate_proxy_id();

    /// @brief Generates a unique ID for a regular server
    /// Creates a new server_id_t with proxy_flag = false and random UUID
    /// @return A newly generated server ID
    static server_id_t generate_server_id();

    /// @brief Creates a server_id_t from a UUID, marking it as a regular server
    /// @param _uuid The UUID to use as the server identifier
    /// @return A server_id_t with proxy_flag = false
    static server_id_t from_server_uuid(uuid_u _uuid);

    /// @brief Creates a server_id_t from a UUID, marking it as a proxy server
    /// @param _uuid The UUID to use as the server identifier
    /// @return A server_id_t with proxy_flag = true
    static server_id_t from_proxy_uuid(uuid_u _uuid);

    /// @brief Default constructor for deserialization
    /// Creates an uninitialized server_id_t. Used when deserializing from network.
    server_id_t() { }

    /// @brief Less-than operator for sorting and container use
    /// Orders proxies before regular servers, then by UUID
    /// @param p The server to compare with
    /// @return true if this < p
    bool operator<(const server_id_t &p) const {
        return (proxy_flag && !p.proxy_flag)
            || (proxy_flag == p.proxy_flag && p.uuid < uuid);
    }

    /// @brief Equality operator
    /// @param p The server to compare with
    /// @return true if both the UUID and proxy flag match
    bool operator==(const server_id_t &p) const {
        return p.proxy_flag == proxy_flag && p.uuid == uuid;
    }

    /// @brief Inequality operator
    /// @param p The server to compare with
    /// @return true if UUID or proxy flag differs
    bool operator!=(const server_id_t &p) const {
        return !(p == *this);
    }

    /// @brief Gets the underlying UUID
    /// @return The UUID portion of this server ID
    uuid_u get_uuid() const {
        return uuid;
    }

    /// @brief Checks if this is a proxy server
    /// @return true if this is a proxy, false if regular storage server
    bool is_proxy() const {
        return proxy_flag;
    }

    /// @brief Gets a human-readable string representation
    /// @return String representation of the server ID (e.g., for logging)
    std::string print() const;

    /// @brief Declares this class as serializable
    /// Allows server_id_t to be serialized/deserialized in RPC messages
    RDB_DECLARE_ME_SERIALIZABLE(server_id_t);

private:
    uuid_u uuid;        ///< The unique UUID identifying this server
    bool proxy_flag;    ///< true if this is a proxy server, false if regular storage server
};

/// @brief Parses a server ID from its string representation
/// Inverse operation of server_id_t::print()
/// @param in The string representation of a server ID
/// @param out Pointer to receive the parsed server_id_t
/// @return true if parsing succeeded, false if format was invalid
bool str_to_server_id(const std::string &in, server_id_t *out);

/// @brief Serializes a server_id_t to a write message
/// Custom serialization handling for network transmission
/// @param wm The write message to serialize into
/// @param server_id The server ID to serialize
void serialize_universal(write_message_t *wm, const server_id_t &server_id);

/// @brief Deserializes a server_id_t from a read stream
/// Custom deserialization handling for network reception
/// @param s The read stream to deserialize from
/// @param server_id Pointer to receive the deserialized server ID
/// @return Success/failure status of deserialization
archive_result_t deserialize_universal(read_stream_t *s, server_id_t *server_id);

/// @brief Writes server ID representation to a printf buffer
/// Formats the server ID in human-readable form
/// @param buf The output buffer to write to
/// @param server_id The server ID to format
void debug_print(printf_buffer_t *buf, const server_id_t &server_id);

/// @}

#endif /* RPC_CONNECTIVITY_SERVER_ID_HPP_ */

