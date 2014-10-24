// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONTAINERS_DATA_BUFFER_HPP_
#define CONTAINERS_DATA_BUFFER_HPP_

#include "allocation/tracking_allocator.hpp"
#include "containers/counted.hpp"
#include "errors.hpp"

class printf_buffer_t;

class data_buffer_t {
private:
    intptr_t ref_count_;
    size_t size_;
    std::unique_ptr<deallocator_base_t> deleter_;
    char bytes_[];

    friend void counted_add_ref(data_buffer_t *buffer);
    friend void counted_set_deleter(data_buffer_t *buffer,
                                    std::unique_ptr<deallocator_base_t> &&d);
    friend void counted_release(data_buffer_t *buffer);

    DISABLE_COPYING(data_buffer_t);

public:
    explicit data_buffer_t(size_t size) : ref_count_(0), size_(size), deleter_() {}

    // data_buffer_t has an arbitrary length array in it, so we need
    // to define our own new / delete.  It's wrong to call new without
    // the new arguments, so we delete that.
    static void *operator new(size_t count, size_t bytes);
    static void *operator new(size_t count) = delete;
    static void operator delete(void *ptr) noexcept;

    static void destroy(data_buffer_t *p) {
        rassert(p->ref_count_ == 0);
        delete p;
    }

    static counted_t<data_buffer_t> create(int64_t size);
    static counted_t<data_buffer_t> create(size_t _size,
                                           std::shared_ptr<tracking_allocator_factory_t> f);

    char *buf() { return bytes_; }
    const char *buf() const { return bytes_; }
    int64_t size() const { return size_; }
};

inline void counted_add_ref(data_buffer_t *buffer) {
    DEBUG_VAR const intptr_t res = __sync_add_and_fetch(&buffer->ref_count_, 1);
    rassert(res > 0);
}

inline void counted_set_deleter(data_buffer_t *buffer,
                                std::unique_ptr<deallocator_base_t> &&d) {
    buffer->deleter_ = std::move(d);
}

inline void counted_release(data_buffer_t *buffer) {
    const intptr_t res = __sync_sub_and_fetch(&buffer->ref_count_, 1);
    rassert(res >= 0);
    if (res == 0) {
        if (buffer->deleter_) {
            buffer->deleter_->deallocate();
        } else {
            delete buffer;
        }
    }
}

void debug_print(printf_buffer_t *buf, const counted_t<data_buffer_t>& ptr);



#endif  // CONTAINERS_DATA_BUFFER_HPP_
