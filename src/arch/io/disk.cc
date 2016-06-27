// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "arch/io/disk.hpp"

#include <fcntl.h>

#ifndef _WIN32
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <libgen.h>
#endif

#include <unistd.h>
#include <limits.h>

#include <algorithm>
#include <functional>

#include "arch/types.hpp"
#include "arch/runtime/thread_pool.hpp"
#include "arch/runtime/runtime.hpp"
#include "arch/io/disk/filestat.hpp"
#include "arch/io/disk/pool.hpp"
#include "arch/io/disk/conflict_resolving.hpp"
#include "arch/io/disk/stats.hpp"
#include "arch/io/disk/accounting.hpp"
#include "backtrace.hpp"
#include "config/args.hpp"
#include "do_on_thread.hpp"
#include "logger.hpp"
#include "utils.hpp"

void verify_aligned_file_access(DEBUG_VAR int64_t file_size, DEBUG_VAR int64_t offset,
                                DEBUG_VAR size_t length,
                                DEBUG_VAR const scoped_array_t<iovec> &bufs);

/* Disk manager object takes care of queueing operations, collecting statistics, preventing
   conflicts, and actually sending them to the disk. */
class linux_disk_manager_t : public home_thread_mixin_t {
public:
    struct action_t : public stats_diskmgr_t::action_t {
        action_t(threadnum_t _cb_thread, linux_iocallback_t *_cb)
            : cb_thread(_cb_thread), cb(_cb) { }
        threadnum_t cb_thread;
        linux_iocallback_t *cb;
    };


    linux_disk_manager_t(linux_event_queue_t *queue,
                         int batch_factor,
                         int max_concurrent_io_requests,
                         perfmon_collection_t *stats) :
        stack_stats(stats, "stack"),
        conflict_resolver(stats),
        accounter(batch_factor),
        backend_stats(stats, "backend", accounter.producer),
        backend(queue, backend_stats.producer, max_concurrent_io_requests),
        outstanding_txn(0)
    {
        /* Hook up the `submit_fun`s of the parts of the IO stack that are above the
        queue. (The parts below the queue use the `passive_producer_t` interface instead
        of a callback function.) */
        stack_stats.submit_fun = std::bind(&conflict_resolving_diskmgr_t::submit,
                                           &conflict_resolver, ph::_1);
        conflict_resolver.submit_fun = std::bind(&accounting_diskmgr_t::submit,
                                                 &accounter, ph::_1);

        /* Hook up everything's `done_fun`. */
        backend.done_fun = std::bind(&stats_diskmgr_2_t::done, &backend_stats, ph::_1);
        backend_stats.done_fun = std::bind(&accounting_diskmgr_t::done, &accounter, ph::_1);
        accounter.done_fun = std::bind(&conflict_resolving_diskmgr_t::done,
                                       &conflict_resolver, ph::_1);
        conflict_resolver.done_fun = std::bind(&stats_diskmgr_t::done, &stack_stats, ph::_1);
        stack_stats.done_fun = std::bind(&linux_disk_manager_t::done, this, ph::_1);
    }

    ~linux_disk_manager_t() {
        rassert(outstanding_txn == 0,
                "Closing a file with outstanding txns (%" PRIiPTR " of them)\n",
                outstanding_txn);
    }

    void *create_account(int pri, int outstanding_requests_limit) {
        return new accounting_diskmgr_t::account_t(&accounter, pri, outstanding_requests_limit);
    }

    void destroy_account(void *account) {
        coro_t::spawn_on_thread([account] {
            // The account destructor can block if there are outstanding requests.
            delete static_cast<accounting_diskmgr_t::account_t *>(account);
        }, home_thread());
    }

    void submit_action_to_stack_stats(action_t *a) {
        assert_thread();
        outstanding_txn++;
        stack_stats.submit(a);
    }

    void submit_write(fd_t fd, const void *buf, size_t count, int64_t offset,
                      void *account, linux_iocallback_t *cb,
                      bool wrap_in_datasyncs) {
        threadnum_t calling_thread = get_thread_id();

        action_t *a = new action_t(calling_thread, cb);
        a->make_write(fd, buf, count, offset, wrap_in_datasyncs);
        a->account = static_cast<accounting_diskmgr_t::account_t *>(account);

        do_on_thread(home_thread(),
                     std::bind(&linux_disk_manager_t::submit_action_to_stack_stats, this,
                               a));
    }

