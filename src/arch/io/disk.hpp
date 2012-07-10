#ifndef ARCH_IO_DISK_HPP_
#define ARCH_IO_DISK_HPP_

#include "utils.hpp"
#include <boost/scoped_ptr.hpp>

#include "config/args.hpp"
#include "arch/io/io_utils.hpp"
#include "arch/runtime/event_queue.hpp"
#include "arch/types.hpp"

class linux_iocallback_t;
struct linux_disk_manager_t;

class linux_file_t;

class perfmon_collection_t;

/* Disk manager object takes care of queueing operations, collecting statistics, preventing
   conflicts, and actually sending them to the disk. */
struct linux_disk_manager_t {
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


class io_backender_t {
public:
    virtual void make_disk_manager(linux_event_queue_t *queue, const int batch_factor,
                                   perfmon_collection_t *stats,
                                   boost::scoped_ptr<linux_disk_manager_t> *out) = 0;

    virtual ~io_backender_t() { }

protected:
    io_backender_t() { }

private:
    DISABLE_COPYING(io_backender_t);
};

class native_io_backender_t : public io_backender_t {
public:
    void make_disk_manager(linux_event_queue_t *queue, const int batch_factor,
                           perfmon_collection_t *stats,
                           boost::scoped_ptr<linux_disk_manager_t> *out);
};

class pool_io_backender_t : public io_backender_t {
public:
    void make_disk_manager(linux_event_queue_t *queue, const int batch_factor,
                           perfmon_collection_t *stats,
                           boost::scoped_ptr<linux_disk_manager_t> *out);
};

void make_io_backender(io_backend_t backend, boost::scoped_ptr<io_backender_t> *out);

class linux_file_account_t {
public:
    linux_file_account_t(linux_file_t *f, int p, int outstanding_requests_limit = UNLIMITED_OUTSTANDING_REQUESTS);
    ~linux_file_account_t();
private:
    friend class linux_file_t;
    linux_file_t *parent;
    /* account is internally a pointer to a accounting_diskmgr_t::account_t object. It has to be
       a void* because accounting_diskmgr_t is a template, so its actual type depends on what
       IO backend is chosen. */
    // Maybe accounting_diskmgr_t shouldn't be a templated class then.
    void *account;
};

class linux_file_t {
public:
    friend class linux_file_account_t;

    enum mode_t {
        mode_read = 1 << 0,
        mode_write = 1 << 1,
        mode_create = 1 << 2
    };

    linux_file_t(const char *path, int mode, bool is_really_direct, perfmon_collection_t *stats, io_backender_t *io_backender, const int batch_factor);

    bool exists();
    bool is_block_device();
    uint64_t get_size();
    void set_size(size_t size);
    void set_size_at_least(size_t size);

    void read_async(size_t offset, size_t length, void *buf, linux_file_account_t *account, linux_iocallback_t *cb);
    void write_async(size_t offset, size_t length, const void *buf, linux_file_account_t *account, linux_iocallback_t *cb);

    void read_blocking(size_t offset, size_t length, void *buf);
    void write_blocking(size_t offset, size_t length, const void *buf);

    ~linux_file_t();

private:
    scoped_fd_t fd;
    bool is_block;
    bool file_exists;
    uint64_t file_size;
    void verify(size_t offset, size_t length, const void *buf);

    /* In a scoped pointer because it's polymorphic */
    boost::scoped_ptr<linux_disk_manager_t> diskmgr;

    /* In a scoped_ptr so we can initialize it after "diskmgr" */
    boost::scoped_ptr<linux_file_account_t> default_account;

    DISABLE_COPYING(linux_file_t);
};

/* The "direct" in linux_direct_file_t refers to the fact that the
file is opened in O_DIRECT mode, and there are restrictions on the
alignment of the chunks being written and read to and from the file. */
class linux_direct_file_t : public linux_file_t {
public:
    linux_direct_file_t(const char *path, int mode, perfmon_collection_t *stats, io_backender_t *io_backender, const int batch_factor = DEFAULT_IO_BATCH_FACTOR) :
        linux_file_t(path, mode, true, stats, io_backender, batch_factor) { }

private:
    DISABLE_COPYING(linux_direct_file_t);
};

class linux_nondirect_file_t : public linux_file_t {
public:
    linux_nondirect_file_t(const char *path, int mode, perfmon_collection_t *stats, io_backender_t *io_backender, const int batch_factor = DEFAULT_IO_BATCH_FACTOR) :
        linux_file_t(path, mode, false, stats, io_backender, batch_factor) { }

private:
    DISABLE_COPYING(linux_nondirect_file_t);
};

#endif // ARCH_IO_DISK_HPP_

