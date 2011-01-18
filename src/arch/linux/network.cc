#include "arch/linux/network.hpp"
#include "arch/linux/thread_pool.hpp"
#include "concurrency/cond_var.hpp"   // For value_cond_t
#include "logger.hpp"
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

/* Network connection object */

linux_net_conn_t::linux_net_conn_t(const char *host, int port) {
    not_implemented();
}

linux_net_conn_t::linux_net_conn_t(fd_t sock)
    : sock(sock),
      registration_thread(-1),
      read_cond(NULL), write_cond(NULL),
      read_was_shut_down(false), write_was_shut_down(false)
{
    assert(sock != INVALID_FD);

    int res = fcntl(sock, F_SETFL, O_NONBLOCK);
    guarantee_err(res == 0, "Could not make socket non-blocking");
}

void linux_net_conn_t::register_with_event_loop() {
    /* Register ourself to receive notifications from the event loop if we have not
    already done so. */

    if (registration_thread == -1) {
        registration_thread = linux_thread_pool_t::thread_id;
        linux_thread_pool_t::thread->queue.watch_resource(sock, poll_event_in|poll_event_out, this);

    } else if (registration_thread != linux_thread_pool_t::thread_id) {
        guarantee(registration_thread == linux_thread_pool_t::thread_id,
            "Must always use a net_conn_t on the same thread.");
    }
}

size_t linux_net_conn_t::read_internal(void *buffer, size_t size) {

    while (true) {
        int res = ::read(sock, buffer, size);
        if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            /* There's no data available right now, so we must wait for a notification from the
            epoll queue */
            value_cond_t<bool> cond;
            read_cond = &cond;
            bool ok = cond.wait();
            read_cond = NULL;
            if (ok) {
                /* We got a notification from the epoll queue; go around the loop again and try to
                read again */
            } else {
                /* We were closed for whatever reason. Whatever signalled us has already called
                on_shutdown_read(). */
                throw read_closed_exc_t();
            }
        } else if (res == 0 || (res == -1 && (errno == ECONNRESET || errno == ENOTCONN))) {
            /* We were closed. This is the first notification that the kernel has given us, so we
            must call on_shutdown_read(). */
            on_shutdown_read();
            throw read_closed_exc_t();
        } else if (res == -1) {
            /* Unknown error. This is not expected, but it will probably happen sometime so we
            shouldn't crash. */
            logERR("Could not read from socket: %s", strerror(errno));
            on_shutdown_read();
            throw read_closed_exc_t();
        } else {
            /* We read some data, whooo */
            return res;
        }
    }
}

void linux_net_conn_t::read(void *buf, size_t size) {

    register_with_event_loop();
    assert(!read_cond);   // Is there a read already in progress?
    if (read_was_shut_down) throw read_closed_exc_t();

    /* First, consume any data in the peek buffer */
    int read_buffer_bytes = std::min(read_buffer.size(), size);
    memcpy(buf, read_buffer.data(), read_buffer_bytes);
    read_buffer.erase(read_buffer.begin(), read_buffer.begin() + read_buffer_bytes);
    buf = reinterpret_cast<void *>(reinterpret_cast<char *>(buf) + read_buffer_bytes);
    size -= read_buffer_bytes;

    /* Now go to the kernel for any more data that we need */
    while (size > 0) {
        size_t delta = read_internal(buf, size);
        assert(delta <= size);
        buf = reinterpret_cast<void *>(reinterpret_cast<char *>(buf) + delta);
        size -= delta;
    }
}

void linux_net_conn_t::peek_until(peek_callback_t *cb) {

    register_with_event_loop();
    assert(!read_cond);
    if (read_was_shut_down) throw read_closed_exc_t();

    while (!cb->check(read_buffer.data(), read_buffer.size())) {

        // cb->check() might have closed the connection
        if (read_was_shut_down) throw read_closed_exc_t();

        // Grow the peek buffer so we have some space to put what we're about to insert
        int old_size = read_buffer.size();
        read_buffer.resize(old_size + IO_BUFFER_SIZE);

        // Insert more data into the peek buffer. This may throw read_closed_exc_t, but we're OK
        // with that.
        size_t delta = read_internal(read_buffer.data() + old_size, IO_BUFFER_SIZE);

        // Shrink again to the size of the actual amount of data we read
        read_buffer.resize(old_size + delta);
    }
}

void linux_net_conn_t::shutdown_read() {

    int res = ::shutdown(sock, SHUT_RD);
    if (res != 0 && errno != ENOTCONN) {
        logERR("Could not shutdown socket for reading: %s", strerror(errno));
    }

    on_shutdown_read();
}