    void submit_resize(fd_t fd, int64_t new_size,
                      void *account, linux_iocallback_t *cb,
                      bool wrap_in_datasyncs) {
        threadnum_t calling_thread = get_thread_id();

        action_t *a = new action_t(calling_thread, cb);
        a->make_resize(fd, new_size, wrap_in_datasyncs);
        a->account = static_cast<accounting_diskmgr_t::account_t *>(account);

        do_on_thread(home_thread(),
                     std::bind(&linux_disk_manager_t::submit_action_to_stack_stats, this,
                               a));
    }

#ifndef USE_WRITEV
#error "USE_WRITEV not defined.  Did you include pool.hpp?"
#elif USE_WRITEV
    void submit_writev(fd_t fd, scoped_array_t<iovec> &&bufs, size_t count,
                       int64_t offset, void *account, linux_iocallback_t *cb) {
        threadnum_t calling_thread = get_thread_id();

        action_t *a = new action_t(calling_thread, cb);
        a->make_writev(fd, std::move(bufs), count, offset);
        a->account = static_cast<accounting_diskmgr_t::account_t *>(account);

        do_on_thread(home_thread(),
                     std::bind(&linux_disk_manager_t::submit_action_to_stack_stats, this,
                               a));
    }
#endif  // USE_WRITEV

    void submit_read(fd_t fd, void *buf, size_t count, int64_t offset, void *account, linux_iocallback_t *cb) {
        threadnum_t calling_thread = get_thread_id();

        action_t *a = new action_t(calling_thread, cb);
        a->make_read(fd, buf, count, offset);
        a->account = static_cast<accounting_diskmgr_t::account_t*>(account);

        do_on_thread(home_thread(),
                     std::bind(&linux_disk_manager_t::submit_action_to_stack_stats, this,
                               a));
    }

    void done(stats_diskmgr_t::action_t *a) {
        assert_thread();
        outstanding_txn--;
        action_t *a2 = static_cast<action_t *>(a);
        bool succeeded = a2->get_succeeded();
        if (succeeded) {
            do_on_thread(a2->cb_thread,
                         std::bind(&linux_iocallback_t::on_io_complete, a2->cb));
        } else {
            do_on_thread(a2->cb_thread,
                         std::bind(&linux_iocallback_t::on_io_failure,
                                   a2->cb, a2->get_io_errno(),
                                   static_cast<int64_t>(a2->get_offset()),
                                   static_cast<int64_t>(a2->get_count())));
        }
        delete a2;
    }

private:
    /* These fields describe the entire IO stack. At the top level, we allocate a new
    action_t object for each operation and record its callback. Then it passes through
    the conflict resolver, which enforces ordering constraints between IO operations by
    holding back operations that must be run after other, currently-running, operations.
    Then it goes to the account manager, which queues up running IO operations according
    to which account they are part of. Finally the "backend" pops the IO operations
    from the queue.

    At two points in the process--once as soon as it is submitted, and again right
    as the backend pops it off the queue--its statistics are recorded. The "stack stats"
    will tell you how many IO operations are queued. The "backend stats" will tell you
    how long the OS takes to perform the operations. Note that it's not perfect, because
    it counts operations that have been queued by the backend but not sent to the OS yet
    as having been sent to the OS. */

    stats_diskmgr_t stack_stats;
    conflict_resolving_diskmgr_t conflict_resolver;
    accounting_diskmgr_t accounter;
    stats_diskmgr_2_t backend_stats;
    pool_diskmgr_t backend;


    intptr_t outstanding_txn;

    DISABLE_COPYING(linux_disk_manager_t);
};

io_backender_t::io_backender_t(file_direct_io_mode_t _direct_io_mode,
                               int max_concurrent_io_requests)
    : direct_io_mode(_direct_io_mode),
      diskmgr(new linux_disk_manager_t(&linux_thread_pool_t::get_thread()->queue,
                                       DEFAULT_IO_BATCH_FACTOR,
                                       max_concurrent_io_requests,
                                       &stats)) { }

io_backender_t::~io_backender_t() { }

file_direct_io_mode_t io_backender_t::get_direct_io_mode() const { return direct_io_mode; }


/* Disk file object */

