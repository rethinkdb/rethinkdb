// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "arch/io/disk.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>

#include <algorithm>

#include "utils.hpp"
#include <boost/bind.hpp>

#include "arch/types.hpp"
#include "arch/runtime/system_event/eventfd.hpp"
#include "config/args.hpp"
#include "backtrace.hpp"
#include "arch/runtime/runtime.hpp"
#include "arch/io/disk/filestat.hpp"
#include "arch/io/disk/pool.hpp"
#include "arch/io/disk/conflict_resolving.hpp"
#include "arch/io/disk/stats.hpp"
#include "arch/io/disk/accounting.hpp"
#include "do_on_thread.hpp"
#include "logger.hpp"

// TODO: If two files are on the same disk, should they share part of the IO stack?

/* Disk manager object takes care of queueing operations, collecting statistics, preventing
   conflicts, and actually sending them to the disk. */
class linux_disk_manager_t : public home_thread_mixin_t {
    typedef conflict_resolving_diskmgr_t<accounting_diskmgr_t::action_t> conflict_resolver_t;
    typedef stats_diskmgr_t<conflict_resolver_t::action_t> stack_stats_t;

public:
    struct action_t : public stack_stats_t::action_t {
        int cb_thread;
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
        stack_stats.submit_fun = boost::bind(&conflict_resolver_t::submit, &conflict_resolver, _1);
        conflict_resolver.submit_fun = boost::bind(&accounting_diskmgr_t::submit, &accounter, _1);

        /* Hook up everything's `done_fun`. */
        backend.done_fun = boost::bind(&stats_diskmgr_2_t::done, &backend_stats, _1);
        backend_stats.done_fun = boost::bind(&accounting_diskmgr_t::done, &accounter, _1);
        accounter.done_fun = boost::bind(&conflict_resolver_t::done, &conflict_resolver, _1);
        conflict_resolver.done_fun = boost::bind(&stack_stats_t::done, &stack_stats, _1);
        stack_stats.done_fun = boost::bind(&linux_disk_manager_t::done, this, _1);
    }

    ~linux_disk_manager_t() {
        rassert(outstanding_txn == 0, "Closing a file with outstanding txns\n");
    }

    void *create_account(int pri, int outstanding_requests_limit) {
        return new accounting_diskmgr_t::account_t(&accounter, pri, outstanding_requests_limit);
    }

    void delayed_destroy(void *_account) {
        on_thread_t t(home_thread());
        delete static_cast<accounting_diskmgr_t::account_t *>(_account);
    }

    void destroy_account(void *account) {
        coro_t::spawn_sometime(boost::bind(&linux_disk_manager_t::delayed_destroy, this, account));
    }

    void submit_action_to_stack_stats(action_t *a) {
        assert_thread();
        outstanding_txn++;
        stack_stats.submit(a);
    }

    void submit_write(fd_t fd, const void *buf, size_t count, size_t offset, void *account, linux_iocallback_t *cb) {
        int calling_thread = get_thread_id();

        action_t *a = new action_t;
        a->make_write(fd, buf, count, offset);
        a->account = static_cast<accounting_diskmgr_t::account_t*>(account);
        a->cb = cb;
        a->cb_thread = calling_thread;

        do_on_thread(home_thread(), boost::bind(&linux_disk_manager_t::submit_action_to_stack_stats, this, a));
    }

    void submit_read(fd_t fd, void *buf, size_t count, size_t offset, void *account, linux_iocallback_t *cb) {
        int calling_thread = get_thread_id();

        action_t *a = new action_t;
        a->make_read(fd, buf, count, offset);
        a->account = static_cast<accounting_diskmgr_t::account_t*>(account);
        a->cb = cb;
        a->cb_thread = calling_thread;

        do_on_thread(home_thread(), boost::bind(&linux_disk_manager_t::submit_action_to_stack_stats, this, a));
    };

