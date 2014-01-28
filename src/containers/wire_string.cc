// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "containers/wire_string.hpp"

#include <limits>

#include "containers/archive/varint.hpp"

size_t serialized_size(const wire_string_t *s) {
    return s->memory_size();
}

write_message_t &operator<<(write_message_t &msg, const wire_string_t *s) {
    serialize_varint_uint64(&msg, s->length());
    msg.append(reinterpret_cast<const void *>(s->data()), s->length());
    return msg;
}

archive_result_t deserialize(read_stream_t *s, wire_string_t **out) {
    *out = NULL;

    uint64_t sz;
    archive_result_t res = deserialize_varint_uint64(s, &sz);
    if (res) { return res; }

    if (sz > std::numeric_limits<size_t>::max()) {
        return ARCHIVE_RANGE_ERROR;
    }

    *out = wire_string_t::create(sz);

    int64_t num_read = force_read(s, (*out)->data(), sz);
    if (num_read == -1) {
        wire_string_t::destroy(*out);
        return ARCHIVE_SOCK_ERROR;
    }
    if (static_cast<uint64_t>(num_read) < sz) {
        wire_string_t::destroy(*out);
        return ARCHIVE_SOCK_EOF;
    }

    return ARCHIVE_SUCCESS;
}