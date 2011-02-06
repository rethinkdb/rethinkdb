#include "containers/shared_buf.hpp"

shared_buf_t::shared_buf_t(weak_buf_t other) {
    rassert(!other.buffer_ || get_thread_id() == reinterpret_cast<shared_buf_buffer_t *>(other.buffer_ - sizeof(shared_buf_buffer_t))->home_thread);

    buffer_ = other.buffer_;
    if (buffer_) {
        shared_buf_buffer_t *shbuf = reinterpret_cast<shared_buf_buffer_t *>(buffer_ - sizeof(shared_buf_buffer_t));
        shbuf->refcount++;
    }
}
