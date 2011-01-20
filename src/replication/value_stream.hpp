#ifndef __REPLICATION_VALUE_STREAM_HPP__
#define __REPLICATION_VALUE_STREAM_HPP__

#include <stddef.h>
#include <vector>

#include "utils2.hpp"
#include "containers/intrusive_list.hpp"
#include "concurrency/cond_var.hpp"

namespace replication {

// Right now this is super-dumb and WILL need to be
// reimplemented/enhanced.  Right now this is enhanceable.

// This will eventually be used by arch/linux/network.hpp

struct charslice {
    char *beg, *end;
    charslice(char *beg_, char *end_) : beg(beg_), end(end_) { }
    charslice() : beg(NULL), end(NULL) { }
};

struct const_charslice {
    const char *beg, *end;
    const_charslice(const char *beg_, const char *end_) : beg(beg_), end(end_) { }
    const_charslice() : beg(NULL), end(NULL) { }
};



// A value_stream_t is only designed to have one reader and one writer
// on the same thread.
//
// The reading end consists of a queue of buffers, waiting to be
// filled, with a growable "local_buffer" behind them, which gets
// filled if the queue is empty.
//
// You can use read_fixed_buffered which waits for the local_buffer to
// reach a certain size.  Use pop_buffer to consume data from the
// local_buffer.
//
// The writing end works by asking for a buf to fill, and then telling
// how much data has been written.  Thus the reader can supply a buf
// to be filled, and the writer can fill it directly.
//
// However, the writer cannot give up ownership of a buffer to the
// value_stream_t and have that passed on to the reader.


class value_stream_t {

    // A node in a list of nodes from which supply external buffers
    // that we would like to have written to.
    struct reader_node_t : public intrusive_list_node_t<reader_node_t> {
        // Gets triggered when the node is part of the zombie_reader_list.
        unicond_t<bool> var;
        charslice buf;
    };

public:
    typedef reader_node_t* read_token_t;

    value_stream_t();
    //     value_stream_t(const char *beg, const char *end);

    // Pushes an external buf onto the reading queue.
    read_token_t read_external(charslice buf);

    // Waits for the external buf (that corresponds to the token) to
    // be filled.  Returns true if it was.
    bool read_wait(read_token_t tok);

    // Waits for the local buf (which gets filled after all the
    // external bufs are filled) to be filled.  You must call
    // pop_buffer without any coroutine switching.
    bool read_fixed_buffered(ssize_t threshold, charslice *slice_out);
    void pop_buffer(ssize_t amount);

    // Gets a buf into which to write data.  You must call
    // data_written without any coroutine switching.
    charslice buf_for_filling(ssize_t desired_size);
    // Writes the data.
    void data_written(ssize_t amount);

private:
    // local_buffer_size <= local_buffer.size(), and it refers to the
    // written-to part of the buffer.
    ssize_t local_buffer_size;
    // local_buffer_size < fixed_buffered_threshold, and this is the
    // threshold at which fixed_buffered_cond gets triggered.
    ssize_t fixed_buffered_threshold;

    // The trigger for read_fixed_buffered.
    unicond_t<bool> fixed_buffered_cond;

    // A buffer into which we read characters.  This isn't performantly optimal.
    std::vector<char> local_buffer;

    intrusive_list_t<reader_node_t> reader_list;
    intrusive_list_t<reader_node_t> zombie_reader_list;

    DISABLE_COPYING(value_stream_t);
};

void write_charslice(value_stream_t& stream, const_charslice sl);


}  // namespace replication

#endif  // __REPLICATION_VALUE_STREAM_HPP__
