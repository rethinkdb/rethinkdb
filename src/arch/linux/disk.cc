#include "errors.hpp"
#include "utils.hpp"
#include <algorithm>
#include <fcntl.h>
#include <linux/fs.h>
#include <stdio.h>
#include <unistd.h>
#include "arch/linux/system_event/eventfd.hpp"
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include "arch/linux/arch.hpp"
#include "config/args.hpp"
#include "utils2.hpp"
#include "arch/linux/coroutines.hpp"
#include "logger.hpp"
#include "arch/linux/disk/aio.hpp"
#include "arch/linux/disk/pool.hpp"
#include "arch/linux/disk/conflict_resolving.hpp"
#include "arch/linux/disk/stats.hpp"
#include "arch/linux/disk/accounting.hpp"

// #define DEBUG_DUMP_WRITES 1

perfmon_duration_sampler_t
    pm_io_disk_stack_reads("io_disk_stack_reads", secs_to_ticks(1)),
    pm_io_disk_stack_writes("io_disk_stack_writes", secs_to_ticks(1));

perfmon_duration_sampler_t
    pm_io_disk_backend_reads("io_disk_backend_reads", secs_to_ticks(1), false),
    pm_io_disk_backend_writes("io_disk_backend_writes", secs_to_ticks(1), false);

/* Disk manager object takes care of queueing operations, collecting statistics, preventing
conflicts, and actually sending them to the disk. Defined as an abstract class so that different
actual implementations can be swapped in at runtime. */

// TODO: If two files are on the same disk, should they share part of the IO stack?

struct linux_disk_manager_t {
    virtual ~linux_disk_manager_t() { }

    virtual void *create_account(int priority, int outstanding_requests_limit) = 0;
    virtual void destroy_account(void *account) = 0;

    virtual void submit_write(fd_t fd, const void *buf, size_t count, size_t offset,
        void *account, linux_iocallback_t *cb) = 0;
    virtual void submit_read(fd_t fd, void *buf, size_t count, size_t offset,
        void *account, linux_iocallback_t *cb) = 0;
};

template<class backend_t>
struct linux_templated_disk_manager_t : public linux_disk_manager_t {

    typedef stats_diskmgr_2_t<typename backend_t::action_t> backend_stats_t;
    typedef accounting_diskmgr_t<typename backend_stats_t::action_t> accounter_t;
    typedef conflict_resolving_diskmgr_t<typename accounter_t::action_t> conflict_resolver_t;
    typedef stats_diskmgr_t<typename conflict_resolver_t::action_t> stack_stats_t;
    struct action_t : public stack_stats_t::action_t {
        linux_iocallback_t *cb;
    };

    /* These fields describe the entire IO stack. At the top level, we allocate a new
    action_t object for each operation and record its callback. Then it passes through
    the conflict resolver, which enforces ordering constraints between IO operations by
    holding back operations that must be run after other, currently-running, operations.
    Then it goes to the account manager, which queues up running IO operations according
    to which account they are part of. Finally the backend (which may be either AIO-based
    or thread-pool-based) pops the IO operations from the queue.

    At two points in the process--once as soon as it is submitted, and again right
    as the backend pops it off the queue--its statistics are recorded. The "stack stats"
    will tell you how many IO operations are queued. The "backend stats" will tell you
    how long the OS takes to perform the operations. Note that it's not perfect, because
    it counts operations that have been queued by the backend but not sent to the OS yet
    as having been sent to the OS. */

    stack_stats_t stack_stats;
    conflict_resolver_t conflict_resolver;
    accounter_t accounter;
    backend_stats_t backend_stats;
    backend_t backend;

    linux_templated_disk_manager_t(linux_event_queue_t *queue, const int batch_factor) :
        stack_stats(&pm_io_disk_stack_reads, &pm_io_disk_stack_writes),
        conflict_resolver(),
        accounter(batch_factor),
        backend_stats(&pm_io_disk_backend_reads, &pm_io_disk_backend_writes, accounter.producer),
        backend(queue, backend_stats.producer),
        outstanding_txn(0)
    {
        /* Hook up the `submit_fun`s of the parts of the IO stack that are above the
        queue. (The parts below the queue use the `passive_producer_t` interface instead
        of a callback function.) */
        stack_stats.submit_fun = boost::bind(&conflict_resolver_t::submit, &conflict_resolver, _1);
        conflict_resolver.submit_fun = boost::bind(&accounter_t::submit, &accounter, _1);

        /* Hook up everything's `done_fun`. */
        backend.done_fun = boost::bind(&backend_stats_t::done, &backend_stats, _1);
        backend_stats.done_fun = boost::bind(&accounter_t::done, &accounter, _1);
        accounter.done_fun = boost::bind(&conflict_resolver_t::done, &conflict_resolver, _1);
        conflict_resolver.done_fun = boost::bind(&stack_stats_t::done, &stack_stats, _1);
        stack_stats.done_fun = boost::bind(&linux_templated_disk_manager_t<backend_t>::done, this, _1);
    }

