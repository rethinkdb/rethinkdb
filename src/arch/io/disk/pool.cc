// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "arch/io/disk/pool.hpp"

#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "arch/io/disk.hpp"
#include "config/args.hpp"
#include "containers/printf_buffer.hpp"
#include "logger.hpp"

int blocker_pool_queue_depth(int max_concurrent_io_requests) {
    guarantee(max_concurrent_io_requests > 0);
    guarantee(max_concurrent_io_requests < MAXIMUM_MAX_CONCURRENT_IO_REQUESTS);
    // TODO: Why make this twice MAX_CONCURRENT_IO_REQUESTS?
    return max_concurrent_io_requests * 2;
}


void debug_print(printf_buffer_t *buf,
                 const pool_diskmgr_action_t &action) {
    buf->appendf("pool_diskmgr_action{is_read=%s, fd=%d, count=%zu, "
                 "offset=%" PRIi64 ", errno=%d}",
                 action.get_is_read() ? "true" : "false",
                 action.get_fd(),
                 action.get_count(),
                 action.get_offset(),
                 action.get_succeeded() ? 0 : action.get_io_errno());
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
    if (wrap_in_datasyncs) {
        int errcode = perform_datasync(fd);
        if (errcode != 0) {
            io_result = -errcode;
            return;
        }
    }

    switch (type) {
    case ACTION_RESIZE: {
        CT_ASSERT(sizeof(off_t) == sizeof(int64_t));
        int res;
        do {
            res = ftruncate(fd, offset);
        } while (res == -1 && get_errno() == EINTR);
        if (res == 0) {
            io_result = 0;
        } else {
            io_result = -get_errno();
            return;
        }
    } break;
    case ACTION_READ:
    case ACTION_WRITE: {
        ssize_t sum = 0;

        iovec *vecs;
        size_t vecs_len;
        get_bufs(&vecs, &vecs_len);
#ifndef USE_WRITEV
#error "USE_WRITEV not defined.  Did you include pool.hpp?"
#elif USE_WRITEV
        const size_t vecs_increment = IOV_MAX;
        size_t len;
        int64_t partial_offset = offset;
        for (size_t i = 0; i < vecs_len; i += len) {
            len = std::min(vecs_increment, vecs_len - i);
            ssize_t res;
            do {
                if (type == ACTION_READ) {
                    res = preadv(fd, vecs + i, len, partial_offset);
                } else {
                    rassert(type == ACTION_WRITE);
                    res = pwritev(fd, vecs + i, len, partial_offset);
                }
            } while (res == -1 && get_errno() == EINTR);

            if (res == -1) {
                io_result = -get_errno();
                return;
            }

            int64_t lensum = 0;
            for (size_t j = i; j < i + len; ++j) {
                lensum += vecs[j].iov_len;
            }
            if (lensum != res && type == ACTION_WRITE) {
                // This happens when running out of disk space.
                // The errno in that case is 0 and doesn't tell us about the
                // real reason for the failed i/o.
                // We set the io_result to be -ENOSPC and print an error stating
                // what actually happened.
                io_result = -ENOSPC;
                logERR("Failed I/O: lensum (%" PRIi64 ") != res (%zd)."
                       " Assuming we ran out of disk space.", lensum, res);
                return;
            }
            guarantee(lensum == res);

            partial_offset += lensum;
            sum += res;
        }
#else  // USE_WRITEV
        guarantee(vecs_len == 1);
        ssize_t res;
        do {
            if (type == ACTION_READ) {
                res = pread(fd, vecs[0].iov_base, vecs[0].iov_len, offset);
            } else {
                rassert(type == ACTION_WRITE);
                res = pwrite(fd, vecs[0].iov_base, vecs[0].iov_len, offset);
            }
        } while (res == -1 && get_errno() == EINTR);

        if (res == -1) {
            io_result = -get_errno();
            return;
        }

        sum = res;
#endif  // USE_WRITEV

        io_result = sum;
    } break;
    default:
        unreachable("Unknown I/O action");
    }

    if (wrap_in_datasyncs) {
        int errcode = perform_datasync(fd);
        if (errcode != 0) {
            io_result = -errcode;
            return;
        }
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