    void done(stack_stats_t::action_t *a) {
        assert_thread();
        outstanding_txn--;
        action_t *a2 = static_cast<action_t *>(a);
        bool succeeded = a2->get_succeeded();
        if (succeeded) {
            do_on_thread(a2->cb_thread, boost::bind(&linux_iocallback_t::on_io_complete, a2->cb));
        } else {
            do_on_thread(a2->cb_thread, boost::bind(&linux_iocallback_t::on_io_failure, a2->cb, a2->get_errno(), static_cast<int64_t>(a2->get_offset()), static_cast<int64_t>(a2->get_count())));
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

    stack_stats_t stack_stats;
    conflict_resolver_t conflict_resolver;
    accounting_diskmgr_t accounter;
    stats_diskmgr_2_t backend_stats;
    pool_diskmgr_t backend;


    int outstanding_txn;

    DISABLE_COPYING(linux_disk_manager_t);
};

io_backender_t::io_backender_t(int max_concurrent_io_requests)
    : diskmgr(new linux_disk_manager_t(&linux_thread_pool_t::thread->queue,
                                       DEFAULT_IO_BATCH_FACTOR,
                                       max_concurrent_io_requests,
                                       &stats)) { }

io_backender_t::~io_backender_t() { }

/* Disk file object */

linux_file_t::linux_file_t(scoped_fd_t &&_fd, uint64_t _file_size, linux_disk_manager_t *_diskmgr)
    : fd(std::move(_fd)), file_size(_file_size), diskmgr(_diskmgr) {
    // TODO: Why do we care whether we're in a thread pool?  (Maybe it's that you can't create a
    // file_account_t outside of the thread pool?  But they're associated with the diskmgr,
    // aren't they?)
    if (linux_thread_pool_t::thread) {
        default_account.init(new file_account_t(this, 1, UNLIMITED_OUTSTANDING_REQUESTS));
    }
}

uint64_t linux_file_t::get_size() {
    return file_size;
}

void linux_file_t::set_size(size_t size) {
    int res;
    do {
        res = ftruncate(fd.get(), size);
    } while (res == -1 && errno == EINTR);
    guarantee_err(res == 0, "Could not ftruncate()");

#if FILE_SYNC_TECHNIQUE == FILE_SYNC_TECHNIQUE_FULLFSYNC

    int fcntl_res;
    do {
        fcntl_res = fcntl(fd.get(), F_FULLFSYNC);
    } while (fcntl_res == -1 && errno == EINTR);
    guarantee_err(fcntl_res == 0, "Could not fsync with fcntl(..., F_FULLFSYNC).");

#elif FILE_SYNC_TECHNIQUE == FILE_SYNC_TECHNIQUE_DSYNC

    // Make sure that the metadata change gets persisted before we start writing to the resized
    // file (to be safe in case of system crashes, etc).
    int fsync_res;
    do {
        fsync_res = fsync(fd.get());
    } while (fsync_res == -1 && errno == EINTR);
    guarantee_err(fsync_res == 0, "Could not fsync.");

#else
#error "Unrecognized FILE_SYNC_TECHNIQUE value.  Did you include the header file?"
#endif  // FILE_SYNC_TECHNIQUE

    file_size = size;
}

void linux_file_t::set_size_at_least(size_t size) {
    /* Grow in large chunks at a time */
    if (file_size < size) {
        set_size(ceil_aligned(size, DEVICE_BLOCK_SIZE * 128));
    }
}

void linux_file_t::read_async(size_t offset, size_t length, void *buf, file_account_t *account, linux_iocallback_t *callback) {
    rassert(diskmgr, "No diskmgr has been constructed (are we running without an event queue?)");
    verify_aligned_file_access(file_size, offset, length, buf);
    diskmgr->submit_read(fd.get(), buf, length, offset,
        account == DEFAULT_DISK_ACCOUNT ? default_account->get_account() : account->get_account(),
        callback);
}

void linux_file_t::write_async(size_t offset, size_t length, const void *buf, file_account_t *account, linux_iocallback_t *callback) {
    rassert(diskmgr, "No diskmgr has been constructed (are we running without an event queue?)");

    verify_aligned_file_access(file_size, offset, length, buf);
    diskmgr->submit_write(fd.get(), buf, length, offset,
        account == DEFAULT_DISK_ACCOUNT ? default_account->get_account() : account->get_account(),
        callback);
}

void linux_file_t::read_blocking(size_t offset, size_t length, void *buf) {
    verify_aligned_file_access(file_size, offset, length, buf);
    ssize_t res;
    do {
        res = pread(fd.get(), buf, length, offset);
    } while (res == -1 && errno == EINTR);

    nice_guarantee(size_t(res) == length, "Blocking read from file failed. Exiting.");
}

void linux_file_t::write_blocking(size_t offset, size_t length, const void *buf) {
    verify_aligned_file_access(file_size, offset, length, buf);
    ssize_t res;
    do {
        res = pwrite(fd.get(), buf, length, offset);
    } while (res == -1 && errno == EINTR);

    nice_guarantee(size_t(res) == length, "Blocking write from file failed. Exiting.");
}

bool linux_file_t::coop_lock_and_check() {
    if (flock(fd.get(), LOCK_EX | LOCK_NB) != 0) {
        rassert(errno == EWOULDBLOCK);
        return false;
    }
    return true;
}

void *linux_file_t::create_account(int priority, int outstanding_requests_limit) {
    return diskmgr->create_account(priority, outstanding_requests_limit);
}

void linux_file_t::destroy_account(void *account) {
    diskmgr->destroy_account(account);
}



linux_file_t::~linux_file_t() {
    // scoped_fd_t's destructor takes care of close()ing the file
}

void verify_aligned_file_access(DEBUG_VAR size_t file_size, DEBUG_VAR size_t offset, DEBUG_VAR size_t length, DEBUG_VAR const void *buf) {
    rassert(buf);
    rassert(offset + length <= file_size);
    rassert(divides(DEVICE_BLOCK_SIZE, intptr_t(buf)));
    rassert(divides(DEVICE_BLOCK_SIZE, offset));
    rassert(divides(DEVICE_BLOCK_SIZE, length));
}

file_open_result_t open_direct_file(const char *path, int mode, io_backender_t *backender, scoped_ptr_t<file_t> *out) {
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

    // For now, we have a whitelist of kernels that don't support O_LARGEFILE (or O_DSYNC, for that
    // matter).  Linux is the only known kernel that has (or may need) the O_LARGEFILE flag.
#ifndef __MACH__
    flags |= O_LARGEFILE;
#endif

#ifndef FILE_SYNC_TECHNIQUE
#error "FILE_SYNC_TECHNIQUE is not defined"
#elif FILE_SYNC_TECHNIQUE == FILE_SYNC_TECHNIQUE_DSYNC
    flags |= O_DSYNC;
#endif  // FILE_SYNC_TECHNIQUE

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

    scoped_fd_t fd;
    {
        int res_open;
        do {
            res_open = open(path, flags, 0644);
        } while (res_open == -1 && errno == EINTR);

        fd.reset(res_open);
    }

    if (fd.get() == INVALID_FD) {
        return file_open_result_t(file_open_result_t::ERROR, errno);
    }

    // When building, we must either support O_DIRECT or F_NOCACHE.  The former works on Linux,
    // the latter works on OS X.
#ifdef __linux__
    // fcntl(2) is documented to take an argument of type long, not of type int, with the F_SETFL
    // command, on Linux.  But POSIX says it's supposed to take an int?  Passing long should be
    // generally fine, with either the x86 or amd64 calling convention, on another system (that
    // supports O_DIRECT) but we use "#ifdef __linux__" (and not "#ifdef O_DIRECT") specifically to
    // avoid such concerns.
    int fcntl_res = fcntl(fd.get(), F_SETFL, static_cast<long>(flags | O_DIRECT));  // NOLINT(runtime/int)
#elif defined(__APPLE__)
    int fcntl_res = fcntl(fd.get(), F_NOCACHE, 1);
#else
#error "Figure out how to do direct I/O and fsync correctly (despite your operating system's lies) on your platform."
#endif  // __linux__, defined(__APPLE__)

    file_open_result_t open_res = (fcntl_res == -1 ? file_open_result_t(file_open_result_t::BUFFERED, 0) : file_open_result_t(file_open_result_t::DIRECT, 0));

    uint64_t file_size = get_file_size(fd.get());

    // TODO: We have a very minor correctness issue here, which is that
    // we don't guarantee data durability for newly created database files.
    // In theory, we would have to fsync() not only the file itself, but
    // also the directory containing it. Otherwise the file might not
    // survive a crash of the system. However, the window for data to get lost
    // this way is just a few seconds after the creation of a new database,
    // until the file system flushes the metadata to disk.

    out->init(new linux_file_t(std::move(fd), file_size, backender->get_diskmgr_ptr()));

    return open_res;
}

void crash_due_to_inaccessible_database_file(const char *path, file_open_result_t open_res) {
    guarantee(open_res.outcome == file_open_result_t::ERROR);
    fail_due_to_user_error(
        "Inaccessible database file: \"%s\": %s"
        "\nSome possible reasons:"
        "\n- the database file couldn't be created or opened for reading and writing"
#ifdef O_NOATIME
        "\n- the user which was used to start the database is not an owner of the file"
#endif
        , path, errno_string(open_res.errsv).c_str());
}
