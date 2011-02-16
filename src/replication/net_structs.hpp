#ifndef __REPLICATION_NET_STRUCTS_HPP__
#define __REPLICATION_NET_STRUCTS_HPP__

#include <stdint.h>

#include "btree/value.hpp"
#include "serializer/translator.hpp"

namespace replication {

enum multipart_aspect { SMALL = 0x81, FIRST = 0x82, MIDDLE = 0x83, LAST = 0x84 };

enum message_code { MSGCODE_NIL = 0, BACKFILL = 0x01, ANNOUNCE = 0x02, NOP = 0x03, ACK = 0x04, SHUTTING_DOWN = 0x05,
                    GOODBYE = 0x06,

                    GET_CAS = 0x21, SARC, INCR, DECR, APPEND, PREPEND, DELETE };

enum role_enum_t { role_master = 0, role_new_slave = 1, role_slave = 2 };



struct net_castime_t {
    cas_t proposed_cas;
    repli_timestamp timestamp;
} __attribute__((__packed__));

struct net_hello_t {
    char hello_magic[16];
    uint32_t replication_protocol_version;
    uint32_t role;
    creation_timestamp_t database_creation_timestamp;
    char informal_name[32];
} __attribute__((__packed__));

struct net_header_t {
    uint8_t message_multipart_aspect;
    uint8_t msgcode;
    uint16_t msgsize;
} __attribute__((__packed__));

struct net_multipart_header_t {
    uint8_t message_multipart_aspect;
    uint8_t msgcode;
    uint16_t msgsize;
    uint32_t ident;
} __attribute__((__packed__));

struct net_backfill_t {
    repli_timestamp timestamp;
} __attribute__((__packed__));

struct net_announce_t {
    repli_timestamp from;
    repli_timestamp to;
} __attribute__((__packed__));

struct net_nop_t {
    repli_timestamp timestamp;
} __attribute__((__packed__));

struct net_ack_t {
    repli_timestamp timestamp;
} __attribute__((__packed__));

struct net_shutting_down_t {
    repli_timestamp timestamp;
} __attribute__((__packed__));

struct net_goodbye_t {
    repli_timestamp timestamp;
} __attribute__((__packed__));


struct net_get_cas_t {
    cas_t proposed_cas;
    repli_timestamp timestamp;
    uint16_t key_size;
    char key[];
} __attribute__((__packed__));

// TODO: Make this structure more efficient, use optional fields for
// flags, exptime, add_policy, old_cas.
struct net_sarc_t {
    repli_timestamp timestamp;
    cas_t proposed_cas;
    mcflags_t flags;
    exptime_t exptime;
    uint16_t key_size;
    uint32_t value_size;
    uint8_t add_policy;
    uint8_t replace_policy;
    cas_t old_cas;
    char keyvalue[];
} __attribute__((__packed__));

struct net_incr_t {
    repli_timestamp timestamp;
    cas_t proposed_cas;
    uint64_t amount;
    uint16_t key_size;
    char key[];
} __attribute__((__packed__));

struct net_decr_t {
    repli_timestamp timestamp;
    cas_t proposed_cas;
    uint64_t amount;
    uint16_t key_size;
    char key[];
} __attribute__((__packed__));

struct net_append_t {
    repli_timestamp timestamp;
    cas_t proposed_cas;
    uint16_t key_size;
    uint32_t value_size;

    // The first key_size bytes are for the key, the next value_size
    // bytes (possibly spanning multiple messages) are for the value.
    char keyvalue[];
} __attribute__((__packed__));

struct net_prepend_t {
    repli_timestamp timestamp;
    cas_t proposed_cas;
    uint16_t key_size;
    uint32_t value_size;

    // The first key_size bytes are for the key, the next value_size
    // bytes (possibly spanning multiple messages) are for the value.
    char keyvalue[];
} __attribute__((__packed__));

struct net_delete_t {
    repli_timestamp timestamp;
    uint16_t key_size;
    char key[];
} __attribute__((__packed__));

template <class T>
struct headed {
    net_header_t hdr;
    T data;
} __attribute__((__packed__));

template <class T>
struct multipart_headed {
    net_multipart_header_t hdr;
    T data;
} __attribute__((__packed__));

}  // namespace replication

#endif  // __REPLICATION_NET_STRUCTS_HPP__
