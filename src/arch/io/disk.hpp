// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_IO_DISK_HPP_
#define ARCH_IO_DISK_HPP_

#include "arch/io/io_utils.hpp"
#include "arch/runtime/event_queue.hpp"
#include "arch/runtime/thread_pool.hpp"
#include "arch/types.hpp"
#include "config/args.hpp"
#include "containers/scoped.hpp"
#include "utils.hpp"

#include "perfmon/core.hpp"

#define FILE_SYNC_TECHNIQUE_DSYNC 1
#define FILE_SYNC_TECHNIQUE_FULLFSYNC 2

#ifdef __MACH__
#define FILE_SYNC_TECHNIQUE FILE_SYNC_TECHNIQUE_FULLFSYNC
#else
#define FILE_SYNC_TECHNIQUE FILE_SYNC_TECHNIQUE_DSYNC
#endif

class linux_iocallback_t;
struct linux_disk_manager_t;

/* Disk manager object takes care of queueing operations, collecting statistics, preventing
   conflicts, and actually sending them to the disk. */


struct linux_disk_manager_t : public home_thread_mixin_t {
    linux_disk_manager_t() { }

    virtual ~linux_disk_manager_t() { }

    virtual void *create_account(int priority, int outstanding_requests_limit) = 0;
    virtual void destroy_account(void *account) = 0;

    virtual void submit_write(fd_t fd, const void *buf, size_t count, size_t offset,
        void *account, linux_iocallback_t *cb) = 0;
    virtual void submit_read(fd_t fd, void *buf, size_t count, size_t offset,
        void *account, linux_iocallback_t *cb) = 0;

private:
    DISABLE_COPYING(linux_disk_manager_t);
};

class io_backender_t : public home_thread_mixin_debug_only_t {
public:
    virtual void make_disk_manager(linux_event_queue_t *queue, const int batch_factor,
                                   perfmon_collection_t *stats,
                                   scoped_ptr_t<linux_disk_manager_t> *out) = 0;

    virtual ~io_backender_t() { }
    linux_disk_manager_t *get_diskmgr_ptr() {return diskmgr.get();}

protected:
    io_backender_t() : queue(&linux_thread_pool_t::thread->queue), batch_factor(DEFAULT_IO_BATCH_FACTOR) {
        //        printf("tst\n");
    }
    linux_event_queue_t *queue;
    int batch_factor;
    perfmon_collection_t stats;
    scoped_ptr_t<linux_disk_manager_t> diskmgr;
private:
    DISABLE_COPYING(io_backender_t);
};

class native_io_backender_t : public io_backender_t {
public:
    native_io_backender_t() { make_disk_manager(queue, batch_factor, &stats, &diskmgr); }
    void make_disk_manager(linux_event_queue_t *queue, const int batch_factor,
                           perfmon_collection_t *stats,
                           scoped_ptr_t<linux_disk_manager_t> *out);
};

class pool_io_backender_t : public io_backender_t {
public:
    pool_io_backender_t() { make_disk_manager(queue, batch_factor, &stats, &diskmgr); }
    void make_disk_manager(linux_event_queue_t *queue, const int batch_factor,
                           perfmon_collection_t *stats,
                           scoped_ptr_t<linux_disk_manager_t> *out);
};

void make_io_backender(io_backend_t backend, scoped_ptr_t<io_backender_t> *out);

// A file_open_result_t is either FILE_OPEN_DIRECT, FILE_OPEN_BUFFERED, or an errno value.
struct file_open_result_t {
    enum outcome_t { DIRECT, BUFFERED, ERROR };
    outcome_t outcome;
    int errsv;

    file_open_result_t(outcome_t _outcome, int _errsv) : outcome(_outcome), errsv(_errsv) {
        guarantee(outcome == ERROR || errsv == 0);
    }
    file_open_result_t() : outcome(ERROR), errsv(0) { }
};

class linux_file_t : public file_t {
public:
    enum mode_t {
        mode_read = 1 << 0,
        mode_write = 1 << 1,
        mode_create = 1 << 2,
        mode_truncate = 1 << 3
    };

    bool is_block_device();
    uint64_t get_size();
    void set_size(size_t size);
    void set_size_at_least(size_t size);

    void read_async(size_t offset, size_t length, void *buf, file_account_t *account, linux_iocallback_t *cb);
    void write_async(size_t offset, size_t length, const void *buf, file_account_t *account, linux_iocallback_t *cb);

    void read_blocking(size_t offset, size_t length, void *buf);
    void write_blocking(size_t offset, size_t length, const void *buf);

    bool coop_lock_and_check();

    void *create_account(int priority, int outstanding_requests_limit);
    void destroy_account(void *account);

    ~linux_file_t();

private:
    linux_file_t(scoped_fd_t *fd, uint64_t file_size, linux_disk_manager_t *diskmgr);
    friend file_open_result_t open_direct_file(const char *path, int mode, io_backender_t *backender, scoped_ptr_t<file_t> *out);

    scoped_fd_t fd;
    uint64_t file_size;

    linux_disk_manager_t *diskmgr;

    scoped_ptr_t<file_account_t> default_account;

    DISABLE_COPYING(linux_file_t);
};

file_open_result_t open_direct_file(const char *path, int mode, io_backender_t *backender, scoped_ptr_t<file_t> *out);

void crash_due_to_inaccessible_database_file(const char *path, file_open_result_t open_res) NORETURN;

// Runs some assertios to make sure that we're aligned to DEVICE_BLOCK_SIZE, not overrunning the
// file size, and that buf is not null.
void verify_aligned_file_access(size_t file_size, size_t offset, size_t length, const void *buf);

#endif // ARCH_IO_DISK_HPP_