    ~linux_templated_disk_manager_t() {
        rassert(outstanding_txn == 0, "Closing a file with outstanding txns\n");
    }

    void *create_account(int pri, int outstanding_requests_limit) {
        return reinterpret_cast<void*>(new typename accounter_t::account_t(&accounter, pri, outstanding_requests_limit));
    }

    void destroy_account(void *account) {
        delete reinterpret_cast<typename accounter_t::account_t*>(account);
    }

    void submit_write(fd_t fd, const void *buf, size_t count, size_t offset, void *account, linux_iocallback_t *cb) {
        outstanding_txn++;
        action_t *a = new action_t;
        a->make_write(fd, buf, count, offset);
        a->account = reinterpret_cast<typename accounter_t::account_t*>(account);
        a->cb = cb;
        stack_stats.submit(a);
    }

    void submit_read(fd_t fd, void *buf, size_t count, size_t offset, void *account, linux_iocallback_t *cb) {
        outstanding_txn++;
        action_t *a = new action_t;
        a->make_read(fd, buf, count, offset);
        a->account = reinterpret_cast<typename accounter_t::account_t*>(account);
        a->cb = cb;
        stack_stats.submit(a);
    };

    void done(typename stack_stats_t::action_t *a) {
        outstanding_txn--;
        action_t *a2 = static_cast<action_t *>(a);
        a2->cb->on_io_complete();
        delete a2;
    }

private:
    int outstanding_txn;
};

/* Disk account object */

linux_file_t::account_t::account_t(linux_file_t *par, int pri, int outstanding_requests_limit) :
    parent(par), account(parent->diskmgr->create_account(pri, outstanding_requests_limit))
    { }

linux_file_t::account_t::~account_t() {
    parent->diskmgr->destroy_account(account);
}

/* Disk file object */

linux_file_t::linux_file_t(const char *path, int mode, bool is_really_direct, const linux_io_backend_t io_backend, const int batch_factor)
    : fd(INVALID_FD), file_size(0)
{
    // Determine if it is a block device

    struct stat64 file_stat;
    bzero((void*)&file_stat, sizeof(file_stat)); // make valgrind happy
    int res = stat64(path, &file_stat);
    guarantee_err(res == 0 || errno == ENOENT, "Could not stat file '%s'", path);

    if (res == -1 && errno == ENOENT) {
        if (!(mode & mode_create)) {
            file_exists = false;
            return;
        }
        is_block = false;
    } else {
        is_block = S_ISBLK(file_stat.st_mode);
    }

    // Construct file flags

    int flags = O_CREAT | (is_really_direct ? O_DIRECT : 0) | O_LARGEFILE;

    if ((mode & mode_write) && (mode & mode_read)) flags |= O_RDWR;
    else if (mode & mode_write) flags |= O_WRONLY;
    else if (mode & mode_read) flags |= O_RDONLY;
    else crash("Bad file access mode.");


    // O_NOATIME requires owner or root privileges. This is a bit of a hack; we assume that
    // if we are opening a regular file, we are the owner, but if we are opening a block device,
    // we are not.
    if (!is_block) flags |= O_NOATIME;

    // Open the file

    fd.reset(open(path, flags, 0644));
    if (fd.get() == INVALID_FD) {
        fail_due_to_user_error(
            "Inaccessible database file: \"%s\": %s"
            "\nSome possible reasons:"
            "%s"    // for creation/open error
            "%s"    // for O_DIRECT message
            "%s",   // for O_NOATIME message
            path, strerror(errno),
            is_block ?
                "\n- the database device couldn't be opened for reading and writing" :
                "\n- the database file couldn't be created or opened for reading and writing",
            is_really_direct ?
                (is_block ?
                    "\n- the database block device cannot be opened with O_DIRECT flag" :
                    "\n- the database file is located on a filesystem that doesn't support O_DIRECT open flag (e.g. in case when the filesystem is working in journaled mode)"
                ) : "",
            !is_block ? "\n- user which was used to start the database is not an owner of the file" : ""
        );
    }

    file_exists = true;

    // Determine the file size

    if (is_block) {
        res = ioctl(fd.get(), BLKGETSIZE64, &file_size);
        guarantee_err(res != -1, "Could not determine block device size");
    } else {
        off64_t size = lseek64(fd.get(), 0, SEEK_END);
        guarantee_err(size != -1, "Could not determine file size");
        res = lseek64(fd.get(), 0, SEEK_SET);
        guarantee_err(res != -1, "Could not reset file position");

        file_size = size;
    }

    // TODO: We have a very minor correctness issue here, which is that
    // we don't guarantee data durability for newly created database files.
    // In theory, we would have to fsync() not only the file itself, but
    // also the directory containing it. Otherwise the file might not
    // survive a crash of the system. However, the window for data to get lost
    // this way is just a few seconds after the creation of a new database,
    // until the file system flushes the metadata to disk.

    // Construct a disk manager. (given that we have an event pool)
    if (linux_thread_pool_t::thread) {
        linux_event_queue_t *queue = &linux_thread_pool_t::thread->queue;
        if (io_backend == aio_native) {
            diskmgr.reset(new linux_templated_disk_manager_t<linux_diskmgr_aio_t>(queue, batch_factor));
        } else {
            diskmgr.reset(new linux_templated_disk_manager_t<pool_diskmgr_t>(queue, batch_factor));
        }

        default_account.reset(new account_t(this, 1, UNLIMITED_OUTSTANDING_REQUESTS));
    }
}

