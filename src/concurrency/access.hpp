// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_ACCESS_HPP_
#define CONCURRENCY_ACCESS_HPP_

enum class access_t {
    read,
    write,
};

inline bool is_read_mode(access_t mode) {
    return mode == access_t::read;
}
inline bool is_write_mode(access_t mode) {
    return !is_read_mode(mode);
}

#endif // CONCURRENCY_ACCESS_HPP_

