#include <fcntl.h>
#include <algorithm>
#include <unistd.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include "arch/linux/arch.hpp"
#include "config/args.hpp"
#include "config/alloc.hpp"
#include "utils.hpp"

// #define DEBUG_DUMP_WRITES 1

/* Network connection object */

linux_net_conn_t::linux_net_conn_t(fd_t sock)
    : sock(sock), callback(NULL), set_me_true_on_delete(NULL) {
    
    assert(sock != INVALID_FD);
    
    int res = fcntl(sock, F_SETFL, O_NONBLOCK);
    check("Could not make socket non-blocking", res != 0);
}

void linux_net_conn_t::set_callback(linux_net_conn_callback_t *cb) {
    
    assert(!callback);
    assert(cb);
    callback = cb;
    
    linux_thread_pool_t::event_queue->watch_resource(sock, EPOLLET|EPOLLIN|EPOLLOUT, this);
}

ssize_t linux_net_conn_t::read_nonblocking(void *buf, size_t count) {
    
    // TODO PERFMON get_cpu_context()->worker->bytes_read += count;
    return ::read(sock, buf, count);
}

ssize_t linux_net_conn_t::write_nonblocking(const void *buf, size_t count) {

    // TODO PERFMON get_cpu_context()->worker->bytes_written += count;
    return ::write(sock, buf, count);
}

void linux_net_conn_t::on_epoll(int events) {
    
    assert(callback);
    
    // So we can tell if the callback deletes the linux_net_conn_t
    bool was_deleted = false;
    set_me_true_on_delete = &was_deleted;
    
    if (events & EPOLLIN || events & EPOLLOUT) {

        if (events & EPOLLIN) callback->on_net_conn_readable();
        if (was_deleted) return;
        
        if (events & EPOLLOUT) callback->on_net_conn_writable();
        if (was_deleted) return;
        
    } else if (events == EPOLLRDHUP || events == EPOLLERR || events == EPOLLHUP) {
              
        callback->on_net_conn_close();
        if (was_deleted) return;
        
        delete this;
        return;
        
    } else {
        fail("epoll_wait came back with an unhandled event");
    }
    
    set_me_true_on_delete = NULL;
}

linux_net_conn_t::~linux_net_conn_t() {
    
    if (set_me_true_on_delete)
        *set_me_true_on_delete = true;
    
    if (sock != INVALID_FD) {
        if (callback) linux_thread_pool_t::event_queue->forget_resource(sock, this);
        ::close(sock);
        sock = INVALID_FD;
    }
}

/* Network listener object */

linux_net_listener_t::linux_net_listener_t(int port)
    : callback(NULL)
{
    
    int res;
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    check("Couldn't create socket", sock == -1);
    
    int sockoptval = 1;
    res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockoptval, sizeof(sockoptval));
    check("Could not set REUSEADDR option", res == -1);
    
    // Bind the socket
    sockaddr_in serv_addr;
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    res = bind(sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
    check("Couldn't bind socket", res != 0);

    // Start listening to connections
    res = listen(sock, 5);
    check("Couldn't listen to the socket", res != 0);
    
    res = fcntl(sock, F_SETFL, O_NONBLOCK);
    check("Could not make socket non-blocking", res != 0);
}

void linux_net_listener_t::set_callback(linux_net_listener_callback_t *cb) {
    
    assert(!callback);
    assert(cb);
    callback = cb;
    
    linux_thread_pool_t::event_queue->watch_resource(sock, EPOLLET|EPOLLIN, this);
}

void linux_net_listener_t::on_epoll(int events) {
    
    while (true) {
        sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int new_sock = accept(sock, (sockaddr*)&client_addr, &client_addr_len);
        
        if (new_sock == INVALID_FD) {
            if (errno == EAGAIN) break;
            else fail("Cannot accept new connection: errno=%s", strerror(errno));
        
        } else {
            callback->on_net_listener_accept(new linux_net_conn_t(new_sock));
        }
    }
}

