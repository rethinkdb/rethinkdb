
#ifndef __IO_CALLS_HPP__
#define __IO_CALLS_HPP__

#include <libaio.h>
#include <vector>
#include "utils.hpp"
#include "config/args.hpp"
#include "config/alloc.hpp"
#include "arch/resource.hpp"
#include "event.hpp"
#include "corefwd.hpp"

typedef int fd_t;
#define INVALID_FD fd_t(-1)

typedef fd_t resource_t;

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
    void process_network_notify(int events);
};

class linux_net_listener_t {

public:
    linux_net_listener_t(int port);
    linux_net_conn_t *accept_blocking();
    ~linux_net_listener_t();

private:
    fd_t sock;
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

/* TODO: Merge linux_io_calls_t with linux_event_queue_t */

class linux_io_calls_t {
    
    friend class linux_event_queue_t;
    friend class linux_direct_file_t;
    
    linux_io_calls_t(linux_event_queue_t *_queue);
    virtual ~linux_io_calls_t();

    typedef std::vector<iocb*, gnew_alloc<iocb*> > request_vector_t;

    void process_requests();
    int process_request_batch(request_vector_t *requests);

    linux_event_queue_t *queue;

    request_vector_t r_requests, w_requests;

    int n_pending;

#ifndef NDEBUG
    // We need the extra pointer in debug mode because
    // tls_small_obj_alloc_accessor creates the pools to expect it
    static const size_t iocb_size = sizeof(iocb) + sizeof(void*);
#else
    static const size_t iocb_size = sizeof(iocb);
#endif

public:
    /**
     * AIO notification support (this is meant to be called by the
     * event queue).
     */
    void aio_notify(iocb *event, int result);
};

#endif // __IO_CALLS_HPP__

