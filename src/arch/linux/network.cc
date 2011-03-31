#include "arch/linux/network.hpp"
#include "arch/linux/thread_pool.hpp"
#include "concurrency/cond_var.hpp"   // For promise_t
#include "logger.hpp"
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/* Network connection object */

static fd_t connect_to(const char *host, int port) {

    struct addrinfo *res;

    /* make a sacrifice to the elders honor by converting port to a string, why
     * can't we just sacrifice a virgin for them (lord knows we have enough
     * virgins in Silicon Valley) */
    char port_str[10]; /* god is it dumb that we have to do this */
    snprintf(port_str, 10, "%d", port);
    //fail_due_to_user_error("Port is too big", (snprintf(port_str, 10, "%d", port) == 10));

    /* make the connection */
    if (getaddrinfo(host, port_str, NULL, &res) != 0) {
        logERR("Failed to look up address %s:%d.\n", host, port);
        goto ERROR_BREAKOUT;
    }

    {
        scoped_fd_t sock(socket(res->ai_family, res->ai_socktype, res->ai_protocol));
        if (sock.get() == INVALID_FD) {
            logERR("Failed to create a socket\n");
            goto ERROR_BREAKOUT;
        }
        if (connect(sock.get(), res->ai_addr, res->ai_addrlen) != 0) {
            /* for some reason the connection failed */
            logERR("Failed to make a connection with error: %s\n", strerror(errno));
            goto ERROR_BREAKOUT;
        }

        guarantee_err(fcntl(sock.get(), F_SETFL, O_NONBLOCK) == 0, "Could not make socket non-blocking");

        freeaddrinfo(res);
        return sock.release();
    }

ERROR_BREAKOUT:
    freeaddrinfo(res);
    throw linux_tcp_conn_t::connect_failed_exc_t();
}

linux_tcp_conn_t::linux_tcp_conn_t(const char *host, int port)
    : sock(connect_to(host, port)), event_watcher(sock.get(), this),
      read_cond(NULL), write_cond(NULL),
      read_was_shut_down(false), write_was_shut_down(false)
    { }

static fd_t connect_to(const ip_address_t &host, int port) {

    scoped_fd_t sock;
    sock.reset(socket(AF_INET, SOCK_STREAM, 0));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = host.addr;
    bzero(addr.sin_zero, sizeof(addr.sin_zero));

    if (connect(sock.get(), reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) != 0) {
        /* for some reason the connection failed */
        logINF("Failed to make a connection with error: %s\n", strerror(errno));
        throw linux_tcp_conn_t::connect_failed_exc_t();
    }

    guarantee_err(fcntl(sock.get(), F_SETFL, O_NONBLOCK) == 0, "Could not make socket non-blocking");

    return sock.release();
}

linux_tcp_conn_t::linux_tcp_conn_t(const ip_address_t &host, int port)
    : sock(connect_to(host, port)), event_watcher(sock.get(), this),
      read_cond(NULL), write_cond(NULL),
      read_was_shut_down(false), write_was_shut_down(false)
    { }

linux_tcp_conn_t::linux_tcp_conn_t(fd_t s) :
    sock(s), event_watcher(sock.get(), this),
    read_cond(NULL), write_cond(NULL),
    read_was_shut_down(false), write_was_shut_down(false)
{
    rassert(sock.get() != INVALID_FD);

    int res = fcntl(sock.get(), F_SETFL, O_NONBLOCK);
    guarantee_err(res == 0, "Could not make socket non-blocking");
}

size_t linux_tcp_conn_t::read_internal(void *buffer, size_t size) {
    rassert(!read_was_shut_down);
    rassert(!read_cond);   // Is there a read already in progress?

    while (true) {
        ssize_t res = ::read(sock.get(), buffer, size);

        if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {

            /* There's no data available right now, so we must wait for a notification from the
            epoll queue */
            cond_t cond;
            read_cond = &cond;
            event_watcher.adjust(poll_event_in, poll_event_in);
            cond.wait();

            /* The thing that signalled us set read_cond to NULL and also deregistered
            us for read events */

            if (read_was_shut_down) {
                /* We were closed for whatever reason. Something else has already called
                on_shutdown_read(). In fact, we were probably signalled by on_shutdown_read(). */
                throw read_closed_exc_t();
            }

            /* Go around the loop and try to read again */

        } else if (res == 0 || (res == -1 && (errno == ECONNRESET || errno == ENOTCONN))) {
            /* We were closed. This is the first notification that the kernel has given us, so we
            must call on_shutdown_read(). */
            on_shutdown_read();
            throw read_closed_exc_t();

        } else if (res == -1) {
            /* Unknown error. This is not expected, but it will probably happen sometime so we
            shouldn't crash. */
            logERR("Could not read from socket: %s\n", strerror(errno));
            on_shutdown_read();
            throw read_closed_exc_t();

        } else {
            /* We read some data, whooo */
            return res;
        }
    }
}

