#ifndef __REPLICATION_NET_STRUCTS_HPP__
#define __REPLICATION_NET_STRUCTS_HPP__

#include <stdint.h>

#include "btree/serializer_config_block.hpp"
#include "btree/node.hpp"
#include "btree/leaf_node.hpp"

namespace replication {

enum multipart_aspect { SMALL = 0x81, FIRST = 0x82, MIDDLE = 0x83, LAST = 0x84 };

enum message_code { BACKFILL = 0x01, ANNOUNCE = 0x02, NOP = 0x03, ACK = 0x04, SHUTTING_DOWN = 0x05,
                    GOODBYE = 0x06,

                    SET = 0x21, APPEND = 0x22, PREPEND = 0x23, DELETE = 0x24 };

struct net_hello_t {
    char hello_magic[16];
    uint32_t replication_protocol_version;
    uint32_t role;
    database_magic_t database_magic;
    char informal_name[32];
} __attribute__((__packed__));

struct net_header_t {
    uint8_t message_multipart_aspect;
    uint8_t msgcode;
} __attribute__((__packed__));

struct net_backfill_t {
    net_header_t header;
    repli_timestamp timestamp;
} __attribute__((__packed__));

struct net_announce_t {
    net_header_t header;
    repli_timestamp from;
    repli_timestamp to;
} __attribute__((__packed__));

struct net_nop_t {
    net_header_t header;
    repli_timestamp timestamp;
} __attribute__((__packed__));

struct net_ack_t {
    net_header_t header;
    repli_timestamp timestamp;
} __attribute__((__packed__));

struct net_shutting_down_t {
    net_header_t header;
    repli_timestamp timestamp;
} __attribute__((__packed__));

struct net_goodbye_t {
    net_header_t header;
    repli_timestamp timestamp;
} __attribute__((__packed__));

// TODO: Add a uint16_t size field, create net_small_operation_t {
// header, size, timestamp }, create a btree_value that lacks the
// 8-bit size field.
struct net_small_set_t {
    net_header_t header;
    repli_timestamp timestamp;
    char btree_pair[];

    const btree_leaf_pair *leaf_pair() const { return reinterpret_cast<const btree_leaf_pair *>(btree_pair); }
    const btree_key *key() const { return &leaf_pair()->key; }
    const btree_value *value() const { return leaf_pair()->value(); }
} __attribute__((__packed__));

struct net_small_append_prepend_t {
    net_header_t header;
    uint16_t size;
    repli_timestamp timestamp;
    char data[];

    const btree_key *key() const { return reinterpret_cast<const btree_key *>(data); }
    const byte *value_beg() const { return reinterpret_cast<const byte *>(data + key()->full_size()); }
    const byte *value_end() const { return data + size; }
} __attribute__((__packed__));

struct net_size_header_t {
    net_header_t header;
    uint16_t size;
} __attribute__((__packed__));

struct net_large_operation_first_t {
    net_size_header_t header;
    repli_timestamp timestamp;
    uint32_t identifier;
} __attribute__((__packed__));

struct net_large_operation_middle_t {
    net_size_header_t header;
    uint32_t identifier;
} __attribute__((__packed__));

struct net_large_operation_last_t {
    net_size_header_t header;
    uint32_t identifier;
} __attribute__((__packed__));

struct net_large_set_t {
    net_large_operation_first_t op_header;
    metadata_flags_t metadata_flags;
    char key_value[];

    const btree_key *key() const { return reinterpret_cast<const btree_key *>(key_value); }
    const char *metadata() const { return key_value + 1 + key()->size; }

    // The size needed before we can call fitsize().
    static size_t prefitsize() { return sizeof(net_large_set_t) + 1; }

    // fitsize is the offset at which the value begins.
    size_t fitsize() const { return sizeof(net_large_set_t) + 1 + key()->size + metadata_size(metadata_flags); }
} __attribute__((__packed__));

struct net_large_append_prepend_t {
    net_large_operation_first_t op_header;
    char data[];
    const btree_key *key() const { return reinterpret_cast<const btree_key *>(data); }

    static size_t prefitsize() { return sizeof(net_large_append_prepend_t) + 1; }
    size_t fitsize() const { return sizeof(net_large_set_t) + 1 + key()->size; }
} __attribute__((__packed__));


}  // namespace replication

#endif  // __REPLICATION_NET_STRUCTS_HPP__
