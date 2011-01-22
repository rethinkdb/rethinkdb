
#ifndef __ARCH_LINUX_DISK_HPP__
#define __ARCH_LINUX_DISK_HPP__

#include <libaio.h>
#include <vector>
#include "utils2.hpp"
#include "config/args.hpp"
#include "arch/linux/event_queue.hpp"
#include "event.hpp"

/* The "direct" in linux_direct_file_t refers to the fact that the file is opened in
O_DIRECT mode, and there are restrictions on the alignment of the chunks being written
and read to and from the file. */

struct linux_iocallback_t {
    virtual ~linux_iocallback_t() {}
    virtual void on_io_complete(event_t *event) = 0;
};

class linux_direct_file_t {
public:
    enum mode_t {
        mode_read = 1 << 0,
        mode_write = 1 << 1,
        mode_create = 1 << 2
    };
    
    linux_direct_file_t(const char *path, int mode, bool is_really_direct = true);
    
    bool exists();
    bool is_block_device();
    uint64_t get_size();
    void set_size(size_t size);
    void set_size_at_least(size_t size);
    
    /* These always return 'false'; the reason they return bool instead of void
    is for consistency with other asynchronous-callback methods */
    bool read_async(size_t offset, size_t length, void *buf, linux_iocallback_t *cb);
    bool write_async(size_t offset, size_t length, void *buf, linux_iocallback_t *cb);
    
    void read_blocking(size_t offset, size_t length, void *buf);
    void write_blocking(size_t offset, size_t length, void *buf);
    
    ~linux_direct_file_t();
    
private:
    fd_t fd;
    bool is_block;
    bool file_exists;
    uint64_t file_size;
    void verify(size_t offset, size_t length, void* buf);

    DISABLE_COPYING(linux_direct_file_t);
};

// Older kernels that don't support eventfd require a shittier
// implementation of linux_io_calls_t because we can't find out about
// AIO notifications in the standard epoll loop.
#ifdef NO_EVENTFD
#include "arch/linux/disk/linux_io_calls_no_eventfd.hpp"
typedef linux_io_calls_no_eventfd_t linux_io_calls_t;
#else
#include "arch/linux/disk/linux_io_calls_eventfd.hpp"
typedef linux_io_calls_eventfd_t linux_io_calls_t;
#endif

#endif // __ARCH_LINUX_DISK_HPP__

