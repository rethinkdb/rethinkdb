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
    : sock(sock), registration_cpu(-1), read_size(0), write_size(0), set_me_true_on_delete(NULL)
{
    int res = fcntl(sock, F_SETFL, O_NONBLOCK);
    guarantee_err(res == 0, "Could not make socket non-blocking");
}

void linux_net_conn_t::register_with_event_loop() {
    if (registration_cpu == -1) {
        registration_cpu = linux_thread_pool_t::cpu_id;
        linux_thread_pool_t::thread->queue.watch_resource(sock, poll_event_in|poll_event_out, this);
    } else {
        guarantee(registration_cpu == linux_thread_pool_t::cpu_id,
            "Must always use a net_conn_t on the same CPU.");
    }
}

void linux_net_conn_t::read(void *buf, size_t size, linux_net_conn_read_callback_t *cb) {
    guarantee(read_size == 0, "Already reading.");
    register_with_event_loop();

    read_buf = buf;
    read_size = size;
    read_cb = cb;
    assert(read_cb);
    pump_read();
}

void linux_net_conn_t::pump_read() {
    if (read_size > 0) {
        assert(read_buf);
        int res = ::read(sock, read_buf, read_size);
        if (res == -1) {
            guarantee_err(errno == EAGAIN || errno == EWOULDBLOCK, "Could not read from socket");
            // RSI do we need to check for EPIPE here, like in pump_write below?
            return;
        } else if (res == 0) {
            // Socket was closed
            delete this;
        } else {
            read_size -= res;
            read_buf = (void*)((char*)read_buf + res);
        }
    }
    if (read_size == 0) {
        read_cb->on_net_conn_read();
    }
}

void linux_net_conn_t::write(const void *buf, size_t size, linux_net_conn_write_callback_t *cb) {
    guarantee(write_size == 0, "Already writing.");
    register_with_event_loop();

    write_buf = buf;
    write_size = size;
    write_cb = cb;
    assert(write_cb);
    pump_write();
}

void linux_net_conn_t::pump_write() {
    if (write_size > 0) {
        assert(write_buf);
        int res = ::write(sock, write_buf, write_size);
        if (res == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            else if (errno == EPIPE) delete this;   // RSI do we need to return here?
            else guarantee_err(false, "Could not write to socket");
        } else if (res == 0) {
            crash("Didn't expect write() to return 0"); // TODO from the write man page it seems that
                                                        // write may return 0 sometimes. Should we
                                                        // really crash?
        } else {
            write_size -= res;
            write_buf = (void*)((char*)write_buf + res);
        }
    }
    if (write_size == 0) {
        write_cb->on_net_conn_write();
    }
}

linux_net_conn_t::~linux_net_conn_t() {
    // So on_event() doesn't call us after we got deleted
    if (set_me_true_on_delete) *set_me_true_on_delete = true;

    if (registration_cpu != -1) {
        assert(registration_cpu == linux_thread_pool_t::cpu_id);
        linux_thread_pool_t::thread->queue.forget_resource(sock, this);
    }

    // sock would be INVALID_FD if our sock was stolen by a
    // linux_oldstyle_net_conn_t.
    if (sock != INVALID_FD) close(sock);

    if (read_size) read_cb->on_net_conn_close();
    if (write_size) write_cb->on_net_conn_close();
}

void linux_net_conn_t::on_event(int events) {
    bool deleted = false;
    set_me_true_on_delete = &deleted;
    if ((events & poll_event_in) && read_size > 0) {
        pump_read();
        if (deleted) return;
    }
    if ((events & poll_event_out) && write_size > 0) {
        pump_write();
        if (deleted) return;
    }
    if (events & poll_event_err) {
        delete this;
        return;
    }
    set_me_true_on_delete = NULL;
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
    guarantee_err(res != -1, "Could not set REUSEADDR option");

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
    conn->sock = INVALID_FD;
    delete conn;
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

