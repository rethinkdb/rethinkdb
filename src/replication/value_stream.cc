#include "value_stream.hpp"

namespace replication {

value_stream_t::value_stream_t()
    : read_mode(read_mode_none), closed(false), buffer() { }

value_stream_t::value_stream_t(const char *beg, const char *end)
    : read_mode(read_mode_none), closed(false), buffer(beg, end) { }

void value_stream_t::read_external(char *buf, size_t size, value_stream_read_external_callback_t *cb) {
    guarantee(read_mode == read_mode_none);
    guarantee(buf != NULL);

    read_mode = read_mode_external;
    external_buf = buf;
    external_buf_size = size;
    external_cb = cb;

    try_report_external();
}

void value_stream_t::try_report_external() {
    guarantee(read_mode == read_mode_external);

    size_t copy_size = std::min(external_buf_size, buffer.size());

    memcpy(external_buf, buffer.data(), copy_size);

    external_buf += copy_size;
    external_buf_size -= copy_size;
    buffer.erase(buffer.begin(), buffer.begin() + copy_size);  // INEFFICIENT

    if (external_buf_size == 0 || closed) {
        value_stream_read_external_callback_t *cb = external_cb;

        read_mode = read_mode_none;
        external_buf = NULL;
        external_buf_size = 0;
        external_cb = NULL;

        if (external_buf_size == 0) {
            cb->on_read_external();
        } else {
            cb->on_read_close(copy_size);
        }
    }
}

void value_stream_t::read_fixed_buffered(size_t size, value_stream_read_fixed_buffered_callback_t *cb) {
    guarantee(read_mode == read_mode_none);

    read_mode = read_mode_fixed;
    desired_fixed_size = size;
    fixed_cb = cb;

    try_report_fixed();
}

void value_stream_t::try_report_fixed() {
    guarantee(read_mode == read_mode_fixed);

    if (desired_fixed_size < buffer.size()) {
        cookie_t cookie;
        fixed_cb->on_read_fixed_buffered(buffer.data(), &cookie);
        guarantee(cookie.acknowledged);
    }
}

void value_stream_t::acknowledge_fixed(cookie_t *cookie) {
    guarantee(read_mode == read_mode_fixed);
    guarantee(cookie->acknowledged == false);

    buffer.erase(buffer.begin(), buffer.begin() + desired_fixed_size);
    read_mode = read_mode_none;
    desired_fixed_size = 0;
    cookie->acknowledged = true;
}

void value_stream_t::inform_space_written(size_t amount_written, write_cookie_t *cookie) {
    guarantee(cookie->acknowledged == false);
    guarantee(amount_written <= cookie->space_allocated);
    guarantee(cookie->space_allocated <= buffer.size());

    buffer.erase(buffer.end() - cookie->space_allocated + amount_written, buffer.end());
    cookie->acknowledged = true;

    try_report_new_data();
}

void value_stream_t::inform_closed(write_cookie_t *cookie) {
    guarantee(cookie->acknowledged == false);
    guarantee(cookie->space_allocated <= buffer.size());

    buffer.erase(buffer.end() - cookie->space_allocated, buffer.end());
    cookie->acknowledged = true;

    try_report_new_data();
}

void value_stream_t::try_report_new_data() {
    if (read_mode == read_mode_fixed) {
        try_report_fixed();
    }
    if (read_mode == read_mode_external) {
        try_report_external();
    }
}


}  // namespace replication
