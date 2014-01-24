// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_ACCESS_HPP_
#define CONCURRENCY_ACCESS_HPP_

enum access_t {
    // Intent to read
    rwi_read,

    // Intent to write
    rwi_write,
};

inline bool is_read_mode(access_t mode) {
    return mode == rwi_read;
}
inline bool is_write_mode(access_t mode) {
    return !is_read_mode(mode);
}

#endif // CONCURRENCY_ACCESS_HPP_

