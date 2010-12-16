#include "arch/linux/network.hpp"
#include "arch/linux/thread_pool.hpp"
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>

/* Network connection object */

linux_net_conn_t::linux_net_conn_t(const char *host, int port) {
    not_implemented();
}

linux_net_conn_t::linux_net_conn_t(fd_t sock)
    : sock(sock),
      registration_thread(-1), set_me_true_on_delete(NULL),
      read_mode(read_mode_none), in_read_buffered_cb(false),
      write_mode(write_mode_none),
      was_shut_down(false)
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

void linux_net_conn_t::read_external(void *buf, size_t size, linux_net_conn_read_external_callback_t *cb) {
    assert(!was_shut_down);

    register_with_event_loop();
    assert(sock != INVALID_FD);
    assert(read_mode == read_mode_none);

    read_mode = read_mode_external;
    external_read_buf = (char *)buf;
    external_read_size = size;
    read_external_cb = cb;
    assert(read_external_cb);

    // If we were reading in read_mode_buffered before this read, we might have
    // read more bytes than necessary, in which case the peek buffer will still
    // contain some data. Drain it out first.
    int peek_buffer_bytes = std::min(peek_buffer.size(), external_read_size);
    memcpy(external_read_buf, peek_buffer.data(), peek_buffer_bytes);
    peek_buffer.erase(peek_buffer.begin(), peek_buffer.begin() + peek_buffer_bytes);
    external_read_buf += peek_buffer_bytes;
    external_read_size -= peek_buffer_bytes;

    try_to_read_external_buf();
}

void linux_net_conn_t::try_to_read_external_buf() {
    assert(read_mode == read_mode_external);

    // The only time when external_read_size would be zero here is if read() was called to ask for
    // zero bytes.
    if (external_read_size > 0) {
        assert(external_read_buf);
        int res = ::read(sock, external_read_buf, external_read_size);

        if (res == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // We'll get called again via on_event() when there is more data
                return;
            } else if (errno == ECONNRESET || errno == ENOTCONN) {
                // Socket was closed
                on_shutdown();
            } else {
                crash("Could not read from socket: %s", strerror(errno));
            }

        } else if (res == 0) {
            // Socket was closed
            on_shutdown();

        } else {
            external_read_size -= res;
            external_read_buf += res;
        }
    }

    if (external_read_size == 0) {
        // The request has been fulfilled
        read_mode = read_mode_none;
        read_external_cb->on_net_conn_read_external();
    }
}

void linux_net_conn_t::read_buffered(linux_net_conn_read_buffered_callback_t *cb) {
    assert(!was_shut_down);

    register_with_event_loop();
    assert(sock != INVALID_FD);
    assert(read_mode == read_mode_none);

    read_mode = read_mode_buffered;
    read_buffered_cb = cb;

    // We call see_if_callback_is_satisfied() first because there might be data
    // already in the peek buffer, or the callback might be satisfied with an empty
    // peek buffer.

    if (!see_if_callback_is_satisfied()) put_more_data_in_peek_buffer();
}

void linux_net_conn_t::put_more_data_in_peek_buffer() {
    assert(read_mode == read_mode_buffered);

    // Grow the peek buffer so we have some space to put what we're about to insert
    int old_size = peek_buffer.size();
    peek_buffer.resize(old_size + IO_BUFFER_SIZE);

    int res = ::read(sock, peek_buffer.data() + old_size, IO_BUFFER_SIZE);

    if (res == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // We will get a callback via on_event() at a later date
            return;
        } else if (errno == ECONNRESET || errno == ENOTCONN) {
            // Socket was closed
            on_shutdown();
        } else {
            crash("Could not read from socket: %s", strerror(errno));
        }

    } else if (res == 0) {
        // Socket was closed
        on_shutdown();

    } else {
        // Shrink the peek buffer so that its 'size' member is only how many bytes are
        // actually in it. Its internal array will probably not shrink.
        peek_buffer.resize(old_size + res);

        if (!see_if_callback_is_satisfied()) {
            if (res == IO_BUFFER_SIZE) {
                // There might be more data in the kernel buffer
                put_more_data_in_peek_buffer();
            }
        }
    }
}

