#include <arpa/inet.h>
#include <algorithm>
#include <fcntl.h>
#include <linux/fs.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include "arch/linux/arch.hpp"
#include "config/args.hpp"
#include "utils2.hpp"

// #define DEBUG_DUMP_WRITES 1

/* Network connection object */

linux_net_conn_t::linux_net_conn_t(fd_t sock)
    : sock(sock), callback(NULL), set_me_true_on_delete(NULL),
      registered_for_write_notifications(false)
{
    
    assert(sock != INVALID_FD);
    
    int res = fcntl(sock, F_SETFL, O_NONBLOCK);
    guarantee_err(res == 0, "Could not make socket non-blocking");
}

void linux_net_conn_t::set_callback(linux_net_conn_callback_t *cb) {
    
    assert(!callback);
    assert(cb);
    callback = cb;
    
    linux_thread_pool_t::thread->queue.watch_resource(sock, poll_event_in, this);
}

ssize_t linux_net_conn_t::read_nonblocking(void *buf, size_t count) {
    return ::read(sock, buf, count);
}

ssize_t linux_net_conn_t::write_nonblocking(const void *buf, size_t count) {

    int res = ::write(sock, buf, count);
    if(res == EAGAIN || res == EWOULDBLOCK) {
        // Whoops, got stuff to write, turn on write notification.
        linux_thread_pool_t::thread->queue.adjust_resource(sock, poll_event_in | poll_event_out, this);
        registered_for_write_notifications = true;
    } else if(registered_for_write_notifications) {
        // We can turn off write notifications now as we no longer
        // have stuff to write and are waiting on the socket.
        linux_thread_pool_t::thread->queue.adjust_resource(sock, poll_event_in, this);
        registered_for_write_notifications = false;
    }

    return res;
}

void linux_net_conn_t::on_event(int events) {
    
    assert(callback);
    
    // So we can tell if the callback deletes the linux_net_conn_t
    bool was_deleted = false;
    set_me_true_on_delete = &was_deleted;
    
    if (events & poll_event_in || events & poll_event_out) {

        if (events & poll_event_in) {
            callback->on_net_conn_readable();
        }
        if (was_deleted) return;
        
        if (events & poll_event_out) {
            callback->on_net_conn_writable();
        }
        if (was_deleted) return;
        
    } else if (events & poll_event_err) {
              
        callback->on_net_conn_close();
        if (was_deleted) return;
        
        delete this;
        return;
        
    } else {
        // TODO: this actually happened at some point. Handle all of
        // these things properly.
        fail("epoll_wait came back with an unhandled event");
    }
    
    set_me_true_on_delete = NULL;
}

