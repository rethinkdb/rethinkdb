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

// Advance the vector, while respecting the device block size alignment for direct I/O
size_t pool_diskmgr_t::action_t::advance_vector(iovec **vecs,
                                                size_t *count,
                                                size_t bytes_done) {
    // Note: if somehow the read/write call returns a result unaligned with 
    // DEVICE_BLOCK_SIZE, there will probably be an error when we try again with the
    // advanced vector.
    size_t bytes_to_advance = bytes_done;

    while (bytes_to_advance > 0) {
        rassert(*count > 0);
        size_t cur_done = std::min((*vecs)->iov_len, bytes_to_advance);
        bytes_to_advance -= cur_done;
        (*vecs)->iov_len -= cur_done;
        (*vecs)->iov_base = static_cast<char *>((*vecs)->iov_base) + cur_done;

        while (*count > 0 && (*vecs)->iov_len == 0) {
            ++(*vecs);
            --(*count);
        }
    }

    return bytes_done;
}

ssize_t pool_diskmgr_t::action_t::vectored_read_write(iovec *vecs,
                                                      size_t vecs_len,
                                                      int64_t partial_offset) {
    int64_t real_offset = offset + partial_offset;
#ifndef USE_WRITEV
#error "USE_WRITEV not defined.  Did you include pool.hpp?"
#elif USE_WRITEV
    size_t vecs_to_use = std::min<size_t>(vecs_len, IOV_MAX);
    if (type == ACTION_READ) {
        return preadv(fd, vecs, vecs_to_use, real_offset);
    }
    rassert(type == ACTION_WRITE);
    return pwritev(fd, vecs, vecs_to_use, real_offset);
#else
    guarantee(vecs_len == 1);
    guarantee(partial_offset == 0);
    if (type == ACTION_READ) {
        return pread(fd, vecs[0].iov_base, vecs[0].iov_len, real_offset);
    }
    rassert(type == ACTION_WRITE);
    return pwrite(fd, vecs[0].iov_base, vecs[0].iov_len, real_offset);
#endif
}

void pool_diskmgr_t::action_t::copy_vectors(scoped_array_t<iovec> *vectors_out) {
    iovec *vectors;
    size_t count;
    get_bufs(&vectors, &count);
    vectors_out->init(count);

    for (size_t i = 0; i < count; ++i) {
        (*vectors_out)[i] = vectors[i];
    }
}

int64_t pool_diskmgr_t::action_t::perform_read_write(iovec *vecs,
                                                     size_t vecs_len) {
    ssize_t res = 0;
    int64_t partial_offset = 0;

    int64_t total_bytes = 0;
    for (size_t i = 0; i < vecs_len; ++i) {
        total_bytes += vecs[i].iov_len;
    }

    while (vecs_len > 0 && partial_offset < total_bytes) {
        do {
            res = vectored_read_write(vecs, vecs_len, partial_offset);
        } while (res == -1 && get_errno() == EINTR);

        if (res == -1) {
            return -get_errno();
        } else if (res == 0 && type == ACTION_WRITE) {
            // This happens when running out of disk space.
            // The errno in that case is 0 and doesn't tell us about the
            // real reason for the failed i/o.
            // We set the io_result to be -ENOSPC and print an error stating
            // what actually happened.
            logERR("Failed I/O: vectored write of %" PRIi64 " bytes stopped after "
                   "%" PRIi64 " bytes. Assuming we ran out of disk space.",
                   total_bytes, partial_offset);
            return -ENOSPC;
        }

        // Advance the vector in DEVICE_BLOCK_SIZE chunks and update our offset
        partial_offset += advance_vector(&vecs, &vecs_len, res);
    }
    return total_bytes;
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
#ifdef _WIN32
        FILE_END_OF_FILE_INFO new_eof;
        new_eof.EndOfFile.QuadPart = offset;
        BOOL res = SetFileInformationByHandle(fd, FileEndOfFileInfo, &new_eof, sizeof(new_eof));
        if (!res) {
            logWRN("SetFileInformationByHandle failed: %s", winerr_string(GetLastError()).c_str());
            io_result = -EIO;
            return;
        }
        io_result = 0;
#else
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
#endif
    } break;
    case ACTION_READ:
    case ACTION_WRITE: {
        // Copy the io vectors because perform_read_write will modify them
        scoped_array_t<iovec> vectors;
        copy_vectors(&vectors);
        io_result = perform_read_write(vectors.data(), vectors.size());
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

