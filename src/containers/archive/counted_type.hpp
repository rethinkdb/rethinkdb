// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONTAINERS_ARCHIVE_COUNTED_TYPE_HPP_
#define CONTAINERS_ARCHIVE_COUNTED_TYPE_HPP_

#include "containers/counted.hpp"

template <class T>
write_message_t &operator<<(write_message_t &msg, const counted_t<const T> &x) {
    msg << x.has();
    if (x.has()) {
        msg << *x;
    }
    return msg;
}

template <class T>
archive_result_t deserialize(read_stream_t *s, counted_t<const T> *thing) {
    bool has;
    archive_result_t res = deserialize(s, &has);
    if (res) {
        return res;
    }
    if (has) {
        scoped_ptr_t<T> tmp(new T);
        res = deserialize(s, tmp.get());
        if (res) {
            return res;
        }
        thing->reset(tmp.release());
    } else {
        thing->reset();
    }
    return res;
}




#endif  // CONTAINERS_ARCHIVE_COUNTED_TYPE_HPP_
