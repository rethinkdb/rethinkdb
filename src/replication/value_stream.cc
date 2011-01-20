#include "value_stream.hpp"

#include <string.h>

namespace replication {

value_stream_t::value_stream_t()
    : local_buffer_size(0), fixed_buffered_threshold(-1) { }

// TODO: Get rid of this constructor, and just call the methods.
value_stream_t::value_stream_t(const char *beg, const char *end)
    : local_buffer_size(0), fixed_buffered_threshold(-1) {
    charslice sl = buf_for_filling(end - beg);
    memcpy(sl.beg, beg, end - beg);
    data_written(end - beg);
}

value_stream_t::read_token_t value_stream_t::read_external(charslice buf) {
    reader_node_t *node = new reader_node_t();
    node->buf = buf;
    reader_list.push_back(node);

    return node;
}

bool value_stream_t::read_wait(read_token_t tok) {
    bool success = tok->var.wait();
    if (success) {
        zombie_reader_list.remove(tok);
    }

    return success;
}

bool value_stream_t::read_fixed_buffered(ssize_t threshold, charslice *slice_out) {
    assert(fixed_buffered_threshold == -1);
    if (local_buffer_size >= threshold) {
        *slice_out = charslice(local_buffer.data(), local_buffer.data() + local_buffer.size());
        return true;
    }
    fixed_buffered_threshold = threshold;
    bool success = fixed_buffered_cond.wait();
    if (success) {
        *slice_out = charslice(local_buffer.data(), local_buffer.data() + local_buffer.size());
    } else {
        *slice_out = charslice(NULL, NULL);
    }
    fixed_buffered_cond.reset();
    fixed_buffered_threshold = -1;
    return success;
}

void value_stream_t::pop_buffer(ssize_t amount) {
    assert(fixed_buffered_threshold == -1);
    assert(amount <= local_buffer_size);
    local_buffer.erase(local_buffer.begin(), local_buffer.begin() + amount);
    local_buffer_size -= amount;
}

charslice value_stream_t::buf_for_filling(ssize_t desired_size) {
    if (reader_list.empty()) {
        local_buffer.resize(local_buffer_size + desired_size);
        char *front = local_buffer.data() + local_buffer_size;
        return charslice(front, front + desired_size);
    } else {
        return reader_list.head()->buf;
    }
}

void value_stream_t::data_written(ssize_t amount) {
    if (reader_list.empty()) {
        assert(ssize_t(local_buffer.size() - local_buffer_size) >= amount);
        local_buffer_size += amount;
        if (fixed_buffered_threshold != -1 && local_buffer_size >= fixed_buffered_threshold) {
            fixed_buffered_cond.pulse(true);
        }
    } else {
        reader_node_t *node = reader_list.head();
        charslice *buf = &node->buf;
        assert(buf->end - buf->beg >= amount);
        buf->beg += amount;
        if (buf->beg == buf->end) {
            reader_list.remove(node);
            zombie_reader_list.push_back(node);
            node->var.pulse(true);
        }
    }
}








}  // namespace replication
