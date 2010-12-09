#include "arch/linux/network.hpp"
#include "arch/linux/thread_pool.hpp"
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>

/* Network connection object */

linux_net_conn_t::linux_net_conn_t(const char *host, int port) {
    fail("Not implemented");
}

linux_net_conn_t::linux_net_conn_t(fd_t sock)
    : sock(sock), registration_cpu(-1), read_size(0), write_size(0), was_shut_down(false),
      set_me_true_on_delete(NULL)
{
    assert(sock != INVALID_FD);

    int res = fcntl(sock, F_SETFL, O_NONBLOCK);
    check("Could not make socket non-blocking", res != 0);
}

void linux_net_conn_t::register_with_event_loop() {

    if (registration_cpu == -1) {
        registration_cpu = linux_thread_pool_t::cpu_id;
        linux_thread_pool_t::thread->queue.watch_resource(sock, poll_event_in|poll_event_out, this);

    } else if (registration_cpu != linux_thread_pool_t::cpu_id) {
        fail("Must always use a net_conn_t on the same CPU.");
    }
}

void linux_net_conn_t::read(void *buf, size_t size, linux_net_conn_read_callback_t *cb) {

    if (was_shut_down) {
        cb->on_net_conn_close();
        return;
    };

    if (read_size) fail("Already reading.");

    register_with_event_loop();

    assert(sock != INVALID_FD);

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
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            else fail("Could not read from socket: %s", strerror(errno));
        } else if (res == 0) {
            // Socket was closed
            on_shutdown();
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

    if (was_shut_down) {
        cb->on_net_conn_close();
        return;
    };

    if (write_size) fail("Already writing.");

    register_with_event_loop();

    assert(sock != INVALID_FD);

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
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            } else if (errno == EPIPE) {
                on_shutdown();
                return;
            } else {
                fail("Could not write to socket: %s", strerror(errno));
            }
        } else if (res == 0) {
            fail("Didn't expect write() to return 0");
        } else {
            write_size -= res;
            write_buf = (void*)((char*)write_buf + res);
        }
    }
    if (write_size == 0) {
        write_cb->on_net_conn_write();
    }
}

void linux_net_conn_t::shutdown() {

    int res = ::shutdown(sock, SHUT_RDWR);
    if (res != 0 && errno != ENOTCONN) {
        fail("Could not shutdown socket: %s", strerror(errno));
    }

    on_shutdown();
}

void linux_net_conn_t::on_shutdown() {

    assert(!was_shut_down);
    assert(sock != INVALID_FD);
    was_shut_down = true;

    if (read_size) read_cb->on_net_conn_close();
    if (write_size) write_cb->on_net_conn_close();

    if (registration_cpu != -1) {
        assert(registration_cpu == linux_thread_pool_t::cpu_id);
        linux_thread_pool_t::thread->queue.forget_resource(sock, this);
    }
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

    bool deleted = false;
    set_me_true_on_delete = &deleted;
    if ((events & poll_event_in) && read_size > 0) {
        pump_read();
        if (deleted || was_shut_down) return;
    }
    if ((events & poll_event_out) && write_size > 0) {
        pump_write();
        if (deleted || was_shut_down) return;
    }
    if (events & poll_event_err) {
        shutdown();
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

    linux_thread_pool_t::thread->queue.watch_resource(sock, poll_event_in, this);
}

void linux_net_listener_t::on_event(int events) {

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

    if (callback) linux_thread_pool_t::thread->queue.forget_resource(sock, this);

    res = shutdown(sock, SHUT_RDWR);
    check("Could not shutdown main socket", res == -1);

    res = close(sock);
    check("Could not close main socket", res != 0);
}

/* Old-style network connection object */

linux_oldstyle_net_conn_t::linux_oldstyle_net_conn_t(linux_net_conn_t *conn)
    : sock(conn->sock), callback(NULL), set_me_true_on_delete(NULL),
      registered_for_write_notifications(false)
{
    assert(sock != INVALID_FD);

    // Destroy the original wrapper since we own the socket now
    assert(conn->registration_cpu == -1);
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
        fail("epoll_wait came back with an unhandled event");
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

