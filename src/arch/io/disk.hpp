// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef ARCH_IO_DISK_HPP_
#define ARCH_IO_DISK_HPP_

#include "arch/io/io_utils.hpp"
#include "arch/types.hpp"
#include "concurrency/auto_drainer.hpp"
#include "containers/scoped.hpp"
#include "utils.hpp"

#include "perfmon/core.hpp"

// The maximum concurrent IO requests per event queue.. the default value.
#define DEFAULT_MAX_CONCURRENT_IO_REQUESTS 64

// The maximum user-specifiable value how many concurrent I/O requests may be done per event
// queue.  (A million is a ridiculously high value, but also safely nowhere near INT_MAX.)
#define MAXIMUM_MAX_CONCURRENT_IO_REQUESTS MILLION

struct iovec;

class linux_iocallback_t;

class linux_disk_manager_t;

class io_backender_t : public home_thread_mixin_debug_only_t {
public:
    // This takes what is effectively a global flag whether to use O_DIRECT here.  Nothing technical
    // stops us from specifying this on a file-by-file basis, but right now there's no desire for
    // that.  See https://github.com/rethinkdb/rethinkdb/issues/97#issuecomment-19778177 .
    io_backender_t(file_direct_io_mode_t direct_io_mode,
                   int max_concurrent_io_requests = DEFAULT_MAX_CONCURRENT_IO_REQUESTS);
    ~io_backender_t();
    linux_disk_manager_t *get_diskmgr_ptr() { return diskmgr.get(); }
    file_direct_io_mode_t get_direct_io_mode() const;

protected:
    const file_direct_io_mode_t direct_io_mode;
    perfmon_collection_t stats;
    scoped_ptr_t<linux_disk_manager_t> diskmgr;

private:
    DISABLE_COPYING(io_backender_t);
};

// A file_open_result_t is either FILE_OPEN_DIRECT, FILE_OPEN_BUFFERED, or an errno value.
struct file_open_result_t {
    enum outcome_t {
        DIRECT,  // file opened in direct I/O mode
        BUFFERED,  // file deliberately opened in buffered mode
        BUFFERED_FALLBACK,  // attempted direct I/O mode, failed
        ERROR  // error in opening file
    };
    outcome_t outcome;
    int errsv;

    file_open_result_t(outcome_t _outcome, int _errsv) : outcome(_outcome), errsv(_errsv) {
        guarantee(outcome == ERROR || errsv == 0);
    }
    file_open_result_t() : outcome(ERROR), errsv(0) { }
};

class linux_file_t : public file_t, public home_thread_mixin_debug_only_t {
public:
    enum mode_t {
        mode_read = 1 << 0,
        mode_write = 1 << 1,
        mode_create = 1 << 2,
        mode_truncate = 1 << 3
    };

    int64_t get_file_size();
    /* WARNING: resize operations do not wait for other operations to finish (with
    the exception of other resize operations).
    They do block other operations, but are not blocked themselves.
    Only issue a `set_file_size()` with a size smaller than the current one if you can
    make absolutely sure that no ongoing I/O operation wants to access the space
    that gets truncated anymore. */
    void set_file_size(int64_t size);
    void set_file_size_at_least(int64_t size);

    void read_async(int64_t offset, size_t length, void *buf, file_account_t *account, linux_iocallback_t *cb);
    void write_async(int64_t offset, size_t length, const void *buf, file_account_t *account, linux_iocallback_t *cb,
                     wrap_in_datasyncs_t wrap_in_datasyncs);

    // Does not guarantee the atomicity that writev guarantees.
    void writev_async(int64_t offset, size_t length, scoped_array_t<iovec> &&bufs,
                      file_account_t *account, linux_iocallback_t *cb);

    bool coop_lock_and_check();

    void *create_account(int priority, int outstanding_requests_limit);
    void destroy_account(void *account);

    ~linux_file_t();

    fd_t maybe_get_fd() { return fd.get(); }

private:
    linux_file_t(scoped_fd_t &&fd, int64_t file_size, linux_disk_manager_t *diskmgr);
    friend file_open_result_t open_file(const char *path, int mode,
                                        io_backender_t *backender,
                                        scoped_ptr_t<file_t> *out);

    scoped_fd_t fd;
    int64_t file_size;

    linux_disk_manager_t *diskmgr;

    scoped_ptr_t<file_account_t> default_account;

    // Used to make sure we do not destruct the linux_file_t until all file size
    // operations have completed.
    auto_drainer_t file_size_ops_drainer;

    DISABLE_COPYING(linux_file_t);
};

class linux_semantic_checking_file_t : public semantic_checking_file_t {
public:
    explicit linux_semantic_checking_file_t(fd_t fd);

    virtual size_t semantic_blocking_read(void *buf, size_t length);
    virtual size_t semantic_blocking_write(const void *buf, size_t length);

private:
    scoped_fd_t fd_;
};

file_open_result_t open_file(const char *path, int mode,
                             io_backender_t *backender,
                             scoped_ptr_t<file_t> *out);

void crash_due_to_inaccessible_database_file(const char *path, file_open_result_t open_res) NORETURN;

// Runs some assertios to make sure that we're aligned to DEVICE_BLOCK_SIZE, not overrunning the
// file size, and that buf is not null.
void verify_aligned_file_access(int64_t file_size, int64_t offset, size_t length, const void *buf);

// Makes blocking syscalls.  Upon error, returns the errno value.
int perform_datasync(fd_t fd);

// Calls fsync() on the parent directory of the given path.
// Returns the errno value in case of an error and 0 otherwise.
MUST_USE int fsync_parent_directory(const char *path);
// Like fsync_parent_directory but warns the user on an error
void warn_fsync_parent_directory(const char *path);

#endif // ARCH_IO_DISK_HPP_