linux_net_conn_t::~linux_net_conn_t() {
    
    if (set_me_true_on_delete)
        *set_me_true_on_delete = true;
    
    if (sock != INVALID_FD) {
        if (callback) linux_thread_pool_t::thread->queue.forget_resource(sock, this);
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
    guarantee_err(sock != -1, "Couldn't create socket");
    
    int sockoptval = 1;
    res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockoptval, sizeof(sockoptval));
    guarantee_err(res == 0, "Could not set REUSEADDR option");
    
    // Bind the socket
    sockaddr_in serv_addr;
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    res = bind(sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
    guarantee_err(res == 0, "Couldn't bind socket");

    // Start listening to connections
    res = listen(sock, 5);
    guarantee_err(res == 0, "Couldn't listen to the socket");
    
    res = fcntl(sock, F_SETFL, O_NONBLOCK);
    guarantee_err(res == 0, "Could not make socket non-blocking");
}

void linux_net_listener_t::set_callback(linux_net_listener_callback_t *cb) {
    
    assert(!callback);
    assert(cb);
    callback = cb;
    
    linux_thread_pool_t::thread->queue.watch_resource(sock, poll_event_in, this);
}

void linux_net_listener_t::on_event(int events) {
    
    while (true) {
        sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int new_sock = accept(sock, (sockaddr*)&client_addr, &client_addr_len);
        
        if (new_sock == INVALID_FD) {
            if (errno == EAGAIN) break;
            else fail("Cannot accept new connection: errno=%s", strerror(errno));   // RSI
        
        } else {
            callback->on_net_listener_accept(new linux_net_conn_t(new_sock));
        }
    }
}

linux_net_listener_t::~linux_net_listener_t() {
    
    int res;
    
    if (callback) linux_thread_pool_t::thread->queue.forget_resource(sock, this);
    
    res = shutdown(sock, SHUT_RDWR);
    guarantee_err(res != -1, "Could not shutdown main socket");
    
    res = close(sock);
    guarantee_err(res == 0, "Could not close main socket");
}

/* Disk file object */

perfmon_counter_t
    pm_io_reads_started("io_reads_started[ioreads]"),
    pm_io_reads_completed("io_reads_completed[ioreads]"),
    pm_io_writes_started("io_writes_started[iowrites]"),
    pm_io_writes_completed("io_writes_completed[iowrites]");

linux_direct_file_t::linux_direct_file_t(const char *path, int mode) {
    
    int res;
    
    // Determine if it is a block device
    
    struct stat64 file_stat;
    bzero((void*)&file_stat, sizeof(file_stat)); // make valgrind happy
    res = stat64(path, &file_stat);
    guarantee_err(res != -1 || errno == ENOENT, "Could not stat file");
    
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
    else guaranteef(false, "Bad file access mode.");
    
    // O_NOATIME requires owner or root privileges. This is a bit of a hack; we assume that
    // if we are opening a regular file, we are the owner, but if we are opening a block device,
    // we are not.
    if (!is_block) flags |= O_NOATIME;
    
    // Open the file
    
    fd = open(path, flags, 0644);
    guarantee_err(fd != INVALID_FD, "Could not open file");
    
    // Determine the file size
    
    if (is_block) {
        res = ioctl(fd, BLKGETSIZE64, &file_size);
        guarantee_err(res != -1, "Could not determine block device size");
        
    } else {
        off64_t size = lseek64(fd, 0, SEEK_END);
        guarantee_err(size != -1, "Could not determine file size");
        res = lseek64(fd, 0, SEEK_SET);
        guarantee_err(res != -1, "Could not reset file position");
        
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
    guarantee_err(res == 0, "Could not ftruncate()");
    file_size = size;
}

void linux_direct_file_t::set_size_at_least(size_t size) {
    
    if (is_block) {
        assert(file_size >= size);
    
    } else {
        /* Grow in large chunks at a time */
        if (file_size < size) {
            // TODO: we should make the growth rate of a db file
            // configurable.
            set_size(ceil_aligned(size, DEVICE_BLOCK_SIZE * 128));
        }
    }
}

bool linux_direct_file_t::read_async(size_t offset, size_t length, void *buf, linux_iocallback_t *callback) {
    
    verify(offset, length, buf);
    
    linux_io_calls_t *iosys = &linux_thread_pool_t::thread->iosys;
    
    // Prepare the request
    iocb *request = new iocb;
    io_prep_pread(request, fd, buf, length, offset);
    io_set_eventfd(request, iosys->aio_notify_fd);
    request->data = callback;

    // Add it to a list of outstanding read requests
    iosys->r_requests.queue.push_back(request);
    
    pm_io_reads_started++;

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
    
    linux_io_calls_t *iosys = &linux_thread_pool_t::thread->iosys;
    
    // Prepare the request
    iocb *request = new iocb;
    io_prep_pwrite(request, fd, buf, length, offset);
    io_set_eventfd(request, iosys->aio_notify_fd);
    request->data = callback;

    // Add it to a list of outstanding write requests
    iosys->w_requests.queue.push_back(request);
    
    pm_io_writes_started++;
    
    // Process whatever is left
    iosys->process_requests();
    
    return false;
}

void linux_direct_file_t::read_blocking(size_t offset, size_t length, void *buf) {
    
    verify(offset, length, buf);
    size_t res = pread(fd, buf, length, offset);
    guarantee_err(res == length, "Blocking read failed");   // RSI: assert?
}

void linux_direct_file_t::write_blocking(size_t offset, size_t length, void *buf) {
    
    verify(offset, length, buf);
    size_t res = pwrite(fd, buf, length, offset);
    guarantee_err(res == length, "Blocking write failed");   // RSI: assert?
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

linux_io_calls_t::linux_io_calls_t(linux_event_queue_t *queue)
    : queue(queue), n_pending(0), r_requests(this), w_requests(this)
{
    int res;
    
    // Create aio context
    
    aio_context = 0;
    res = io_setup(MAX_CONCURRENT_IO_REQUESTS, &aio_context);
    guaranteef(res == 0, "Could not setup aio context");    // errors are returned in res (negated) instead of errno
    
    // Create aio notify fd
    
    aio_notify_fd = eventfd(0, 0);
    guarantee_err(aio_notify_fd != -1, "Could not create aio notification fd");

    res = fcntl(aio_notify_fd, F_SETFL, O_NONBLOCK);
    guarantee_err(res == 0, "Could not make aio notify fd non-blocking");

    queue->watch_resource(aio_notify_fd, poll_event_in, this);
}

linux_io_calls_t::~linux_io_calls_t()
{
    int res;
    
    assert(n_pending == 0);
    
    res = close(aio_notify_fd);
    guarantee_err(res == 0, "Could not close aio_notify_fd");
    
    res = io_destroy(aio_context);
    guarantee_err(res == 0, "Could not destroy aio_context");
}

void linux_io_calls_t::on_event(int) {
    
    int res, nevents;
    eventfd_t nevents_total;
    
    res = eventfd_read(aio_notify_fd, &nevents_total);
    guarantee_err(res == 0, "Could not read aio_notify_fd value");

    // Note: O(1) array allocators are hard. To avoid all the
    // complexity, we'll use a fixed sized array and call io_getevents
    // multiple times if we have to (which should be very unlikely,
    // anyway).
    io_event events[MAX_IO_EVENT_PROCESSING_BATCH_SIZE];
    
    do {
        // Grab the events. Note: we need to make sure we don't read
        // more than nevents_total, otherwise we risk reading an io
        // event and getting an eventfd for this read event later due
        // to the way the kernel is structured. Better avoid this
        // complexity (hence std::min below).
        nevents = io_getevents(aio_context, 0,
                               std::min((int)nevents_total, MAX_IO_EVENT_PROCESSING_BATCH_SIZE),
                               events, NULL);
        guarantee_err(nevents >= 1, "Waiting for AIO event failed");
        
        // Process the events
        for(int i = 0; i < nevents; i++) {
            aio_notify((iocb*)events[i].obj, events[i].res);
        }
        nevents_total -= nevents;
        
    } while (nevents_total > 0);
}

void linux_io_calls_t::aio_notify(iocb *event, int result) {
    
    // Update stats
    if (event->aio_lio_opcode == IO_CMD_PREAD) pm_io_reads_completed++;
    else pm_io_writes_completed++;
    
    // Schedule the requests we couldn't finish last time
    n_pending--;
    process_requests();
    
    // Check for failure (because the higher-level code usually doesn't)
    if (result != (int)event->u.c.nbytes) {
        errno = -result;
        check("Read or write failed", 1);   // RSI
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
    delete event;
}

void linux_io_calls_t::process_requests() {
    if (n_pending > TARGET_IO_QUEUE_DEPTH)
        return;
    
    int res = 0;
    while(!r_requests.queue.empty() || !w_requests.queue.empty()) {
        res = r_requests.process_request_batch();
        if(res < 0)
            break;
        
        res = w_requests.process_request_batch();
        if(res < 0)
            break;
    }
    guarantee_err(res >= 0 || res == -EAGAIN, "Could not submit IO request");
}

linux_io_calls_t::queue_t::queue_t(linux_io_calls_t *parent)
    : parent(parent)
{
    queue.reserve(MAX_CONCURRENT_IO_REQUESTS);
}

int linux_io_calls_t::queue_t::process_request_batch() {
    // Submit a batch
    int res = 0;
    if(queue.size() > 0) {
        res = io_submit(parent->aio_context,
                        std::min(queue.size(), size_t(TARGET_IO_QUEUE_DEPTH / 2)),
                        &queue[0]);
        if (res > 0) {
            // TODO: erase will cause the vector to shift elements in
            // the back. Perhaps we should optimize this somehow.
            queue.erase(queue.begin(), queue.begin() + res);
            parent->n_pending += res;
        }
    }
    return res;
}

linux_io_calls_t::queue_t::~queue_t() {
    assert(queue.size() == 0);
}