linux_file_t::linux_file_t(scoped_fd_t &&_fd, int64_t _file_size, linux_disk_manager_t *_diskmgr)
    : fd(std::move(_fd)), file_size(_file_size), diskmgr(_diskmgr) {
    // TODO: Why do we care whether we're in a thread pool?  (Maybe it's that you can't create a
    // file_account_t outside of the thread pool?  But they're associated with the diskmgr,
    // aren't they?)
    if (linux_thread_pool_t::get_thread()) {
        default_account.init(new file_account_t(this, 1, UNLIMITED_OUTSTANDING_REQUESTS));
    }
}

int64_t linux_file_t::get_file_size() {
    return file_size;
}

/* If you want to use this for downsizing a file, please check the WARNING about
`set_file_size()` in disk.hpp. */
void linux_file_t::set_file_size(int64_t size) {
    assert_thread();
    rassert(diskmgr, "No diskmgr has been constructed (are we running without an event queue?)");

    struct rs_callback_t : public linux_iocallback_t, cond_t {
        void on_io_complete() {
            delete this;
        }

        void on_io_failure(int errsv, int64_t offset, int64_t) {
            crash("ftruncate failed.  (%s) (target size = %" PRIi64 ")",
                  errno_string(errsv).c_str(), offset);
        }

        auto_drainer_t::lock_t lock;
    };
    rs_callback_t *rs_callback = new rs_callback_t();
    rs_callback->lock = file_size_ops_drainer.lock();
    diskmgr->submit_resize(fd.get(), size, default_account->get_account(),
                           rs_callback, true);

    file_size = size;
}

// For growing in large chunks at a time.
int64_t chunk_factor(int64_t size) {
    // x is at most 6.25% of size.
    int64_t x = (size / (DEFAULT_EXTENT_SIZE * 16)) * DEFAULT_EXTENT_SIZE;
    return clamp<int64_t>(x, DEVICE_BLOCK_SIZE * 128, DEFAULT_EXTENT_SIZE * 64);
}

void linux_file_t::set_file_size_at_least(int64_t size) {
    assert_thread();
    if (file_size < size) {
        /* Grow in large chunks at a time */
        set_file_size(ceil_aligned(size, chunk_factor(size)));
    }
}

void linux_file_t::read_async(int64_t offset, size_t length, void *buf, file_account_t *account, linux_iocallback_t *callback) {
    rassert(diskmgr, "No diskmgr has been constructed (are we running without an event queue?)");
    verify_aligned_file_access(file_size, offset, length, buf);
    diskmgr->submit_read(fd.get(), buf, length, offset,
        account == DEFAULT_DISK_ACCOUNT ? default_account->get_account() : account->get_account(),
        callback);
}

void linux_file_t::write_async(int64_t offset, size_t length, const void *buf,
                               file_account_t *account, linux_iocallback_t *callback,
                               wrap_in_datasyncs_t wrap_in_datasyncs) {
    rassert(diskmgr, "No diskmgr has been constructed (are we running without an event queue?)");
    verify_aligned_file_access(file_size, offset, length, buf);
    diskmgr->submit_write(fd.get(), buf, length, offset,
                          account == DEFAULT_DISK_ACCOUNT ? default_account->get_account() : account->get_account(),
                          callback,
                          wrap_in_datasyncs == WRAP_IN_DATASYNCS);
}