bool linux_net_conn_t::see_if_callback_is_satisfied() {
    assert(read_mode == read_mode_buffered);
    assert(!in_read_buffered_cb);

    in_read_buffered_cb = true;   // Make it legal to call accept_buffer()

    read_buffered_cb->on_net_conn_read_buffered(peek_buffer.data(), peek_buffer.size());

    if (in_read_buffered_cb) {
        // accept_buffer() was not called; our offer was rejected.
        in_read_buffered_cb = false;
        return false;

    } else {
        // accept_buffer() was called and it set in_read_buffered_cb to false. It also
        // already removed the appropriate amount of data from the peek buffer and set
        // the read mode to read_mode_none. The read_buffered_cb might have gone on to
        // start another read, though, so there's no guarantee that the read mode is
        // still read_mode_none.
        return true;

    }
}

void linux_net_conn_t::accept_buffer(size_t bytes) {
    assert(read_mode == read_mode_buffered);
    assert(in_read_buffered_cb);

    assert(bytes <= peek_buffer.size());
    peek_buffer.erase(peek_buffer.begin(), peek_buffer.begin() + bytes);

    // So that the callback can start another read after calling accept_buffer()
    in_read_buffered_cb = false;
    read_mode = read_mode_none;
}

void linux_net_conn_t::write_external(const void *buf, size_t size, linux_net_conn_write_external_callback_t *cb) {
    assert(!was_shut_down);

    register_with_event_loop();
    assert(sock != INVALID_FD);
    assert(write_mode == write_mode_none);

    write_mode = write_mode_external;
    external_write_buf = (const char *)buf;
    external_write_size = size;
    write_external_cb = cb;
    assert(write_external_cb);
    try_to_write_external_buf();
}

void linux_net_conn_t::try_to_write_external_buf() {
    assert(write_mode == write_mode_external);

    // The only time when external_write_size would be zero here is if write() was called
    // with zero bytes.
    if (external_write_size > 0) {
        assert(external_write_buf);
        int res = ::write(sock, external_write_buf, external_write_size);
        if (res == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOBUFS) {
                return;
            } else if (errno == EPIPE || errno == ENOTCONN || errno == EHOSTUNREACH ||
                    errno == ENETDOWN || errno == EHOSTDOWN) {
                on_shutdown();
                return;
            } else {
                crash("Could not write to socket: %s", strerror(errno));
            }
        } else if (res == 0) {
            crash("Didn't expect write() to return 0");
        } else {
            external_write_size -= res;
            external_write_buf += res;
        }
    }

    if (external_write_size == 0) {
        // The request has been fulfilled
        write_mode = write_mode_none;
        write_external_cb->on_net_conn_write_external();
    }
}

void linux_net_conn_t::shutdown() {
    assert(!in_read_buffered_cb, "Please don't call net_conn_t::shutdown() from within "
        "on_net_conn_read_buffered() without calling accept_buffer(). The net_conn_t is "
        "sort of stupid and you just broke its fragile little mind.");

    int res = ::shutdown(sock, SHUT_RDWR);
    if (res != 0 && errno != ENOTCONN) {
        crash("Could not shutdown socket: %s", strerror(errno));
    }

    on_shutdown();
}

void linux_net_conn_t::on_shutdown() {
    assert(!was_shut_down);
    assert(sock != INVALID_FD);
    was_shut_down = true;

    // Inform any readers or writers that were waiting that the socket has been closed.

    // If there are no readers or writers, nothing gets informed until an attempt is made
    // to read or write. Is this a problem?

    switch (read_mode) {
        case read_mode_none: break;
        case read_mode_external: read_external_cb->on_net_conn_close(); break;
        case read_mode_buffered: read_buffered_cb->on_net_conn_close(); break;
        default: unreachable();
    }

    switch (write_mode) {
        case write_mode_none: break;
        case write_mode_external: write_external_cb->on_net_conn_close(); break;
        default: unreachable();
    }

    // Deregister ourself with the event loop

    if (registration_thread != -1) {
        assert(registration_thread == linux_thread_pool_t::thread_id);
        linux_thread_pool_t::thread->queue.forget_resource(sock, this);
    }
}

