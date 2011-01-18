#ifndef __REPLICATION_VALUE_STREAM_HPP__
#define __REPLICATION_VALUE_STREAM_HPP__

#include <stddef.h>
#include <vector>

#include "utils2.hpp"

namespace replication {

// Right now this is super-dumb and WILL need to be
// reimplemented/enhanced.  Right now this is enhanceable.

// This will eventually be used by arch/linux/network.hpp

struct value_stream_read_external_callback_t {
    virtual void on_read_external() = 0;
    virtual void on_read_close(size_t num_remaining_bytes_dumped) = 0;
    virtual ~value_stream_read_external_callback_t() { }
};

class cookie_t {
public:
    friend class value_stream_t;
protected:
    cookie_t() : acknowledged(false) { }

private:
    bool acknowledged;
    DISABLE_COPYING(cookie_t);
};

struct value_stream_read_fixed_buffered_callback_t {
    virtual void on_read_fixed_buffered(const char *data, cookie_t *cookie) = 0;
    virtual void on_read_close() = 0;
    virtual ~value_stream_read_fixed_buffered_callback_t() { }
};



class write_cookie_t : private cookie_t {
public:
    friend class value_stream_t;
private:
    write_cookie_t(size_t allocated) : cookie_t(), space_allocated(allocated) { }
    size_t space_allocated;
    DISABLE_COPYING(write_cookie_t);
};

struct value_stream_writing_action_t {
    virtual void write_and_inform(char *buf, size_t size, write_cookie_t *cookie) = 0;
    virtual ~value_stream_writing_action_t() { }
};



class value_stream_t {
public:
    value_stream_t();
    value_stream_t(const char *beg, const char *end);

    // For reading out of the stream...

    void read_external(char *buf, size_t size, value_stream_read_external_callback_t *cb);

    // cb _must_ call acknowledge_fixed before returning.
    void read_fixed_buffered(size_t size, value_stream_read_fixed_buffered_callback_t *cb);
    void acknowledge_fixed(cookie_t *cookie);

    // For writing into the stream...

    // action _must_ call inform_space_written or inform_closed before returning.
    void open_space_for_writing(size_t size, value_stream_writing_action_t *action);
    void inform_space_written(size_t amount_written, write_cookie_t *cookie);
    void inform_closed(write_cookie_t *cookie);

private:
    void try_report_external();
    void try_report_fixed();
    void try_report_new_data();

    enum {
        read_mode_none,
        read_mode_external,
        read_mode_fixed,
    } read_mode;

    bool closed;
    std::vector<char> buffer;
    char *external_buf;
    size_t external_buf_size;
    value_stream_read_external_callback_t *external_cb;
    value_stream_read_fixed_buffered_callback_t *fixed_cb;
    size_t desired_fixed_size;

    DISABLE_COPYING(value_stream_t);
};

}  // namespace replication

#endif  // __REPLICATION_VALUE_STREAM_HPP__
