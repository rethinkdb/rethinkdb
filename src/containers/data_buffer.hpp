// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONTAINERS_DATA_BUFFER_HPP_
#define CONTAINERS_DATA_BUFFER_HPP_

#include "containers/counted.hpp"
#include "errors.hpp"

class printf_buffer_t;

struct data_buffer_t {
private:
    intptr_t ref_count_;
    size_t size_;
    char bytes_[];

    friend void counted_add_ref(data_buffer_t *buffer);
    friend void counted_release(data_buffer_t *buffer);

    DISABLE_COPYING(data_buffer_t);

public:
    static void destroy(data_buffer_t *p) {
        rassert(p->ref_count_ == 0);
        free(p);
    }

    static counted_t<data_buffer_t> create(int64_t size);

    char *buf() { return bytes_; }
    const char *buf() const { return bytes_; }
    int64_t size() const { return size_; }
};

inline void counted_add_ref(data_buffer_t *buffer) {
    DEBUG_VAR const intptr_t res = __sync_add_and_fetch(&buffer->ref_count_, 1);
    rassert(res > 0);
}

inline void counted_release(data_buffer_t *buffer) {
    const intptr_t res = __sync_sub_and_fetch(&buffer->ref_count_, 1);
    rassert(res >= 0);
    if (res == 0) {
        data_buffer_t::destroy(buffer);
    }
}

void debug_print(printf_buffer_t *buf, const counted_t<data_buffer_t>& ptr);



#endif  // CONTAINERS_DATA_BUFFER_HPP_
