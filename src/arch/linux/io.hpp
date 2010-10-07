
#ifndef __IO_CALLS_HPP__
#define __IO_CALLS_HPP__

#include <libaio.h>
#include <vector>
#include "utils.hpp"
#include "config/args.hpp"
#include "config/alloc.hpp"
#include "arch/resource.hpp"
#include "arch/linux/event_queue.hpp"
#include "event.hpp"
#include "corefwd.hpp"

struct linux_event_queue_t;

struct linux_iocallback_t {
    virtual ~linux_iocallback_t() {}
    virtual void on_io_complete(event_t *event) = 0;
};

struct linux_net_conn_callback_t {
    virtual void on_net_conn_readable() = 0;
    virtual void on_net_conn_writable() = 0;
    virtual void on_net_conn_close() = 0;
};

class linux_net_conn_t :
    public linux_epoll_callback_t,
    // linux_net_conn_t can be safely destroyed on a different core from the one it was created on,
    // unlike most data types in RethinkDB.
    public alloc_mixin_t<cross_thread_alloc_accessor_t<malloc_alloc_t>, linux_net_conn_t>
{
    
public:
    void set_callback(linux_net_conn_callback_t *cb);
    ssize_t read_nonblocking(void *buf, size_t count);
    ssize_t write_nonblocking(const void *buf, size_t count);
    ~linux_net_conn_t();
    
private:
    friend class linux_io_calls_t;
    friend class linux_net_listener_t;
    friend class linux_event_queue_t;
    
    fd_t sock;
    linux_net_conn_callback_t *callback;
    bool *set_me_true_on_delete;   // So we can tell if a callback deletes the conn_fsm_t
    
    linux_net_conn_t(fd_t);
    void on_epoll(int events);
};

struct linux_net_listener_callback_t {
    virtual void on_net_listener_accept(linux_net_conn_t *conn) = 0;
};

class linux_net_listener_t :
    public linux_epoll_callback_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, linux_net_listener_t>
{

public:
    linux_net_listener_t(int port);
    void set_callback(linux_net_listener_callback_t *cb);
    ~linux_net_listener_t();

private:
    fd_t sock;
    linux_net_listener_callback_t *callback;
    
    void on_epoll(int events);
};

class linux_direct_file_t :
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, linux_direct_file_t>
{

public:
    enum mode_t {
        mode_read = 1 << 0,
        mode_write = 1 << 1
    };
    
    linux_direct_file_t(const char *path, int mode);
    
    bool is_block_device();
    size_t get_size();
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
    uint64_t file_size;
    void verify(size_t offset, size_t length, void* buf);
};

/* TODO: Batch requests internally so that we can send multiple requests per syscall */

#endif // __IO_CALLS_HPP__

