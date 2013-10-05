#include "containers/archive/stl_types.hpp"

namespace std {

size_t serialized_size(const std::string &s) {
    return varint_uint64_serialized_size(s.size()) + s.size();
}

write_message_t &operator<<(write_message_t &msg, const std::string &s) {
    serialize_varint_uint64(&msg, s.size());
    msg.append(s.data(), s.size());
    return msg;
}

archive_result_t deserialize(read_stream_t *s, std::string *out) {
    out->clear();

    uint64_t sz;
    archive_result_t res = deserialize_varint_uint64(s, &sz);
    if (res) { return res; }

    if (sz > std::numeric_limits<size_t>::max()) {
        return ARCHIVE_RANGE_ERROR;
    }

    // Unfortunately we have to do an extra copy before dumping data
    // into a std::string.
    std::vector<char> v(sz);

    int64_t num_read = force_read(s, v.data(), sz);
    if (num_read == -1) {
        return ARCHIVE_SOCK_ERROR;
    }
    if (static_cast<uint64_t>(num_read) < sz) {
        return ARCHIVE_SOCK_EOF;
    }

    out->assign(v.data(), v.size());

    return ARCHIVE_SUCCESS;
}

}  // namespace std