linux_net_listener_t::~linux_net_listener_t() {
    
    int res;
    
    if (callback) linux_thread_pool_t::event_queue->forget_resource(sock, this);
    
    res = shutdown(sock, SHUT_RDWR);
    check("Could not shutdown main socket", res == -1);
    
    res = close(sock);
    check("Could not close main socket", res != 0);
}

/* Disk file object */

linux_direct_file_t::linux_direct_file_t(const char *path, int mode) {
    
    int res;
    
    // Determine if it is a block device
    
    struct stat64 file_stat;
    bzero((void*)&file_stat, sizeof(file_stat)); // make valgrind happy
    res = stat64(path, &file_stat);
    check("Could not stat file", res == -1 && errno != ENOENT);
    
    if (res == -1 && errno == ENOENT) {
        is_block = false;
    } else {
        is_block = S_ISBLK(file_stat.st_mode);
    }
    
    // Construct file flags
    
    int flags = O_CREAT | O_DIRECT | O_LARGEFILE;
    
    if (mode & (mode_read | mode_write)) flags |= O_RDWR;
    else if (mode & mode_write) flags |= O_WRONLY;
    else if (mode & mode_read) flags |= O_RDONLY;
    else fail("Bad mode.");
    
    // O_NOATIME requires owner or root privileges. This is a bit of a hack; we assume that
    // if we are opening a regular file, we are the owner, but if we are opening a block device,
    // we are not.
    if (!is_block) flags |= O_NOATIME;
    
    // Open the file
    
    fd = open(path, flags, 0644);
    check("Could not open file", fd == INVALID_FD);
    
    // Determine the file size
    
    if (is_block) {
        res = ioctl(fd, BLKGETSIZE64, &file_size);
        check("Could not determine block device size", res == -1);
        
    } else {
        off64_t size = lseek64(fd, 0, SEEK_END);
        check("Could not determine file size", size == -1);
        res = lseek64(fd, 0, SEEK_SET);
        check("Could not reset file position", res == -1);
        
        file_size = size;
    }
}

bool linux_direct_file_t::is_block_device() {
    return is_block;
}

size_t linux_direct_file_t::get_size() {
    return file_size;
}

void linux_direct_file_t::set_size(size_t size) {
    assert(!is_block);
    int res = ftruncate(fd, size);
    check("Could not ftruncate()", res == -1);
    file_size = size;
}

void linux_direct_file_t::set_size_at_least(size_t size) {
    
    if (is_block) {
        assert(file_size >= size);
    
    } else {
        /* Grow in large chunks at a time */
        if (file_size < size) {
            set_size(ceil_aligned(size, DEVICE_BLOCK_SIZE * 128));
        }
    }
}

bool linux_direct_file_t::read_async(size_t offset, size_t length, void *buf, linux_iocallback_t *callback) {
    
    verify(offset, length, buf);
    
    linux_event_queue_t *queue = linux_thread_pool_t::event_queue;
    linux_io_calls_t *iosys = &queue->iosys;
    
    // Prepare the request
    iocb *request = (iocb*)tls_small_obj_alloc_accessor<alloc_t>::get_alloc<iocb>()->malloc(iosys->iocb_size);
    io_prep_pread(request, fd, buf, length, offset);
    io_set_eventfd(request, queue->aio_notify_fd);
    request->data = callback;

    // Add it to a list of outstanding read requests
    iosys->r_requests.push_back(request);

    // Process whatever is left
    iosys->process_requests();
    
    return false;
}

bool linux_direct_file_t::write_async(size_t offset, size_t length, void *buf, linux_iocallback_t *callback) {
    
#ifdef DEBUG_DUMP_WRITES
    printf("--- WRITE BEGIN ---\n");
    print_backtrace(stdout);
    printf("\n");
    print_hd(buf, offset, length);
    printf("---- WRITE END ----\n\n");
#endif
    
    verify(offset, length, buf);
    
    linux_event_queue_t *queue = linux_thread_pool_t::event_queue;
    linux_io_calls_t *iosys = &queue->iosys;
    
    // Prepare the request
    iocb *request = (iocb*)tls_small_obj_alloc_accessor<alloc_t>::get_alloc<iocb>()->malloc(iosys->iocb_size);
    io_prep_pwrite(request, fd, buf, length, offset);
    io_set_eventfd(request, queue->aio_notify_fd);
    request->data = callback;

    // Add it to a list of outstanding write requests
    iosys->w_requests.push_back(request);
    
    // Process whatever is left
    iosys->process_requests();
    
    return false;
}

