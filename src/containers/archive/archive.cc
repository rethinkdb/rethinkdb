#include "containers/archive/archive.hpp"

#include <string.h>

#include <algorithm>

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

