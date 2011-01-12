#include "send_buffer.hpp"
#include "concurrency/task.hpp"

send_buffer_t::send_buffer_t(net_conn_t *conn)
    : conn(conn)
#ifndef NDEBUG
      , state(ready)
#endif
{ }

void send_buffer_t::write(size_t s, const char *v) {
    assert(state == ready, "The state was %d but it should have been %d", state, ready);

    /* Don't immediately write; instead, buffer the data. */

    size_t bytes = buffer.size();
    buffer.resize(bytes + s);
    memcpy(buffer.data() + bytes, v, s);
}

void send_buffer_t::write_external(size_t s, const char *v, send_buffer_external_write_callback_t *cb) {
    coro_t::spawn(&send_buffer_t::do_write_external, this, s, v, cb);
}

bool send_buffer_t::co_write_external(size_t bytes, const char *buffer) {
    
    /* There's probably a better way to do this. Maybe we can call do_write_external(), or a variant
    thereof, directly? */
    struct : public send_buffer_external_write_callback_t, value_cond_t<bool> {
        void on_send_buffer_write_external() { pulse(true); }
        void on_send_buffer_socket_closed() { pulse(false); }
    } cb;
    write_external(bytes, buffer, &cb);
    return cb.wait();
}

void send_buffer_t::do_write_external(size_t s, const char *v, send_buffer_external_write_callback_t *cb) {
#ifndef NDEBUG
    assert(state == ready);
    state = writing;
    assert(conn->is_write_open());
#endif

    if (buffer.size()) {
        /* There's data in our internal buffer that we haven't flushed yet. Flush it now
        before we write the external buffer. */
        net_conn_write_external_result_t result = ::co_write_external(conn, buffer.data(), buffer.size());
        if (result.result == net_conn_write_external_result_t::closed) {
            /* We just finished writing the external buffer. */
#ifndef NDEBUG
            state = ready;
#endif
            cb->on_send_buffer_socket_closed();
            return;
        }
        buffer.resize(0);
    }
    net_conn_write_external_result_t result = ::co_write_external(conn, v, s);
    if (result.result == net_conn_write_external_result_t::closed) {
#ifndef NDEBUG
        state = ready;
#endif
        cb->on_send_buffer_socket_closed();
        return;
    }

#ifndef NDEBUG
    state = ready;
#endif
    cb->on_send_buffer_write_external();
}

void send_buffer_t::flush(send_buffer_callback_t *cb) {
    coro_t::spawn(&send_buffer_t::do_flush, this, cb);
}

void send_buffer_t::do_flush(send_buffer_callback_t *cb) {
#ifndef NDEBUG
    assert(state == ready);
    state = flushing;
#endif

    assert(conn->is_write_open());
    net_conn_write_external_result_t result = ::co_write_external(conn, buffer.data(), buffer.size());
    if (result.result == net_conn_write_external_result_t::closed) {
        /* We were interrupted while flushing the internal buffer by
        request of the request handler. */

#ifndef NDEBUG
        state = ready;
#endif
        cb->on_send_buffer_socket_closed();
    } else {
        /* We just finished flushing the internal buffer by request of the request handler. */
        buffer.resize(0);

#ifndef NDEBUG
        state = ready;
#endif
        cb->on_send_buffer_flush();
    }
}

void send_buffer_t::discard() {
    buffer.clear();
}
