#include "send_buffer.hpp"

send_buffer_t::send_buffer_t(net_conn_t *conn)
    : conn(conn), flush_cb(NULL), writing(false), writing_buffer(&buffers[0]), growing_buffer(&buffers[1])
{
}

void send_buffer_t::write(size_t bytes, const char *buffer) {
    size_t s = growing_buffer->size();
    growing_buffer->resize(s + bytes);
    memcpy(growing_buffer->data() + s, buffer, bytes);

    if (!writing) {
        write_growing_buffer();
    }
}

void send_buffer_t::flush(send_buffer_callback_t *cb) {
    assert(!flush_cb);
    assert(!conn->closed());
    if (writing) {
        flush_cb = cb;
    } else {
        assert(growing_buffer->size() == 0);
        cb->on_send_buffer_flush();
    }
}

void send_buffer_t::on_net_conn_write_external() {
    writing = false;
    if (growing_buffer->size() != 0) {
        write_growing_buffer();
    } else if (flush_cb) {
        send_buffer_callback_t *cb = flush_cb;
        flush_cb = NULL;
        cb->on_send_buffer_flush();
    }
}

void send_buffer_t::on_net_conn_close() {
    if (flush_cb) {
        send_buffer_callback_t *cb = flush_cb;
        flush_cb = NULL;
        cb->on_send_buffer_socket_closed();
    }
}

void send_buffer_t::write_growing_buffer() {

    if (conn->closed()) return;

    assert(!writing);
    std::vector<char> *temp = writing_buffer;
    writing_buffer = growing_buffer;
    temp->resize(0);   // Wipe and reuse the buffer
    growing_buffer = temp;

    writing = true;
    conn->write_external(writing_buffer->data(), writing_buffer->size(), this);
}