void linux_file_t::writev_async(int64_t offset, size_t length,
                                scoped_array_t<iovec> &&bufs,
                                file_account_t *account, linux_iocallback_t *callback) {
    rassert(diskmgr != nullptr,
            "No diskmgr has been constructed (are we running without an event queue?)");
    verify_aligned_file_access(file_size, offset, length, bufs);

#ifndef USE_WRITEV
#error "USE_WRITEV not defined.  Did you include pool.hpp?"
#elif USE_WRITEV
    diskmgr->submit_writev(fd.get(), std::move(bufs), length, offset,
                           account == DEFAULT_DISK_ACCOUNT
                           ? default_account->get_account()
                           : account->get_account(),
                           callback);
#else  // USE_WRITEV
    // OS X doesn't have pwritev.  Using lseek followed by a writev would
    // require adding a mutex for OS X.  We simply break up the writes into
    // separate write calls.

    struct intermediate_cb_t : public linux_iocallback_t {
        void on_io_complete() {
            guarantee(refcount > 0);
            --refcount;
            if (refcount == 0) {
                linux_iocallback_t *local_cb = cb;
                delete this;
                local_cb->on_io_complete();
            }
        }

        size_t refcount;
        linux_iocallback_t *cb;
    };

    intermediate_cb_t *intermediate_cb = new intermediate_cb_t;
    // Hold a refcount while we launch writes.
    intermediate_cb->refcount = 1;
    intermediate_cb->cb = callback;

    int64_t partial_offset = offset;
    for (size_t i = 0; i < bufs.size(); ++i) {
        ++intermediate_cb->refcount;
        diskmgr->submit_write(fd.get(), bufs[i].iov_base, bufs[i].iov_len,
                              partial_offset, account == DEFAULT_DISK_ACCOUNT
                              ? default_account->get_account()
                              : account->get_account(),
                              intermediate_cb,
                              false);
        partial_offset += bufs[i].iov_len;
    }
    guarantee(partial_offset - offset == static_cast<int64_t>(length));

    // Release its refcount.
    intermediate_cb->on_io_complete();
#endif  // USE_WRITEV

}

bool linux_file_t::coop_lock_and_check() {
#ifdef _WIN32
    // TODO WINDOWS
    return true;
#else
    if (flock(fd.get(), LOCK_EX | LOCK_NB) != 0) {
        rassert(get_errno() == EWOULDBLOCK);
        return false;
    }
    return true;
#endif
}

void *linux_file_t::create_account(int priority, int outstanding_requests_limit) {
    assert_thread();
    return diskmgr->create_account(priority, outstanding_requests_limit);
}

void linux_file_t::destroy_account(void *account) {
    assert_thread();
    diskmgr->destroy_account(account);
}



linux_file_t::~linux_file_t() {
    // scoped_fd_t's destructor takes care of close()ing the file
}

void verify_aligned_file_access(DEBUG_VAR int64_t file_size, DEBUG_VAR int64_t offset,
                                DEBUG_VAR size_t length,
                                DEBUG_VAR const scoped_array_t<iovec> &bufs) {
#ifndef NDEBUG
    rassert(static_cast<int64_t>(offset + length) <= file_size);
    rassert(divides(DEVICE_BLOCK_SIZE, offset));
    rassert(divides(DEVICE_BLOCK_SIZE, length));

    size_t sum = 0;
    for (size_t i = 0; i < bufs.size(); ++i) {
        rassert(divides(DEVICE_BLOCK_SIZE, bufs[i].iov_len));
        rassert(divides(DEVICE_BLOCK_SIZE, reinterpret_cast<intptr_t>(bufs[i].iov_base)));
        sum += bufs[i].iov_len;
    }
    rassert(sum == length);
#endif  // NDEBUG
}

void verify_aligned_file_access(DEBUG_VAR int64_t file_size, DEBUG_VAR int64_t offset,
                                DEBUG_VAR size_t length, DEBUG_VAR const void *buf) {
    rassert(buf);
    rassert(static_cast<int64_t>(offset + length) <= file_size);
    rassert(divides(DEVICE_BLOCK_SIZE, reinterpret_cast<intptr_t>(buf)));
    rassert(divides(DEVICE_BLOCK_SIZE, offset));
    rassert(divides(DEVICE_BLOCK_SIZE, length));
}

