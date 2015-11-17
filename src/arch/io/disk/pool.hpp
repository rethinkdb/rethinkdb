// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef ARCH_IO_DISK_POOL_HPP_
#define ARCH_IO_DISK_POOL_HPP_

#include <sys/uio.h>

#include <functional>
#include <string>

#include "arch/runtime/event_queue.hpp"
#include "arch/io/blocker_pool.hpp"
#include "concurrency/queue/passive_producer.hpp"
#include "containers/scoped.hpp"

#ifndef __linux__
#define USE_WRITEV 0
#else
#define USE_WRITEV 1
#endif

struct iovec;
class pool_diskmgr_t;
class printf_buffer_t;

/* The pool disk manager uses a thread pool in conjunction with synchronous
(blocking) IO calls to asynchronously run IO requests. */

struct pool_diskmgr_action_t
    : private blocker_pool_t::job_t {
    pool_diskmgr_action_t() { }

    void make_write(fd_t _fd, const void *_buf, size_t _count, int64_t _offset,
                    bool _wrap_in_datasyncs) {
        type = ACTION_WRITE;
        wrap_in_datasyncs = _wrap_in_datasyncs;
        fd = _fd;
        buf_and_count.iov_base = const_cast<void *>(_buf);
        buf_and_count.iov_len = _count;
        offset = _offset;
    }

    void make_resize(fd_t _fd, int64_t _new_size,
                    bool _wrap_in_datasyncs) {
        type = ACTION_RESIZE;
        wrap_in_datasyncs = _wrap_in_datasyncs;
        fd = _fd;
        buf_and_count.iov_base = NULL;
        buf_and_count.iov_len = 0;
        offset = _new_size;
    }

#ifndef USE_WRITEV
#error "USE_WRITEV not defined... but we are in pool.hpp.  Where is it?"
#elif USE_WRITEV
    void make_writev(fd_t _fd, scoped_array_t<iovec> &&_bufs, size_t _count, int64_t _offset) {
        type = ACTION_WRITE;
        wrap_in_datasyncs = false;
        fd = _fd;
        iovecs = std::move(_bufs);
        buf_and_count.iov_base = NULL;
        buf_and_count.iov_len = _count;
        offset = _offset;
    }
#endif

    void make_read(fd_t _fd, void *_buf, size_t _count, int64_t _offset) {
        type = ACTION_READ;
        wrap_in_datasyncs = false;
        fd = _fd;
        buf_and_count.iov_base = _buf;
        buf_and_count.iov_len = _count;
        offset = _offset;
    }

    bool get_is_write() const { return type == ACTION_WRITE; }
    bool get_is_resize() const { return type == ACTION_RESIZE; }
    bool get_is_read() const { return type == ACTION_READ; }
    fd_t get_fd() const { return fd; }
    void get_bufs(iovec **iovecs_out, size_t *iovecs_len_out) {
        if (buf_and_count.iov_base != NULL) {
            *iovecs_out = &buf_and_count;
            *iovecs_len_out = 1;
        } else {
            *iovecs_out = iovecs.data();
            *iovecs_len_out = iovecs.size();
        }
    }
    size_t get_count() const { return buf_and_count.iov_len; }
    int64_t get_offset() const { return offset; }

    void set_successful_due_to_conflict() { io_result = get_count(); }
    bool get_succeeded() const { return io_result == static_cast<int64_t>(get_count()); }
    int get_io_errno() const {
        rassert(io_result < 0);
        return -io_result;
    }

private:
    friend class pool_diskmgr_t;
    pool_diskmgr_t *parent;

    enum action_type_t {ACTION_READ, ACTION_WRITE, ACTION_RESIZE};
    action_type_t type;
    bool wrap_in_datasyncs;
    fd_t fd;

    // Either type is ACTION_RESIZE, or buf_and_count.iov_base is used, or iovecs
    // is used (for writev).  If iovecs is used, then buf_and_count.iov_len is the
    // sum of the iovecs' iov_len fields.  Currently readv is not supported, but if
    // you need it, it should be easy to add.
    scoped_array_t<iovec> iovecs;
    iovec buf_and_count;
    int64_t offset;

    // Helper functions for vectored reads/writes
    static size_t advance_vector(iovec **vecs, size_t *count, size_t bytes_done);
    ssize_t vectored_read_write(iovec *vecs, size_t vecs_len, int64_t partial_offset);
    void copy_vectors(scoped_array_t<iovec> *vectors_out);
    int64_t perform_read_write(iovec *vecs, size_t count);

    int64_t io_result;

    void run();
    void done();

    DISABLE_COPYING(pool_diskmgr_action_t);
};

void debug_print(printf_buffer_t *buf,
                 const pool_diskmgr_action_t &action);

class pool_diskmgr_t : private availability_callback_t, public home_thread_mixin_debug_only_t {
public:
    friend struct pool_diskmgr_action_t;
    typedef pool_diskmgr_action_t action_t;

    /* The `pool_diskmgr_t` will draw actions to run from `source`. It will call `done_fun`
    on each one when it's done. */
    pool_diskmgr_t(linux_event_queue_t *queue, passive_producer_t<action_t *> *source,
                   int max_concurrent_io_requests);
    std::function<void(action_t *)> done_fun;
    ~pool_diskmgr_t();

private:
    const int queue_depth;
    passive_producer_t<action_t *> *source;
    blocker_pool_t blocker_pool;

    void on_source_availability_changed();
    int n_pending;
    void pump();

    DISABLE_COPYING(pool_diskmgr_t);
};

#endif /* ARCH_IO_DISK_POOL_HPP_ */
