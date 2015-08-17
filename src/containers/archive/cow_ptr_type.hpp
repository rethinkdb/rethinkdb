// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONTAINERS_ARCHIVE_COW_PTR_TYPE_HPP_
#define CONTAINERS_ARCHIVE_COW_PTR_TYPE_HPP_

#include "containers/archive/archive.hpp"
#include "containers/cow_ptr.hpp"

template <cluster_version_t W, class T>
void serialize(write_message_t *wm, const cow_ptr_t<T> &x) {
    serialize<W>(wm, *x);
}

template <cluster_version_t W, class T>
MUST_USE archive_result_t deserialize(read_stream_t *s, cow_ptr_t<T> *x) {
    typename cow_ptr_t<T>::change_t change(x);
    return deserialize<W>(s, change.get());
}

#endif  // CONTAINERS_ARCHIVE_COW_PTR_TYPE_HPP_