bool linux_net_conn_t::closed() {
    return was_shut_down;
}

linux_net_conn_t::~linux_net_conn_t() {
    // sock would be INVALID_FD if our sock was stolen by a
    // linux_oldstyle_net_conn_t.
    if (sock != INVALID_FD) {
        // So on_event() doesn't call us after we got deleted
        if (set_me_true_on_delete) *set_me_true_on_delete = true;

        assert(was_shut_down);
        ::close(sock);
    }
}

void linux_net_conn_t::on_event(int events) {
    assert(sock != INVALID_FD);
    assert(!was_shut_down);

    // So we get notified if 'this' gets deleted and don't try to do any more
    // operations with it.
    bool deleted = false;
    set_me_true_on_delete = &deleted;

    if (events & poll_event_in) {
        switch (read_mode) {
            case read_mode_none: break;
            case read_mode_external: try_to_read_external_buf(); break;
            case read_mode_buffered: put_more_data_in_peek_buffer(); break;
            default: unreachable();
        }
        if (deleted || was_shut_down) {
            set_me_true_on_delete = NULL;
            return;
        }
    }
    if (events & poll_event_out) {
        switch (write_mode) {
            case write_mode_none: break;
            case write_mode_external: try_to_write_external_buf(); break;
            default: unreachable();
        }
        if (deleted || was_shut_down) {
            set_me_true_on_delete = NULL;
            return;
        }
    }
    if (events & poll_event_err) {
        // Should this be on_shutdown()? The difference is that by calling shutdown(), we
        // cause ::shutdown() to be called on our socket. But if there was a socket error,
        // it might have shut itself down already. This is probably safe.
        shutdown();
        set_me_true_on_delete = NULL;
        return;
    }

    set_me_true_on_delete = NULL;
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
                        guarantee_err(false, "Cannot accept new connection");
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

/* Old-style network connection object */

linux_oldstyle_net_conn_t::linux_oldstyle_net_conn_t(linux_net_conn_t *conn)
    : sock(conn->sock), callback(NULL), set_me_true_on_delete(NULL),
      registered_for_write_notifications(false)
{
    assert(sock != INVALID_FD);

    // Destroy the original wrapper since we own the socket now
    assert(conn->registration_thread == -1);
    conn->sock = INVALID_FD;
}

void linux_oldstyle_net_conn_t::set_callback(linux_oldstyle_net_conn_callback_t *cb) {
    assert(!callback);
    assert(cb);
    callback = cb;

    linux_thread_pool_t::thread->queue.watch_resource(sock, poll_event_in, this);
}

ssize_t linux_oldstyle_net_conn_t::read_nonblocking(void *buf, size_t count) {
    return ::read(sock, buf, count);
}

ssize_t linux_oldstyle_net_conn_t::write_nonblocking(const void *buf, size_t count) {
    int res = ::write(sock, buf, count);
    if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
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

void linux_oldstyle_net_conn_t::on_event(int events) {
    assert(callback);

    // So we can tell if the callback deletes the linux_oldstyle_net_conn_t
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
        crash("epoll_wait came back with an unhandled event");
    }

    set_me_true_on_delete = NULL;
}

linux_oldstyle_net_conn_t::~linux_oldstyle_net_conn_t() {
    if (set_me_true_on_delete)
        *set_me_true_on_delete = true;

    if (sock != INVALID_FD) {
        if (callback) linux_thread_pool_t::thread->queue.forget_resource(sock, this);
        ::close(sock);
        sock = INVALID_FD;
    }
}

