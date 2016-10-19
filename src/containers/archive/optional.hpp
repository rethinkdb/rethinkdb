#ifndef CONTAINERS_ARCHIVE_OPTIONAL_HPP_
#define CONTAINERS_ARCHIVE_OPTIONAL_HPP_

#include "containers/archive/archive.hpp"
#include "containers/optional.hpp"

template <cluster_version_t W, class T>
void serialize(write_message_t *wm, const optional<T> &x) {
    bool exists = x.has_value();
    serialize<W>(wm, exists);
    if (exists) {
        serialize<W>(wm, x.get());
    }
}

template <cluster_version_t W, class T>
MUST_USE archive_result_t deserialize(read_stream_t *s, optional<T> *x) {
    bool exists;

    archive_result_t res = deserialize<W>(s, &exists);
    if (bad(res)) { return res; }
    if (exists) {
        x->set(T());
        res = deserialize<W>(s, &x->get());
        return res;
    } else {
        x->reset();
        return archive_result_t::SUCCESS;
    }
}

#endif  // CONTAINERS_ARCHIVE_OPTIONAL_HPP_
