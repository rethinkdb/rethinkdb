#include "value_stream.hpp"

#include <string.h>

namespace replication {

value_stream_t::value_stream_t()
    : local_buffer_size(0), fixed_buffered_threshold(-1) { }

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
    rassert(fixed_buffered_threshold == -1);
    if (local_buffer_size >= threshold) {
        *slice_out = charslice(local_buffer.data(), local_buffer.data() + local_buffer.size());
        return true;
    }
    fixed_buffered_threshold = threshold;
    bool success = fixed_buffered_cond.wait();
    if (success) {
        side_buffer.resize(threshold);
        std::copy(local_buffer.begin(), local_buffer.begin() + threshold, side_buffer.begin());
        *slice_out = charslice(side_buffer.data(), side_buffer.data() + side_buffer.size());
    } else {
        *slice_out = charslice(NULL, NULL);
    }
    fixed_buffered_cond.reset();
    fixed_buffered_threshold = -1;
    return success;
}

void value_stream_t::pop_buffer(ssize_t amount) {
    rassert(fixed_buffered_threshold == -1);
    rassert(amount <= local_buffer_size);
    local_buffer.erase(local_buffer.begin(), local_buffer.begin() + amount);
    side_buffer.clear();
    local_buffer_size -= amount;
}

charslice value_stream_t::buf_for_filling(ssize_t desired_size) {
    if (reader_list.empty()) {
        write_side_buffer.resize(desired_size);
        return charslice(write_side_buffer.data(), write_side_buffer.data() + desired_size);
    } else {
        return reader_list.head()->buf;
    }
}

void value_stream_t::data_written(ssize_t amount) {
    if (reader_list.empty()) {
        rassert(ssize_t(write_side_buffer.size()) >= amount);
        local_buffer.insert(local_buffer.end(), write_side_buffer.begin(), write_side_buffer.end());
        write_side_buffer.clear();
        if (fixed_buffered_threshold != -1 && local_buffer_size >= fixed_buffered_threshold) {
            fixed_buffered_cond.pulse(true);
        }
    } else {
        reader_node_t *node = reader_list.head();
        charslice *buf = &node->buf;
        rassert(buf->end - buf->beg >= amount);
        buf->beg += amount;
        if (buf->beg == buf->end) {
            reader_list.remove(node);
            zombie_reader_list.push_back(node);
            node->var.pulse(true);
        }
    }
}

void write_charslice(value_stream_t& stream, const_charslice r) {
    while (r.beg < r.end) {
        charslice w = stream.buf_for_filling(r.end - r.beg);
        ssize_t n = std::min(w.end - w.beg, r.end - r.beg);
        memcpy(w.beg, r.beg, n);
        stream.data_written(n);
        r.beg += n;
    }
}






}  // namespace replication
