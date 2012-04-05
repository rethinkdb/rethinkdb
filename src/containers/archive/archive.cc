#include "containers/archive/archive.hpp"

#include <string.h>

#include <algorithm>

int64_t force_read(read_stream_t *s, void *p, int64_t n) {
    rassert(n >= 0);

    char *chp = static_cast<char *>(p);
    int64_t written_so_far = 0;
    while (n > 0) {
        int64_t res = s->read(chp, n);
        if (res == 0) {
            return written_so_far;
        }
        if (res == -1) {
            // We don't communicate what data has been read so far.
            // I'm guessing callers to force_read don't care.
            return -1;
        }
        rassert(res < n);

        written_so_far += res;
        chp += res;
        n -= res;
    }
    return written_so_far;
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

