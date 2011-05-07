#ifndef __ARCH_LINUX_DISK_HPP__
#define __ARCH_LINUX_DISK_HPP__

#include <libaio.h>
#include <vector>
#include "utils2.hpp"
#include <boost/scoped_ptr.hpp>
#include "config/args.hpp"
#include "arch/linux/event_queue.hpp"

/* Types of IO backends */
enum linux_io_backend_t {
    aio_native, aio_pool
};

struct linux_iocallback_t {
    virtual ~linux_iocallback_t() {}
    virtual void on_io_complete() = 0;
};

struct linux_disk_manager_t;

#define DEFAULT_DISK_ACCOUNT NULL

class linux_file_t {
public:
    enum mode_t {
        mode_read = 1 << 0,
        mode_write = 1 << 1,
        mode_create = 1 << 2
    };

    struct account_t {
        account_t(linux_file_t *f, int p);
        ~account_t();
    private:
        friend class linux_file_t;
        linux_file_t *parent;
        /* account is internally a pointer to a accounting_diskmgr_t::account_t object. It has to be
        a void* because accounting_diskmgr_t is a template, so its actual type depends on what
        IO backend is chosen. */
        void *account;
    };

    linux_file_t(const char *path, int mode, bool is_really_direct, const linux_io_backend_t io_backend);

    bool exists();
    bool is_block_device();
    uint64_t get_size();
    void set_size(size_t size);
    void set_size_at_least(size_t size);

    /* These always return 'false'; the reason they return bool instead of void
    is for consistency with other asynchronous-callback methods */
    bool read_async(size_t offset, size_t length, void *buf, account_t *account, linux_iocallback_t *cb);
    bool write_async(size_t offset, size_t length, const void *buf, account_t *account, linux_iocallback_t *cb);

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
    boost::scoped_ptr<account_t> default_account;

    DISABLE_COPYING(linux_file_t);
};

/* The "direct" in linux_direct_file_t refers to the fact that the
file is opened in O_DIRECT mode, and there are restrictions on the
alignment of the chunks being written and read to and from the file. */
class linux_direct_file_t : public linux_file_t {
public:
    linux_direct_file_t(const char *path, int mode, const linux_io_backend_t io_backend = aio_native) :
        linux_file_t(path, mode, true, io_backend) { }

private:
    DISABLE_COPYING(linux_direct_file_t);
};

class linux_nondirect_file_t : public linux_file_t {
public:
    linux_nondirect_file_t(const char *path, int mode, const linux_io_backend_t io_backend = aio_native) :
        linux_file_t(path, mode, false, io_backend) { }

private:
    DISABLE_COPYING(linux_nondirect_file_t);
};

#endif // __ARCH_LINUX_DISK_HPP__

