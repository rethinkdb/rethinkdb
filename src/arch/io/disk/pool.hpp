#ifndef ARCH_IO_DISK_POOL_HPP_
#define ARCH_IO_DISK_POOL_HPP_

#include "errors.hpp"
#include <boost/function.hpp>

#include "arch/runtime/event_queue.hpp"
#include "arch/io/blocker_pool.hpp"
#include "concurrency/queue/passive_producer.hpp"



/* The pool disk manager uses a thread pool in conjunction with synchronous
(blocking) IO calls to asynchronously run IO requests. */

class pool_diskmgr_t : private availability_callback_t, public home_thread_mixin_debug_only_t {
public:
    struct action_t : private blocker_pool_t::job_t, public home_thread_mixin_debug_only_t {

        void make_write(fd_t f, const void *b, size_t c, off_t o) {
            is_read = false;
            fd = f;
            buf = const_cast<void*>(b);
            count = c;
            offset = o;
            successful_due_to_conflict = false;
        }
        void make_read(fd_t f, void *b, size_t c, off_t o) {
            is_read = true;
            fd = f;
            buf = b;
            count = c;
            offset = o;
            successful_due_to_conflict = false;
        }

        bool get_is_write() const { return !is_read; }
        bool get_is_read() const { return is_read; }
        fd_t get_fd() const { return fd; }
        void *get_buf() const { return buf; }
        size_t get_count() const { return count; }
        off_t get_offset() const { return offset; }

        void set_successful_due_to_conflict() { successful_due_to_conflict = true; }
        bool get_succeeded() const { return successful_due_to_conflict || io_result == static_cast<int>(count); }

        std::string backtrace;

    private:
        friend class pool_diskmgr_t;
        pool_diskmgr_t *parent;

        bool is_read;
        fd_t fd;
        void *buf;
        size_t count;
        off_t offset;

        // TODO: io_result almost probably definitely should be an
        // int64_t or was at some point, like in libaio.
        int io_result;

        /* Why we need this: before having this field an action was considered
         * successful if io_result == count this only worked if the action was
         * actually run. If a read is done very shortly after a write on the
         * same area of a file then that read will instead use the writes data.
         * (See confict_resolving.hpp/tcc for more explanation of this.)
         * However when it does this ioresult is unset and thus not an
         * indication of success. Because when a read is satisfied by a
         * conflict there's no way for it to fail we give the people above a
         * way to explicitly specify success via this mechanism. */
        bool successful_due_to_conflict;

        void run();
        void done();
    };

    /* The `pool_diskmgr_t` will draw actions to run from `source`. It will call `done_fun`
    on each one when it's done. */
    pool_diskmgr_t(linux_event_queue_t *queue, passive_producer_t<action_t *> *source);
    boost::function<void(action_t *)> done_fun;
    ~pool_diskmgr_t();

private:
    passive_producer_t<action_t *> *source;
    blocker_pool_t blocker_pool;

    void on_source_availability_changed();
    int n_pending;
    void pump();

    DISABLE_COPYING(pool_diskmgr_t);
};

#endif /* ARCH_IO_DISK_POOL_HPP_ */