file_open_result_t open_file(const char *path, const int mode, io_backender_t *backender,
                             scoped_ptr_t<file_t> *out) {
    scoped_fd_t fd;

#ifdef _WIN32
    DWORD create_mode;
    if (mode & linux_file_t::mode_truncate) {
        create_mode = CREATE_ALWAYS;
    } else if (mode & linux_file_t::mode_create) {
        create_mode = OPEN_ALWAYS;
    } else {
        create_mode = OPEN_EXISTING;
    }

    DWORD access_mode = 0;
    if (mode & linux_file_t::mode_write) {
        access_mode |= GENERIC_WRITE;
    }
    if (mode & linux_file_t::mode_read) {
        access_mode |= GENERIC_READ;
    }
    if (access_mode == 0) {
        crash("Bad file access mode.");
    }

    // TODO WINDOWS: is all this sharing necessary?
    DWORD share_mode = FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE;

    DWORD attributes = FILE_ATTRIBUTE_NORMAL;

    // Supporting fully unbuffered file i/o on Windows would require
    // aligning all reads and writes to the sector size of the volume
    // (See docs for FILE_FLAG_NO_BUFFERING).
    //
    // Instead we only use FILE_FLAG_WRITE_THROUGH
    DWORD flags =
        backender->get_direct_io_mode() == file_direct_io_mode_t::direct_desired
        ? FILE_FLAG_WRITE_THROUGH
        : 0;

    fd.reset(CreateFile(path, access_mode, share_mode, nullptr, create_mode, flags | attributes, nullptr));
    if (fd.get() == INVALID_FD) {
        logERR("CreateFile failed: %s: %s", path, winerr_string(GetLastError()).c_str());
        return file_open_result_t(file_open_result_t::ERROR, EIO);
    }

    file_open_result_t open_res = file_open_result_t(flags & FILE_FLAG_WRITE_THROUGH
                                                     ? file_open_result_t::DIRECT
                                                     : file_open_result_t::BUFFERED,
                                                     0);

#else
    // Construct file flags

    // Let's have a sanity check for our attempt to check whether O_DIRECT and O_NOATIME are
    // available as preprocessor defines.
#ifndef O_CREAT
#error "O_CREAT and other open flags are apparently not defined in the preprocessor."
#endif

    int flags = 0;

    if (mode & linux_file_t::mode_create) {
        flags |= O_CREAT;
    }

    // We support file truncation for opening temporary files used when starting up creation of some
    // serializer file that needs some initialization before being put in its permanent place.
    if (mode & linux_file_t::mode_truncate) {
        flags |= O_TRUNC;
    }

    // For now, we have a whitelist of kernels that don't support O_LARGEFILE.  Linux is
    // the only known kernel that has (or may need) the O_LARGEFILE flag.
#ifdef __linux__
    flags |= O_LARGEFILE;
#endif

    if ((mode & linux_file_t::mode_write) && (mode & linux_file_t::mode_read)) {
        flags |= O_RDWR;
    } else if (mode & linux_file_t::mode_write) {
        flags |= O_WRONLY;
    } else if (mode & linux_file_t::mode_read) {
        flags |= O_RDONLY;
    } else {
        crash("Bad file access mode.");
    }

    // Makes writes not update the access time of the file where available.  That's more efficient.
#ifdef O_NOATIME
    flags |= O_NOATIME;
#endif

    // Open the file.

    {
        int res_open;
        do {
            res_open = open(path, flags, 0644);
        } while (res_open == -1 && get_errno() == EINTR);

        fd.reset(res_open);
    }

    if (fd.get() == INVALID_FD) {
        return file_open_result_t(file_open_result_t::ERROR, get_errno());
    }

    // When building, we must either support O_DIRECT or F_NOCACHE.  The former works on Linux,
    // the latter works on OS X.
    file_open_result_t open_res;

    switch (backender->get_direct_io_mode()) {
    case file_direct_io_mode_t::direct_desired: {
#ifdef __linux__
        // fcntl(2) is documented to take an argument of type long, not of type int, with the
        // F_SETFL command, on Linux.  But POSIX says it's supposed to take an int?  Passing long
        // should be generally fine, with either the x86 or amd64 calling convention, on another
        // system (that supports O_DIRECT) but we use "#ifdef __linux__" (and not "#ifdef O_DIRECT")
        // specifically to avoid such concerns.
        const int fcntl_res = fcntl(fd.get(), F_SETFL,
                                    static_cast<long>(flags | O_DIRECT));  // NOLINT(runtime/int)
#elif defined(__APPLE__)
        const int fcntl_res = fcntl(fd.get(), F_NOCACHE, 1);
#elif defined(_WIN32)
        // TODO WINDOWS
        const int fcntl_res = -1;
#else
#error "Figure out how to do direct I/O and fsync correctly (despite your operating system's lies) on your platform."
#endif  // __linux__, defined(__APPLE__)
        open_res = file_open_result_t(fcntl_res == -1 ?
                                      file_open_result_t::BUFFERED_FALLBACK :
                                      file_open_result_t::DIRECT,
                                      0);
    } break;
    case file_direct_io_mode_t::buffered_desired: {
        // We can typically improve read performance by disabling read-ahead.
        // Our access patterns are usually pretty random, and on startup we already
        // do read-ahead internally in our cache.
        int disable_readahead_res = -1;
#ifdef __linux__
        // From the man-page:
        //  Under Linux, POSIX_FADV_NORMAL sets the readahead window to the
        //  default size for the backing device; POSIX_FADV_SEQUENTIAL doubles
        //  this size, and POSIX_FADV_RANDOM disables file readahead entirely.
        //  These changes affect the entire file, not just the specified region
        //  (but other open file handles to the same file are unaffected)
        disable_readahead_res = posix_fadvise(fd.get(), 0, 0, POSIX_FADV_RANDOM);
#elif defined(__APPLE__)
        const int fcntl_res = fcntl(fd.get(), F_RDAHEAD, 0);
        disable_readahead_res = fcntl_res == -1
                                ? get_errno()
                                : 0;
#elif defined(_WIN32)
        // TODO WINDOWS
#endif
        if (disable_readahead_res != 0) {
            // Non-critical error. Just print a warning and keep going.
            logWRN("Failed to disable read-ahead on '%s' (errno %d). You might see "
                   "decreased read performance.", path, disable_readahead_res);
        }

        open_res = file_open_result_t(file_open_result_t::BUFFERED, 0);
    } break;
    default:
        unreachable();
    }
#endif

    const int64_t file_size = get_file_size(fd.get());

    // Call fsync() on the parent directory to guarantee that the newly
    // created file's directory entry is persisted to disk.
    warn_fsync_parent_directory(path);

    out->init(new linux_file_t(std::move(fd), file_size, backender->get_diskmgr_ptr()));

    return open_res;
}