size_t linux_tcp_conn_t::read_some(void *buf, size_t size) {

    rassert(size > 0);
    rassert(!read_cond);
    if (read_was_shut_down) throw read_closed_exc_t();

    if (read_buffer.size()) {
        /* Return the data from the peek buffer */
        size_t read_buffer_bytes = std::min(read_buffer.size(), size);
        memcpy(buf, read_buffer.data(), read_buffer_bytes);
        read_buffer.erase(read_buffer.begin(), read_buffer.begin() + read_buffer_bytes);
        return read_buffer_bytes;
    } else {
        /* Go to the kernel _once_. */
        return read_internal(buf, size);
    }
}

void linux_tcp_conn_t::read(void *buf, size_t size) {

    rassert(!read_cond);   // Is there a read already in progress?
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
        rassert(delta <= size);
        buf = reinterpret_cast<void *>(reinterpret_cast<char *>(buf) + delta);
        size -= delta;
    }
}

void linux_tcp_conn_t::read_more_buffered() {

    rassert(!read_cond);
    if (read_was_shut_down) throw read_closed_exc_t();

    size_t old_size = read_buffer.size();
    read_buffer.resize(old_size + IO_BUFFER_SIZE);
    size_t delta = read_internal(read_buffer.data() + old_size, IO_BUFFER_SIZE);

    read_buffer.resize(old_size + delta);
}

const_charslice linux_tcp_conn_t::peek() const {

    rassert(!read_cond);   // Is there a read already in progress?
    if (read_was_shut_down) throw read_closed_exc_t();

    return const_charslice(read_buffer.data(), read_buffer.data() + read_buffer.size());
}

void linux_tcp_conn_t::pop(size_t len) {

    rassert(!read_cond);
    if (read_was_shut_down) throw read_closed_exc_t();

    rassert(len <= read_buffer.size());
    read_buffer.erase(read_buffer.begin(), read_buffer.begin() + len);  // INEFFICIENT
}

void linux_tcp_conn_t::shutdown_read() {
    int res = ::shutdown(sock.get(), SHUT_RD);
    if (res != 0 && errno != ENOTCONN) {
        logERR("Could not shutdown socket for reading: %s\n", strerror(errno));
    }

    on_shutdown_read();
}

void linux_tcp_conn_t::on_shutdown_read() {
    rassert(!read_was_shut_down);
    read_was_shut_down = true;

    // Stop watching for read events on the event loop.
    event_watcher.adjust(poll_event_in, 0);

    // Inform any readers that were waiting that the socket has been closed.
    if (read_cond) {
        cond_t *rc = read_cond;
        read_cond = NULL;
        rc->pulse();
    }
}

bool linux_tcp_conn_t::is_read_open() {
    return !read_was_shut_down;
}

void linux_tcp_conn_t::write_internal(const void *buf, size_t size) {
    rassert(!write_was_shut_down);

    while (size > 0) {
        int res = ::write(sock.get(), buf, size);

        if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {

            /* Wait for a notification from the event queue */
            cond_t cond;
            write_cond = &cond;
            event_watcher.adjust(poll_event_out, poll_event_out);
            cond.wait();

            /* The thing that signalled us set write_cond to NULL and also deregistered
            us for write events */

            if (write_was_shut_down) {
                /* We were closed for whatever reason. Whatever signalled us has already called
                   on_shutdown_write(). */
                throw write_closed_exc_t();
            }

            /* Go around the loop and try to read again */

        } else if (res == -1 && (errno == EPIPE || errno == ENOTCONN || errno == EHOSTUNREACH ||
                                 errno == ENETDOWN || errno == EHOSTDOWN || errno == ECONNRESET)) {
            /* These errors are expected to happen at some point in practice */
            on_shutdown_write();
            throw write_closed_exc_t();

        } else if (res == -1) {
            /* In theory this should never happen, but it probably will. So we write a log message
               and then shut down normally. */
            logERR("Could not write to socket: %s\n", strerror(errno));
            on_shutdown_write();
            throw write_closed_exc_t();

        } else if (res == 0) {
            /* This should never happen either, but it's better to write an error message than to
               crash completely. */
            logERR("Didn't expect write() to return 0.\n");
            on_shutdown_write();
            throw write_closed_exc_t();

        } else {
            rassert(res <= (int)size);
            buf = reinterpret_cast<const void *>(reinterpret_cast<const char *>(buf) + res);
            size -= res;
        }
    }
}

void linux_tcp_conn_t::write(const void *buf, size_t size) {

    rassert(!write_cond);
    if (write_was_shut_down) throw write_closed_exc_t();

    /* To preserve ordering, all buffered data must be sent before this chunk of data is sent */
    flush_buffer();

    /* Now send this chunk of data */
    write_internal(buf, size);
}

