#include "send_buffer.hpp"

send_buffer_t::send_buffer_t(net_conn_t *conn)
    : conn(conn), flush_cb(NULL)
{
}

void send_buffer_t::write(size_t s, const char *v) {

    assert(!flush_cb, "Can't write to a send_buffer_t while it's flushing.");

    size_t bytes = buffer.size();
    buffer.resize(bytes + s);
    memcpy(buffer.data() + bytes, v, s);
}

void send_buffer_t::flush(send_buffer_callback_t *cb) {
    assert(!flush_cb);
    flush_cb = cb;
    assert(!conn->closed());
    conn->write_external(buffer.data(), buffer.size(), this);
}

void send_buffer_t::on_net_conn_write_external() {
    assert(flush_cb);
    send_buffer_callback_t *cb = flush_cb;
    flush_cb = NULL;
    buffer.resize(0);
    cb->on_send_buffer_flush();
}

void send_buffer_t::on_net_conn_close() {
    assert(flush_cb);
    send_buffer_callback_t *cb = flush_cb;
    flush_cb = NULL;
    cb->on_send_buffer_socket_closed();
}