bool linux_file_t::exists() {
    return file_exists;
}

bool linux_file_t::is_block_device() {
    return is_block;
}

size_t linux_file_t::get_size() {
    return file_size;
}

void linux_file_t::set_size(size_t size) {
    rassert(!is_block);
    int res = ftruncate(fd.get(), size);
    guarantee_err(res == 0, "Could not ftruncate()");
    fsync(fd.get()); // Make sure that the metadata change gets persisted
                     // before we start writing to the resized file.
                     // (to be safe in case of system crashes etc.)
    file_size = size;
}

void linux_file_t::set_size_at_least(size_t size) {
    if (is_block) {
        rassert(file_size >= size);
    } else {
        /* Grow in large chunks at a time */
        if (file_size < size) {
            // TODO: we should make the growth rate of a db file
            // configurable.
            set_size(ceil_aligned(size, DEVICE_BLOCK_SIZE * 128));
        }
    }
}

bool linux_file_t::read_async(size_t offset, size_t length, void *buf, account_t *account, linux_iocallback_t *callback) {
    rassert(diskmgr, "No diskmgr has been constructed (are we running without an event queue?)");

    verify(offset, length, buf);
    diskmgr->submit_read(fd.get(), buf, length, offset,
        account == DEFAULT_DISK_ACCOUNT ? default_account->account : account->account,
        callback);

    return false;
}

bool linux_file_t::write_async(size_t offset, size_t length, const void *buf, account_t *account, linux_iocallback_t *callback) {
    rassert(diskmgr, "No diskmgr has been constructed (are we running without an event queue?)");

#ifdef DEBUG_DUMP_WRITES
    printf("--- WRITE BEGIN ---\n");
    print_backtrace(stdout);
    printf("\n");
    print_hd(buf, offset, length);
    printf("---- WRITE END ----\n\n");
#endif

    verify(offset, length, buf);
    diskmgr->submit_write(fd.get(), buf, length, offset,
        account == DEFAULT_DISK_ACCOUNT ? default_account->account : account->account,
        callback);

    return false;
}

void linux_file_t::read_blocking(size_t offset, size_t length, void *buf) {
    verify(offset, length, buf);
 tryagain:
    ssize_t res = pread(fd.get(), buf, length, offset);
    if (res == -1 && errno == EINTR) {
        goto tryagain;
    }
    rassert(size_t(res) == length, "Blocking read failed");
    (void)res;
}

void linux_file_t::write_blocking(size_t offset, size_t length, const void *buf) {
    verify(offset, length, buf);
 tryagain:
    ssize_t res = pwrite(fd.get(), buf, length, offset);
    if (res == -1 && errno == EINTR) {
        goto tryagain;
    }
    rassert(size_t(res) == length, "Blocking write failed");
    (void)res;
}

linux_file_t::~linux_file_t() {
    // scoped_fd_t's destructor takes care of close()ing the file
}

void linux_file_t::verify(UNUSED size_t offset, UNUSED size_t length, UNUSED const void *buf) {
    rassert(buf);
    rassert(offset + length <= file_size);
#ifndef NDEBUG
    bool bufmod = (intptr_t)buf % DEVICE_BLOCK_SIZE == 0;
    rassert(bufmod);
    bool offmod = offset % DEVICE_BLOCK_SIZE == 0;
    rassert(offmod);
    bool lengthmod = length % DEVICE_BLOCK_SIZE == 0;
    rassert(lengthmod);
#endif
}