void linux_net_conn_t::on_shutdown_read() {

    assert(!read_was_shut_down);
    read_was_shut_down = true;

    // Deregister ourself with the event loop. If the write half of the connection
    // is still open, the make sure we stay registered for write.
    if (registration_thread != -1) {
        assert(registration_thread == linux_thread_pool_t::thread_id);
        if (write_was_shut_down) {
            linux_thread_pool_t::thread->queue.forget_resource(sock, this);
        } else {
            linux_thread_pool_t::thread->queue.adjust_resource(sock, poll_event_out, this);
        }
    }

    // Inform any readers that were waiting that the socket has been closed.
    if (read_cond) {
        read_cond->pulse(false);
        read_cond = NULL;
    }
}

bool linux_net_conn_t::is_read_open() {
    return !read_was_shut_down;
}

void linux_net_conn_t::write_internal(const void *buf, size_t size) {

    while (size > 0) {
        int res = ::write(sock, buf, size);
        if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            /* There's no space for our data right now, so we must wait for a notification from the
            epoll queue */
            value_cond_t<bool> cond;
            write_cond = &cond;
            bool ok = cond.wait();
            write_cond = NULL;
            if (ok) {
                /* We got a notification from the epoll queue; go around the loop again and try to
                write again */
            } else {
                /* We were closed for whatever reason. Whatever signalled us has already called
                on_shutdown_write(). */
                throw write_closed_exc_t();
            }
        } else if (res == -1 && (errno == EPIPE || errno == ENOTCONN || errno == EHOSTUNREACH ||
                errno == ENETDOWN || errno == EHOSTDOWN || errno == ECONNRESET)) {
            /* These errors are expected to happen at some point in practice */
            on_shutdown_write();
            throw write_closed_exc_t();
        } else if (res == -1) {
            /* In theory this should never happen, but it probably will. So we write a log message
            and then shut down normally. */
            logERR("Could not write to socket: %s", strerror(errno));
            on_shutdown_write();
            throw write_closed_exc_t();
        } else if (res == 0) {
            /* This should never happen either, but it's better to write an error message than to
            crash completely. */
            logERR("Didn't expect write() to return 0.");
            on_shutdown_write();
            throw write_closed_exc_t();
        } else {
            assert(res <= (int)size);
            buf = reinterpret_cast<const void *>(reinterpret_cast<const char *>(buf) + res);
            size -= res;
        }
    }
}

void linux_net_conn_t::write(const void *buf, size_t size) {

    register_with_event_loop();
    assert(!write_cond);
    if (write_was_shut_down) throw write_closed_exc_t();

    /* To preserve ordering, all buffered data must be sent before this chunk of data is sent */
    flush_buffer();

    /* Now send this chunk of data */
    write_internal(buf, size);
}

void linux_net_conn_t::write_buffered(const void *buf, size_t size) {

    register_with_event_loop();
    assert(!write_cond);
    if (write_was_shut_down) throw write_closed_exc_t();

    /* Append data to write_buffer */
    int old_size = write_buffer.size();
    write_buffer.resize(old_size + size);
    memcpy(write_buffer.data() + old_size, buf, size);
}

void linux_net_conn_t::flush_buffer() {

    register_with_event_loop();
    assert(!write_cond);
    if (write_was_shut_down) throw write_closed_exc_t();

    write_internal(write_buffer.data(), write_buffer.size());
    write_buffer.clear();
}

void linux_net_conn_t::shutdown_write() {

    int res = ::shutdown(sock, SHUT_WR);
    if (res != 0 && errno != ENOTCONN) {
        logERR("Could not shutdown socket for writing: %s", strerror(errno));
    }

    on_shutdown_write();
}

void linux_net_conn_t::on_shutdown_write() {

    assert(!write_was_shut_down);
    write_was_shut_down = true;

    // Deregister ourself with the event loop. If the read half of the connection
    // is still open, the make sure we stay registered for read.
    if (registration_thread != -1) {
        assert(registration_thread == linux_thread_pool_t::thread_id);
        if (read_was_shut_down) {
            linux_thread_pool_t::thread->queue.forget_resource(sock, this);
        } else {
            linux_thread_pool_t::thread->queue.adjust_resource(sock, poll_event_in, this);
        }
    }

    // Inform any writers that were waiting that the socket has been closed.
    if (write_cond) {
        write_cond->pulse(false);
        write_cond = NULL;
    }
}

bool linux_net_conn_t::is_write_open() {
    return !write_was_shut_down;
}

linux_net_conn_t::~linux_net_conn_t() {

    assert(read_was_shut_down);
    assert(write_was_shut_down);

    int res = ::close(sock);
    if (res != 0) {
        logERR("close() failed: %s", strerror(errno));
    }
}