void linux_direct_file_t::read_blocking(size_t offset, size_t length, void *buf) {
    
    verify(offset, length, buf);
    size_t res = pread(fd, buf, length, offset);
    check("Blocking read failed", res != length);
}

void linux_direct_file_t::write_blocking(size_t offset, size_t length, void *buf) {
    
    verify(offset, length, buf);
    size_t res = pwrite(fd, buf, length, offset);
    check("Blocking write failed", res != length);
}

linux_direct_file_t::~linux_direct_file_t() {
    
    close(fd);
}

void linux_direct_file_t::verify(size_t offset, size_t length, void *buf) {
    
    assert(buf);
    assert(offset + length <= file_size);
    assert((intptr_t)buf % DEVICE_BLOCK_SIZE == 0);
    assert(offset % DEVICE_BLOCK_SIZE == 0);
    assert(length % DEVICE_BLOCK_SIZE == 0);
}

/* Async IO scheduler */

/* TODO: Batch requests internally so that we can send multiple requests per
call to io_submit(). */

linux_io_calls_t::linux_io_calls_t(linux_event_queue_t *_queue)
    : queue(_queue),
      n_pending(0)
{
    r_requests.reserve(MAX_CONCURRENT_IO_REQUESTS);
    w_requests.reserve(MAX_CONCURRENT_IO_REQUESTS);
}

linux_io_calls_t::~linux_io_calls_t()
{
    assert(r_requests.empty());
    assert(w_requests.empty());
    assert(n_pending == 0);
}

void linux_io_calls_t::aio_notify(iocb *event, int result) {
    // Schedule the requests we couldn't finish last time
    n_pending--;
    process_requests();
    
    // Check for failure (because the higher-level code usually doesn't)
    if (result != (int)event->u.c.nbytes) {
        errno = -result;
        check("Read or write failed", 1);
    }
    
    // Notify the interested party about the event
    linux_iocallback_t *callback = (linux_iocallback_t*)event->data;
    
    // Prepare event_t for the callback
    event_t qevent;
    bzero((void*)&qevent, sizeof(qevent));
    qevent.state = NULL;
    qevent.result = result;
    qevent.buf = event->u.c.buf;
    qevent.offset = event->u.c.offset;
    qevent.op = event->aio_lio_opcode == IO_CMD_PREAD ? eo_read : eo_write;

    callback->on_io_complete(&qevent);

    // Free the iocb structure
    tls_small_obj_alloc_accessor<alloc_t>::get_alloc<iocb>()->free(event);
}

void linux_io_calls_t::process_requests() {
    if(n_pending > TARGET_IO_QUEUE_DEPTH)
        return;
    
    int res = 0;
    while(!r_requests.empty() || !w_requests.empty()) {
        res = process_request_batch(&r_requests);
        if(res < 0)
            break;
        
        res = process_request_batch(&w_requests);
        if(res < 0)
            break;
    }
    check("Could not submit IO request", res < 0 && res != -EAGAIN);
}

int linux_io_calls_t::process_request_batch(request_vector_t *requests) {
    // Submit a batch
    int res = 0;
    if(requests->size() > 0) {
        res = io_submit(queue->aio_context,
                        std::min(requests->size(), size_t(TARGET_IO_QUEUE_DEPTH / 2)),
                        &requests->operator[](0));
        if(res > 0) {
            // TODO: erase will cause the vector to shift elements in
            // the back. Perhaps we should optimize this somehow.
            requests->erase(requests->begin(), requests->begin() + res);
            n_pending += res;
        }
    }
    return res;
}

