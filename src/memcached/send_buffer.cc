#include "send_buffer.hpp"

send_buffer_t::send_buffer_t(net_conn_t *conn)
    : conn(conn), flush_cb(NULL),
      external_write_buffer(NULL), external_write_size(0), external_write_cb(NULL)
{
}

void send_buffer_t::write(size_t s, const char *v) {

    assert(!external_write_cb, "Can't write to a send buffer while it's doing an external write.");
    assert(!flush_cb, "Can't write to a send_buffer_t while it's flushing.");

    /* Don't immediately write; instead, buffer the data. */

    size_t bytes = buffer.size();
    buffer.resize(bytes + s);
    memcpy(buffer.data() + bytes, v, s);
}

void send_buffer_t::write_external(size_t s, const char *v, send_buffer_external_write_callback_t *cb) {

    assert(!external_write_cb, "Can't write to a send buffer while it's doing an external write.");
    assert(!flush_cb, "Can't write to a send_buffer_t while it's flushing.");
    assert(!conn->closed());

    external_write_cb = cb;
    external_write_buffer = v;
    external_write_size = s;

    if (buffer.size()) {
        /* There's data in our internal buffer that we haven't flushed yet. Flush it now
        before we write the external buffer. */
        conn->write_external(buffer.data(), buffer.size(), this);
    } else {
        conn->write_external(external_write_buffer, external_write_size, this);
    }
}

void send_buffer_t::flush(send_buffer_callback_t *cb) {

    assert(!flush_cb);
    assert(!external_write_cb);

    flush_cb = cb;
    assert(!conn->closed());
    conn->write_external(buffer.data(), buffer.size(), this);
}

void send_buffer_t::on_net_conn_write_external() {
    if (flush_cb) {
        /* We just finished flushing the internal buffer by request of the request handler. */
        send_buffer_callback_t *cb = flush_cb;
        flush_cb = NULL;
        buffer.resize(0);
        cb->on_send_buffer_flush();
    } else if (external_write_cb) {
        if (buffer.size()) {
            /* We just finished flushing the internal buffer in preparation for
            writing an external buffer. */
            buffer.resize(0);
            conn->write_external(external_write_buffer, external_write_size, this);
        } else {
            /* We just finished writing the external buffer. */
            send_buffer_external_write_callback_t *cb = external_write_cb;
            external_write_cb = NULL;
            cb->on_send_buffer_write_external();
        }
    } else {
        /* We're writing but we can't figure out why... */
        unreachable();
    }
}

void send_buffer_t::on_net_conn_close() {
    if (flush_cb) {
        /* We were interrupted while flushing the internal buffer by
        request of the request handler. */
        send_buffer_callback_t *cb = flush_cb;
        flush_cb = NULL;
        cb->on_send_buffer_socket_closed();
    } else if (external_write_cb) {
        /* We were interrupted while writing an external buffer or while
        flushing the internal buffer in preparation for writing an
        external buffer. */
        send_buffer_external_write_callback_t *cb = external_write_cb;
        external_write_cb = NULL;
        cb->on_send_buffer_socket_closed();
    } else {
        /* We're writing but we can't figure out why... */
        unreachable();
    }
}