void linux_net_conn_t::on_event(int events) {

    if ((events & poll_event_err) && (events & poll_event_hup)) {
        /* We get this when the socket is closed but there is still data we are trying to send.
        For example, it can sometimes be reproduced by sending "nonsense\r\n" and then sending
        "set [key] 0 0 [length] noreply\r\n[value]\r\n" a hundred times then immediately closing the
        socket.
        
        I speculate that the "error" part comes from the fact that there is undelivered data
        in the socket send buffer, and the "hup" part comes from the fact that the remote end
        has hung up.
        
        Ignore it; the other logic will handle it properly. */
        
    } else if (events & poll_event_err) {
        /* We don't know why we got this, so shut the hell down. */
        logERR("Unexpected poll_event_err. Events: %d\n", events);
        if (!read_was_shut_down) shutdown_read();
        if (!write_was_shut_down) shutdown_write();
        return;
    }

    if (events & poll_event_in) {
        assert(!read_was_shut_down);
        if (read_cond) {
            read_cond->pulse(true);
            read_cond = NULL;
        }
    }

    if (events & poll_event_out) {
        assert(!write_was_shut_down);
        if (write_cond) {
            write_cond->pulse(true);
            write_cond = NULL;
        }
    }
}

/* Network listener object */

linux_net_listener_t::linux_net_listener_t(int port)
    : callback(NULL)
{
    defunct = false;

    int res;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    guarantee_err(sock != INVALID_FD, "Couldn't create socket");

    int sockoptval = 1;
    res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockoptval, sizeof(sockoptval));
    guarantee_err(res != -1, "Could not set REUSEADDR option");

    /* XXX Making our socket NODELAY prevents the problem where responses to
     * pipelined requests are delayed, since the TCP Nagle algorithm will
     * notice when we send mulitple small packets and try to coalesce them. But
     * if we are only sending a few of these small packets quickly, like during
     * pipeline request responses, then Nagle delays for around 40 ms before
     * sending out those coalesced packets if they don't reach the max window
     * size. So for latency's sake we want to disable Nagle.
     *
     * This might decrease our throughput, so perhaps we should add a
     * runtime option for it.
     *
     * - Jordan 12/22/10
     */
    res = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &sockoptval, sizeof(sockoptval));
    guarantee_err(res != -1, "Could not set TCP_NODELAY option");

    // Bind the socket
    sockaddr_in serv_addr;
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    res = bind(sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
    if (res != 0) {
        fprintf(stderr, "Couldn't bind socket: %s\n", strerror(errno));
        // We cannot simply terminate here, since this may lead to corrupted database files.
        // defunct myself and rely on server to handle this condition and shutdown gracefully...
        defunct = true;
        return;
    }

    // Start listening to connections
    res = listen(sock, 5);
    guarantee_err(res == 0, "Couldn't listen to the socket");

    res = fcntl(sock, F_SETFL, O_NONBLOCK);
    guarantee_err(res == 0, "Could not make socket non-blocking");
}

void linux_net_listener_t::set_callback(linux_net_listener_callback_t *cb) {
    if (defunct)
        return;

    assert(!callback);
    assert(cb);
    callback = cb;

    linux_thread_pool_t::thread->queue.watch_resource(sock, poll_event_in, this);
}

void linux_net_listener_t::on_event(int events) {
    if (defunct)
        return;

    if (events != poll_event_in) {
        logERR("Unexpected event mask: %d\n", events);
    }

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int new_sock = accept(sock, (sockaddr*)&client_addr, &client_addr_len);

        if (new_sock == INVALID_FD) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            else {
                switch (errno) {
                    case EPROTO:
                    case ENOPROTOOPT:
                    case ENETDOWN:
                    case ENONET:
                    case ENETUNREACH:
                    case EINTR:
                        break;
                    default:
                        // We can't do anything about failing accept, but we still
                        // must continue processing current connections' request.
                        // Thus, we can't bring down the server, and must ignore
                        // the error.
                        logERR("Cannot accept new connection: %s\n", strerror(errno));
                        break;
                }
            }
        } else {
            callback->on_net_listener_accept(new linux_net_conn_t(new_sock));
        }
    }
}

linux_net_listener_t::~linux_net_listener_t() {
    if (defunct)
        return;

    int res;

    if (callback) linux_thread_pool_t::thread->queue.forget_resource(sock, this);

    res = shutdown(sock, SHUT_RDWR);
    guarantee_err(res == 0, "Could not shutdown main socket");

    res = close(sock);
    guarantee_err(res == 0, "Could not close main socket");
}

