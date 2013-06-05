// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "arch/io/disk/pool.hpp"

#include <fcntl.h>

#include "arch/io/disk.hpp"
#include "config/args.hpp"

int blocker_pool_queue_depth(int max_concurrent_io_requests) {
    guarantee(max_concurrent_io_requests > 0);
    guarantee(max_concurrent_io_requests < MAXIMUM_MAX_CONCURRENT_IO_REQUESTS);
    // TODO: Why make this twice MAX_CONCURRENT_IO_REQUESTS?
    return max_concurrent_io_requests * 2;
}


void debug_print(printf_buffer_t *buf,
                 const pool_diskmgr_action_t &action) {
    buf->appendf("pool_diskmgr_action{is_read=%s, fd=%d, buf=%p, count=%zu, "
                 "offset=%" PRIi64 ", errno=%d}",
                 action.get_is_read() ? "true" : "false",
                 action.get_fd(),
                 action.get_buf(),
                 action.get_count(),
                 action.get_offset(),
                 action.get_succeeded() ? 0 : action.get_errno());
}

pool_diskmgr_t::pool_diskmgr_t(linux_event_queue_t *queue,
                               passive_producer_t<action_t *> *_source,
                               int max_concurrent_io_requests)
    : queue_depth(blocker_pool_queue_depth(max_concurrent_io_requests)),
      source(_source),
      blocker_pool(max_concurrent_io_requests, queue),
      n_pending(0) {
    if (source->available->get()) { pump(); }
    source->available->set_callback(this);
}

pool_diskmgr_t::~pool_diskmgr_t() {
    assert_thread();
    source->available->unset_callback();
}

void pool_diskmgr_t::action_t::run() {
    if (is_read) {
        ssize_t res;
        do {
            res = pread(fd, buf, count, offset);
        } while (res == -1 && errno == EINTR);
        io_result = res >= 0 ? res : -errno;
    } else {
        ssize_t res;
        do {
            res = pwrite(fd, buf, count, offset);
        } while (res == -1 && errno == EINTR);
        io_result = res >= 0 ? res : -errno;

        // On OS X, we have to manually fsync to complete the write.  (We use F_FULLFSYNC because
        // fsync lies.)  On Linux we just open the descriptor with the O_DSYNC flag.
#ifndef FILE_SYNC_TECHNIQUE
#error "FILE_SYNC_TECHNIQUE is not defined"
#elif FILE_SYNC_TECHNIQUE == FILE_SYNC_TECHNIQUE_FULLFSYNC
        if (res >= 0) {
            int fcntl_res;
            do {
                fcntl_res = fcntl(fd, F_FULLFSYNC);
            } while (fcntl_res == -1 && errno == EINTR);

            io_result = fcntl_res == -1 ? -errno : res;
        }
#endif  // FILE_SYNC_TECHNIQUE
    }
}

void pool_diskmgr_action_t::done() {
    parent->assert_thread();
    parent->n_pending--;
    parent->pump();
    parent->done_fun(this);
}

void pool_diskmgr_t::on_source_availability_changed() {
    assert_thread();
    /* This is called when the queue used to be empty but now has requests on
    it, and also when the queue's last request is consumed. */
    if (source->available->get()) pump();
}

void pool_diskmgr_t::pump() {
    assert_thread();
    while (source->available->get() && n_pending < queue_depth) {
        action_t *a = source->pop();
        a->parent = this;
        n_pending++;
        blocker_pool.do_job(a);
    }
}

