// Copyright 2010-2016 RethinkDB, all rights reserved.
#include "rpc/connectivity/server_id.hpp"

server_id_t server_id_t::generate_proxy_id() {
    server_id_t res;
    res.uuid = generate_uuid();
    res.proxy_flag = true;
    return res;
}

server_id_t server_id_t::generate_server_id() {
    server_id_t res;
    res.uuid = generate_uuid();
    res.proxy_flag = false;
    return res;
}

server_id_t server_id_t::from_server_uuid(uuid_u _uuid) {
    server_id_t res;
    res.uuid = _uuid;
    res.proxy_flag = false;
    return res;
}

server_id_t server_id_t::from_proxy_uuid(uuid_u _uuid) {
    server_id_t res;
    res.uuid = _uuid;
    res.proxy_flag = true;
    return res;
}

std::string server_id_t::print() const {
    if (is_proxy()) {
        return "proxy-" + uuid_to_str(uuid);
    } else {
        return uuid_to_str(uuid);
    }
}

bool str_to_server_id(const std::string &in, server_id_t *out) {
    bool is_proxy;
    std::string uuid_str;
    if (in.length() >= 6 && in.substr(0, 6) == "proxy-") {
        is_proxy = true;
        uuid_str = in.substr(6);
    } else {
        is_proxy = false;
        uuid_str = in;
    }
    uuid_u uuid;
    if (!str_to_uuid(uuid_str, &uuid)) {
        return false;
    }
    if (is_proxy) {
        *out = server_id_t::from_proxy_uuid(uuid);
    } else {
        *out = server_id_t::from_server_uuid(uuid);
    }
    return true;
}

void debug_print(printf_buffer_t *buf, const server_id_t &server_id) {
    buf->appendf("%s", server_id.print().c_str());
}

// Universal serialization functions: you MUST NOT change their implementations.
// (You could find a way to remove these functions, though.)
void serialize_universal(write_message_t *wm, const server_id_t &server_id) {
    // We take the UUID and flag it in one of the reserved bits of version 4 UUIDs,
    // depending on whether the ID is for a proxy or not.
    // The reason for this is so that we don't break compatibility with older servers
    // that expect a plain `uuid_u`.
    uuid_u flagged_uuid = server_id.get_uuid();
    guarantee((flagged_uuid.data()[8] & 0x40) == 0);
    if (server_id.is_proxy()) {
        flagged_uuid.data()[8] = flagged_uuid.data()[8] ^ 0x40;
    }
    serialize_universal(wm, flagged_uuid);
}
archive_result_t deserialize_universal(read_stream_t *s, server_id_t *server_id) {
    uuid_u flagged_uuid;
    archive_result_t res = deserialize_universal(s, &flagged_uuid);
    if (bad(res)) { return res; }
    bool is_proxy = (flagged_uuid.data()[8] & 0x40) == 0x40;
    if (is_proxy) {
        // Undo the flagging
        uuid_u uuid = flagged_uuid;
        uuid.data()[8] = uuid.data()[8] ^ 0x40;
        *server_id = server_id_t::from_proxy_uuid(uuid);
    } else {
        *server_id = server_id_t::from_server_uuid(flagged_uuid);
    }
    return archive_result_t::SUCCESS;
}

template <cluster_version_t W>
void serialize(write_message_t *wm, const server_id_t &sid) {
    serialize_universal(wm, sid);
}
template <cluster_version_t W>
archive_result_t deserialize(read_stream_t *s, server_id_t *sid) {
    return deserialize_universal(s, sid);
}
INSTANTIATE_SERIALIZABLE_SINCE_v1_13(server_id_t);
