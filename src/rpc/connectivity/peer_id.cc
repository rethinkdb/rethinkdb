// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rpc/connectivity/peer_id.hpp"

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(peer_id_t, uuid);

// Universal serialization functions: you MUST NOT change their implementations.
// (You could find a way to remove these functions, though.)
void serialize_universal(write_message_t *wm, const peer_id_t &peer_id) {
    serialize_universal(wm, peer_id.get_uuid());
}
archive_result_t deserialize_universal(read_stream_t *s, peer_id_t *peer_id) {
    uuid_u uuid;
    archive_result_t res = deserialize_universal(s, &uuid);
    if (bad(res)) { return res; }
    *peer_id = peer_id_t(uuid);
    return archive_result_t::SUCCESS;
}

void debug_print(printf_buffer_t *buf, const peer_id_t &peer_id) {
    debug_print(buf, peer_id.get_uuid());
}