void linux_tcp_conn_t::write_buffered(const void *buf, size_t size) {

    rassert(!write_cond);
    if (write_was_shut_down) throw write_closed_exc_t();

    /* Append data to write_buffer */
    int old_size = write_buffer.size();
    write_buffer.resize(old_size + size);
    memcpy(write_buffer.data() + old_size, buf, size);
}

void linux_tcp_conn_t::flush_buffer() {

    rassert(!write_cond);
    if (write_was_shut_down) throw write_closed_exc_t();

    write_internal(write_buffer.data(), write_buffer.size());
    write_buffer.clear();
}

void linux_tcp_conn_t::shutdown_write() {
    int res = ::shutdown(sock.get(), SHUT_WR);
    if (res != 0 && errno != ENOTCONN) {
        logERR("Could not shutdown socket for writing: %s\n", strerror(errno));
    }

    on_shutdown_write();
}

void linux_tcp_conn_t::on_shutdown_write() {
    rassert(!write_was_shut_down);
    write_was_shut_down = true;

    // Stop listening for write events on the event loop
    event_watcher.adjust(poll_event_out, 0);

    // Inform any writers that were waiting that the socket has been closed.
    if (write_cond) {
        cond_t *wc = write_cond;
        write_cond = NULL;
        wc->pulse();
    }
}

bool linux_tcp_conn_t::is_write_open() {
    return !write_was_shut_down;
}

linux_tcp_conn_t::~linux_tcp_conn_t() {
    if (is_read_open()) shutdown_read();
    if (is_write_open()) shutdown_write();

    /* scoped_fd_t's destructor will take care of close()ing the socket. */
}

void linux_tcp_conn_t::on_event(int events) {
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

        rassert(!read_was_shut_down);

        /* Deregister for further read events */
        event_watcher.adjust(poll_event_in, 0);

        if (read_cond) {
            cond_t *rc = read_cond;
            read_cond = NULL;
            rc->pulse();
        }
    }

    if (events & poll_event_out) {

        rassert(!write_was_shut_down);

        /* Deregister ourself for further write events in case we're on a level-triggered system.
        If we are on a level-triggered system and we remain registered for write events,
        we will get a flood of write events, the CPU will spin, signals will be starved out,
        and Bad Things will happen. */
        event_watcher.adjust(poll_event_out, 0);

        if (write_cond) {
            cond_t *wc = write_cond;
            write_cond = NULL;
            wc->pulse();
        }
    }
}

/* Network listener object */

linux_tcp_listener_t::linux_tcp_listener_t(int port)
    : callback(NULL)
{
    int res;

    sock.reset(socket(AF_INET, SOCK_STREAM, 0));
    guarantee_err(sock.get() != INVALID_FD, "Couldn't create socket");

    int sockoptval = 1;
    res = setsockopt(sock.get(), SOL_SOCKET, SO_REUSEADDR, &sockoptval, sizeof(sockoptval));
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
    res = setsockopt(sock.get(), IPPROTO_TCP, TCP_NODELAY, &sockoptval, sizeof(sockoptval));
    guarantee_err(res != -1, "Could not set TCP_NODELAY option");

    // Bind the socket
    sockaddr_in serv_addr;
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    res = bind(sock.get(), (sockaddr*)&serv_addr, sizeof(serv_addr));
    if (res != 0) {
        if (errno == EADDRINUSE) {
            throw address_in_use_exc_t();
        } else {
            crash("Could not bind socket: %s\n", strerror(errno));
        }
    }

    // Start listening to connections
    res = listen(sock.get(), 5);
    guarantee_err(res == 0, "Couldn't listen to the socket");

    res = fcntl(sock.get(), F_SETFL, O_NONBLOCK);
    guarantee_err(res == 0, "Could not make socket non-blocking");
}

void linux_tcp_listener_t::set_callback(linux_tcp_listener_callback_t *cb) {
    rassert(!callback);
    rassert(cb);
    callback = cb;

    linux_thread_pool_t::thread->queue.watch_resource(sock.get(), poll_event_in, this);
}

void linux_tcp_listener_t::handle(fd_t socket) {
    boost::scoped_ptr<linux_tcp_conn_t> conn(new linux_tcp_conn_t(socket));
    callback->on_tcp_listener_accept(conn);
}

void linux_tcp_listener_t::on_event(int events) {
    if (events != poll_event_in) {
        logERR("Unexpected event mask: %d\n", events);
    }

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int new_sock = accept(sock.get(), (sockaddr*)&client_addr, &client_addr_len);

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
                    case EMFILE:
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
            coro_t::spawn_now(boost::bind(&linux_tcp_listener_t::handle, this, new_sock));
        }
    }
}

linux_tcp_listener_t::~linux_tcp_listener_t() {
    int res;

    if (callback) linux_thread_pool_t::thread->queue.forget_resource(sock.get(), this);

    res = shutdown(sock.get(), SHUT_RDWR);
    guarantee_err(res == 0, "Could not shutdown main socket");

    // scoped_fd_t destructor will close() the socket
}

