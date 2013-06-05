// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef ARCH_IO_DISK_POOL_HPP_
#define ARCH_IO_DISK_POOL_HPP_

#include <string>

#include "errors.hpp"
#include <boost/function.hpp>

#include "arch/runtime/event_queue.hpp"
#include "arch/io/blocker_pool.hpp"
#include "concurrency/queue/passive_producer.hpp"


class pool_diskmgr_t;

/* The pool disk manager uses a thread pool in conjunction with synchronous
(blocking) IO calls to asynchronously run IO requests. */

struct pool_diskmgr_action_t
    : private blocker_pool_t::job_t, public home_thread_mixin_debug_only_t {
    pool_diskmgr_action_t() { }

    void make_write(fd_t f, const void *b, size_t c, int64_t o) {
        is_read = false;
        fd = f;
        buf = const_cast<void *>(b);
        count = c;
        offset = o;
    }
    void make_read(fd_t f, void *b, size_t c, int64_t o) {
        is_read = true;
        fd = f;
        buf = b;
        count = c;
        offset = o;
    }

    bool get_is_write() const { return !is_read; }
    bool get_is_read() const { return is_read; }
    fd_t get_fd() const { return fd; }
    void *get_buf() const { return buf; }
    size_t get_count() const { return count; }
    int64_t get_offset() const { return offset; }

    void set_successful_due_to_conflict() { io_result = count; }
    bool get_succeeded() const { return io_result == static_cast<int64_t>(count); }
    int get_errno() const {
        rassert(io_result < 0);
        return -io_result;
    }

    std::string backtrace;

private:
    friend class pool_diskmgr_t;
    pool_diskmgr_t *parent;

    bool is_read;
    fd_t fd;
    void *buf;
    size_t count;
    int64_t offset;

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
    boost::function<void(action_t *)> done_fun;
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
