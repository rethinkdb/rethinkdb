#include "containers/archive.hpp"

#include "arch/io/network.hpp"

tcp_conn_stream_t::tcp_conn_stream_t(const char *host, int port, int local_port)
    : conn_(new tcp_conn_t(host, port, local_port)) { }

tcp_conn_stream_t::tcp_conn_stream_t(const ip_address_t &host, int port, int local_port)
    : conn_(new tcp_conn_t(host, port, local_port)) { }

tcp_conn_stream_t::~tcp_conn_stream_t() {
    delete conn_;
}

int64_t tcp_conn_stream_t::read(void *p, int64_t n) {
    // Returns the number of bytes read, or 0 upon EOF, -1 upon error.
    // Right now this function cannot "error".
    try {
        size_t result = conn_->read_some(p, n);
        rassert(result > 0);
        rassert(int64_t(result) <= n);
        return result;
    } catch (tcp_conn_t::read_closed_exc_t &e) {
        return 0;
    }
}

int64_t tcp_conn_stream_t::write(const void *p, int64_t n) {
    try {
        // write writes everything or throws an exception.
        conn_->write(p, n);
        return n;
    } catch (tcp_conn_t::write_closed_exc_t &e) {
        return -1;
    }
}

void write_message_t::append(const void *p, int64_t n) {
    while (n > 0) {
        if (buffers_.empty() || buffers_.tail()->size == write_buffer_t::DATA_SIZE) {
            buffers_.push_back(new write_buffer_t);
        }

        write_buffer_t *b = buffers_.tail();
        int64_t k = std::min<int64_t>(n, write_buffer_t::DATA_SIZE - b->size);

        memcpy(b->data + b->size, p, k);
        b->size += k;
        p = static_cast<const char *>(p) + k;
        n = n - k;
    }
}

