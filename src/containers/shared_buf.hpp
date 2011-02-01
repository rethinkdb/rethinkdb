#ifndef __CONTAINERS_SHARED_BUF_HPP__
#define __CONTAINERS_SHARED_BUF_HPP__

#include <stddef.h>
#include "utils.hpp"

// TODO: Sometime before the year 3000, compare the performance to
// using boost::shared_array, which uses atomic incr/decr operations.

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
    void decr_refcount() { refcount--; if (refcount == 0) { free(this); } }
};

class weak_buf_t;

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

    // Must be called on the home_thread of the shared_buf_buffer_t.
    explicit shared_buf_t(weak_buf_t other);

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

    friend class weak_buf_t;

private:
    // Points to the data field of the shared_buf_buffer_t!
    char *buffer_;

    DISABLE_COPYING(shared_buf_t);
};

class weak_buf_t {
public:
    weak_buf_t() : buffer_(NULL) { }
    explicit weak_buf_t(shared_buf_t& buf) {
        buffer_ = buf.buffer_;
    }
    ~weak_buf_t() {
        buffer_ = NULL;
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

    friend class shared_buf_t;
private:
    char *buffer_;
};

// A pointer to data sitting amid a shared buffer.
template <class T>
class buffed_data_t {
public:
    buffed_data_t() : buf_(), offset_(0) { }

    // HACK, the third parameter gives us the same interface as
    // streampair, which is used in replication::check_pass.
    buffed_data_t(weak_buf_t buf, size_t offset, size_t end = 0)
        : buf_(buf), offset_(offset) { }

    void swap(buffed_data_t<T>& other) {
        size_t tmp = other.offset_;
        other.offset_ = offset_;
        offset_ = tmp;
        buf_.swap(other.buf_);
    }

    T *get() { return buf_.get<T>(offset_); }

    void release();

private:
    shared_buf_t buf_;
    size_t offset_;
};



#endif  // __CONTAINERS_SHARED_BUF_HPP__
