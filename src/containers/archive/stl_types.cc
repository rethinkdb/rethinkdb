#include "containers/archive/stl_types.hpp"

namespace std {

size_t serialized_size(const std::string &s) {
    return serialized_size_t<int64_t>::value + s.size();
}

write_message_t &operator<<(write_message_t &msg, const std::string &s) {
    const char *data = s.data();
    int64_t sz = s.size();
    rassert(sz >= 0);

    msg << sz;
    msg.append(data, sz);

    return msg;
}

archive_result_t deserialize(read_stream_t *s, std::string *out) {
    out->clear();

    int64_t sz;
    archive_result_t res = deserialize(s, &sz);
    if (res) { return res; }

    if (sz < 0) {
        return ARCHIVE_RANGE_ERROR;
    }

    // Unfortunately we have to do an extra copy before dumping data
    // into a std::string.
    std::vector<char> v(sz);
    rassert(v.size() == uint64_t(sz));

    int64_t num_read = force_read(s, v.data(), sz);
    if (num_read == -1) {
        return ARCHIVE_SOCK_ERROR;
    }
    if (num_read < sz) {
        return ARCHIVE_SOCK_EOF;
    }

    rassert(num_read == sz, "force_read returned an invalid value %" PRIi64, num_read);

    out->assign(v.data(), v.size());

    return ARCHIVE_SUCCESS;
}

}  // namespace std
