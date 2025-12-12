// Copyright 2010-2014 RethinkDB, all rights reserved.

/// @file peer_id.hpp
/// @brief Peer identification for cluster nodes and connections
///
/// Provides a wrapper around UUID for identifying individual cluster nodes
/// and peers in the RethinkDB network. Each peer in the cluster selects its
/// own peer ID (UUID) for identification in network communications.
///
/// @defgroup ServerIdentification Server and Peer Identification
/// @{

#ifndef RPC_CONNECTIVITY_PEER_ID_HPP_
#define RPC_CONNECTIVITY_PEER_ID_HPP_

#include "containers/uuid.hpp"
#include "rpc/serialize_macros.hpp"

/// @brief Unique identifier for a peer in the cluster
///
/// `peer_id_t` wraps a UUID to uniquely identify a peer (node) in the RethinkDB
/// cluster. Each newly created cluster node generates its own UUID and uses it
/// as its peer ID for network communications.
///
/// This type is commonly used in maps and sets for tracking peers and their state.
///
/// @example
/// @code
/// // Each peer generates its own ID on startup
/// peer_id_t local_peer(generate_uuid());
///
/// // Compare peer IDs
/// if (remote_peer == local_peer) {
///     // Same peer
/// }
///
/// // Use in containers (supports < operator)
/// std::map<peer_id_t, peer_info_t> peer_registry;
/// peer_registry[local_peer] = my_info;
///
/// // Check for nil/uninitialized peers
/// if (!peer.is_nil()) {
///     // Peer is properly initialized
/// }
/// @endcode
///
/// @note The nil UUID (all zeros) is used to represent uninitialized peers
/// @note Supports use as map keys via operator<()
class peer_id_t {
public:
    /// @brief Equality operator
    /// @param p The peer to compare with
    /// @return true if both peers have the same UUID
    bool operator==(const peer_id_t &p) const {
        return p.uuid == uuid;
    }

    /// @brief Inequality operator
    /// @param p The peer to compare with
    /// @return true if peers have different UUIDs
    bool operator!=(const peer_id_t &p) const {
        return p.uuid != uuid;
    }

    /// @brief Less-than operator for sorting and container use
    /// Allows use of peer_id_t in std::map and std::set
    /// @param p The peer to compare with
    /// @return true if this < p
    bool operator<(const peer_id_t &p) const {
        return p.uuid < uuid;
    }

    /// @brief Default constructor initializes to nil UUID
    /// Creates an uninitialized peer ID
    peer_id_t()
        : uuid(nil_uuid())
    { }

    /// @brief Constructor with explicit UUID
    /// @param u The UUID to use as this peer's identifier
    explicit peer_id_t(uuid_u u) : uuid(u) { }

    /// @brief Gets the underlying UUID
    /// @return The UUID identifying this peer
    uuid_u get_uuid() const {
        return uuid;
    }

    /// @brief Checks if this is the nil UUID
    /// @return true if this peer is uninitialized (nil UUID)
    bool is_nil() const {
        return uuid.is_nil();
    }

    /// @brief Declares this class as serializable
    /// Allows peer_id_t to be serialized/deserialized in RPC messages
    RDB_DECLARE_ME_SERIALIZABLE(peer_id_t);

private:
    uuid_u uuid;  ///< The UUID identifying this peer
};

/// @brief Serializes a peer_id_t to a write message
/// Custom serialization handling for network transmission
/// @param wm The write message to serialize into
/// @param peer_id The peer ID to serialize
void serialize_universal(write_message_t *wm, const peer_id_t &peer_id);

/// @brief Deserializes a peer_id_t from a read stream
/// Custom deserialization handling for network reception
/// @param s The read stream to deserialize from
/// @param peer_id Pointer to receive the deserialized peer ID
/// @return Success/failure status of deserialization
archive_result_t deserialize_universal(read_stream_t *s, peer_id_t *peer_id);

/// @brief Writes peer ID representation to a printf buffer
/// Formats the peer ID in human-readable form
/// @param buf The output buffer to write to
/// @param peer_id The peer ID to format
void debug_print(printf_buffer_t *buf, const peer_id_t &peer_id);

/// @}

#endif /* RPC_CONNECTIVITY_PEER_ID_HPP_ */

