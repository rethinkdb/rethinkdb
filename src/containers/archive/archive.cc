#include "containers/archive/archive.hpp"

#include <string.h>

#include <algorithm>

#include "containers/uuid.hpp"

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
        rassert(res <= n);

        written_so_far += res;
        chp += res;
        n -= res;
    }
    return written_so_far;
}

write_message_t::~write_message_t() {
    while (write_buffer_t *buffer = buffers_.head()) {
        buffers_.remove(buffer);
        delete buffer;
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

int send_write_message(write_stream_t *s, const write_message_t *msg) {
    intrusive_list_t<write_buffer_t> *list = const_cast<write_message_t *>(msg)->unsafe_expose_buffers();
    for (write_buffer_t *p = list->head(); p; p = list->next(p)) {
        int64_t res = s->write(p->data, p->size);
        if (res == -1) {
            return -1;
        }
        rassert(res == p->size);
    }
    return 0;
}


write_message_t &operator<<(write_message_t &msg, const uuid_t &uuid) {
    msg.append(uuid.data(), uuid_t::static_size());
    return msg;
}

MUST_USE archive_result_t deserialize(read_stream_t *s, uuid_t *uuid) {
    int64_t sz = uuid_t::static_size();
    int64_t res = force_read(s, uuid->data(), sz);

    if (res == -1) { return ARCHIVE_SOCK_ERROR; }
    if (res < sz) { return ARCHIVE_SOCK_EOF; }
    rassert(res == sz);
    return ARCHIVE_SUCCESS;
}

