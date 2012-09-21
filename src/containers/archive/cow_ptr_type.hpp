#ifndef CONTAINERS_ARCHIVE_COW_PTR_TYPE_HPP_
#define CONTAINERS_ARCHIVE_COW_PTR_TYPE_HPP_

#include "containers/archive/archive.hpp"
#include "containers/cow_ptr.hpp"

template <class T>
write_message_t &operator<<(write_message_t &msg, const cow_ptr_t<T> &x) {
    msg << *x;
    return msg;
}

template <class T>
MUST_USE archive_result_t deserialize(read_stream_t *s, cow_ptr_t<T> *x) {
    typename cow_ptr_t<T>::change_t change(x);
    return deserialize(s, change.get());
}

#endif  // CONTAINERS_ARCHIVE_COW_PTR_TYPE_HPP_
