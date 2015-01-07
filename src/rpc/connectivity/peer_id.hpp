// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RPC_CONNECTIVITY_PEER_ID_HPP_
#define RPC_CONNECTIVITY_PEER_ID_HPP_

#include "containers/uuid.hpp"
#include "rpc/serialize_macros.hpp"

/* `peer_id_t` is a wrapper around a `uuid_u`. Each newly
created cluster node picks a UUID to be its peer-ID. */
class peer_id_t {
public:
    bool operator==(const peer_id_t &p) const {
        return p.uuid == uuid;
    }
    bool operator!=(const peer_id_t &p) const {
        return p.uuid != uuid;
    }

    /* This allows us to have maps keyed by `peer_id_t` */
    bool operator<(const peer_id_t &p) const {
        return p.uuid < uuid;
    }

    peer_id_t()
        : uuid(nil_uuid())
    { }

    explicit peer_id_t(uuid_u u) : uuid(u) { }

    uuid_u get_uuid() const {
        return uuid;
    }

    bool is_nil() const {
        return uuid.is_nil();
    }

    RDB_DECLARE_ME_SERIALIZABLE(peer_id_t);

private:
    uuid_u uuid;
};

void serialize_universal(write_message_t *wm, const peer_id_t &peer_id);
archive_result_t deserialize_universal(read_stream_t *s, peer_id_t *peer_id);

void debug_print(printf_buffer_t *buf, const peer_id_t &peer_id);

#endif /* RPC_CONNECTIVITY_PEER_ID_HPP_ */