void crash_due_to_inaccessible_database_file(const char *path, file_open_result_t open_res) {
    guarantee(open_res.outcome == file_open_result_t::ERROR);
    fail_due_to_user_error(
        "Inaccessible database file: \"%s\": %s"
        "\nSome possible reasons:"
        "\n- the database file couldn't be created or opened for reading and writing"
        "\n- the user which was used to start the database is not an owner of the file"
        , path, errno_string(open_res.errsv).c_str());
}

// Upon error, returns the errno value.
int perform_datasync(fd_t fd) {
    // On OS X, we use F_FULLFSYNC because fsync lies.  fdatasync is not available.  On
    // Linux we just use fdatasync.

#ifdef __MACH__

    int fcntl_res;
    do {
        fcntl_res = fcntl(fd, F_FULLFSYNC);
    } while (fcntl_res == -1 && get_errno() == EINTR);

    return fcntl_res == -1 ? get_errno() : 0;

#elif defined(_WIN32)

    BOOL res = FlushFileBuffers(fd);
    if (!res) {
        logWRN("FlushFileBuffers failed: %s", winerr_string(GetLastError()).c_str());
        return EIO;
    }
    return 0;

#elif defined(__linux__)

    int res = fdatasync(fd);
    return res == -1 ? get_errno() : 0;

#else
#error "perform_datasync not implemented"
#endif  // __MACH__
}

MUST_USE int fsync_parent_directory(const char *path) {
    // Locate the parent directory
#ifdef _WIN32
    // TODO WINDOWS
    (void) path;
    return 0;
#else
    char absolute_path[PATH_MAX];
    char *abs_res = realpath(path, absolute_path);
    guarantee_err(abs_res != nullptr, "Failed to determine absolute path for '%s'", path);
    char *parent_path = dirname(absolute_path); // Note: modifies absolute_path

    // Get a file descriptor on the parent directory
    int res;
    do {
        res = open(parent_path, O_RDONLY);
    } while (res == -1 && get_errno() == EINTR);
    if (res == -1) {
        return get_errno();
    }
    scoped_fd_t fd(res);

    do {
        res = fsync(fd.get());
    } while (res == -1 && get_errno() == EINTR);
    if (res == -1) {
        return get_errno();
    }

    return 0;
#endif
}

void warn_fsync_parent_directory(const char *path) {
    int sync_res = fsync_parent_directory(path);
    if (sync_res != 0) {
        logWRN("Failed to sync parent directory of \"%s\" (errno: %d - %s). "
               "You may encounter data loss in case of a system failure. "
               "(Is the file located on a filesystem that doesn't support directory sync? "
               "e.g. VirtualBox shared folders)",
               path, sync_res, errno_string(sync_res).c_str());
    }
}
