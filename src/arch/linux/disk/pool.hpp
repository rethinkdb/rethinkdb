#ifndef __ARCH_LINUX_DISK_POOL_HPP__
#define __ARCH_LINUX_DISK_POOL_HPP__

#include "arch/linux/event_queue.hpp"
#include "arch/linux/blocker_pool.hpp"
#include <boost/function.hpp>

/* The pool disk manager uses a thread pool in conjunction with synchronous
(blocking) IO calls to asynchronously run IO requests. */

struct pool_diskmgr_t {

    pool_diskmgr_t(linux_event_queue_t *queue);

    struct action_t : private blocker_pool_t::job_t {

        void make_write(fd_t f, const void *b, size_t c, off_t o) {
            is_read = false;
            fd = f;
            buf = const_cast<void*>(b);
            count = c;
            offset = o;
        }
        void make_read(fd_t f, void *b, size_t c, off_t o) {
            is_read = true;
            fd = f;
            buf = b;
            count = c;
            offset = o;
        }

        bool get_is_read() const { return is_read; }
        fd_t get_fd() const { return fd; }
        void *get_buf() const { return buf; }
        size_t get_count() const { return count; }
        off_t get_offset() const { return offset; }

    private:
        friend class pool_diskmgr_t;
        pool_diskmgr_t *parent;

        bool is_read;
        fd_t fd;
        void *buf;
        size_t count;
        off_t offset;

        void run();
        void done();
    };

    /* Call submit() to start an operation. done_fun() will be called when the operation is
    over. */
    void submit(action_t *action);
    boost::function<void(action_t *)> done_fun;

private:
    blocker_pool_t blocker_pool;
};

#endif /* __ARCH_LINUX_DISK_POOL_HPP__ */
