#ifndef __CONTAINERS_SHARED_BUF_HPP__
#define __CONTAINERS_SHARED_BUF_HPP__

#include <stddef.h>
#include "utils.hpp"

// This helps us avoid copying stuff out of a network buffer, and then
// copying it into another buffer, and then copying it into another
// buffer, and so on.

struct shared_buf_buffer_t {
    int home_thread;
    int refcount;
    size_t length;
    char data[];

    friend class shared_buf_t;
private:
    bool incr_refcount() { refcount++; return true; }
    bool decr_refcount() { refcount--; if (refcount == 0) { free(this); } return true; }
};

class shared_buf_t {
public:
    shared_buf_t() : buffer_(NULL) { }
    explicit shared_buf_t(size_t n) {
        shared_buf_buffer_t *shbuf = reinterpret_cast<shared_buf_buffer_t *>(malloc(sizeof(shared_buf_buffer_t) + n));
        buffer_ = reinterpret_cast<char *>(shbuf) + sizeof(shared_buf_buffer_t);
        shbuf->home_thread = get_thread_id();
        shbuf->refcount = 1;
        shbuf->length = n;
    }

    // Avoid using this capriciously.  It would be nice if we could
    // compress cross-thread reference count incr/decr messages so
    // that only one is sent per event loop, but right now we don't.
    // Use swap if it's appropriate.
    explicit shared_buf_t(shared_buf_t& other) {
        buffer_ = other.buffer_;
        if (buffer_) {
            shared_buf_buffer_t *shbuf = reinterpret_cast<shared_buf_buffer_t *>(buffer_ - sizeof(shared_buf_buffer_t));
            do_on_thread(shbuf->home_thread, shbuf, &shared_buf_buffer_t::incr_refcount);
        }
    }
    ~shared_buf_t() {
        release();
    }

    void swap(shared_buf_t& other) {
        char *tmp = buffer_;
        buffer_ = other.buffer_;
        other.buffer_ = tmp;
    }

    char *get() {
        return buffer_;
    }

    size_t size() {
        rassert(buffer_);
        shared_buf_buffer_t *shbuf = reinterpret_cast<shared_buf_buffer_t *>(buffer_ - sizeof(shared_buf_buffer_t));
        return shbuf->length;
    }

    template <class T>
    T *get(size_t off) {
        rassert(buffer_ != NULL);
        return reinterpret_cast<T *>(buffer_ + off);
    }

    void release() {
        if (buffer_) {
            shared_buf_buffer_t *shbuf = reinterpret_cast<shared_buf_buffer_t *>(buffer_ - sizeof(shared_buf_buffer_t));
            do_on_thread(shbuf->home_thread, shbuf, &shared_buf_buffer_t::decr_refcount);
            buffer_ = NULL;
        }
    }

private:
    // Points to the data field of the shared_buf_buffer_t!
    char *buffer_;
};

// A pointer to data sitting amid a shared buffer.
template <class T>
class buffed_data_t {
public:
    buffed_data_t() : buf_(), offset_(0) { }

    buffed_data_t(shared_buf_t& sharebuf, size_t offset) {
        
    }

    void swapset(shared_buf_t& buf, size_t offset);
    void swap(buffed_data_t<T>& other);

    T *get() { return buf_.get<T>(offset_); }

    void release();

private:
    shared_buf_t buf_;
    size_t offset_;
};



#endif  // __CONTAINERS_SHARED_BUF_HPP__
