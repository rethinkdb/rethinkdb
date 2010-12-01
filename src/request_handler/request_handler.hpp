#ifndef __REQUEST_HANDLER_HPP__
#define __REQUEST_HANDLER_HPP__

#include "event.hpp"
#include "arch/arch.hpp"
#include "conn_fsm.hpp"
#include "store.hpp"

struct event_t;
class rh_data_provider_t;

class request_handler_t :
    private cpu_message_t
{

public:
    explicit request_handler_t()
        : data_provider(NULL), request_count(0) {}
    virtual ~request_handler_t() {}

    enum parse_result_t {
        op_malformed,
        op_partial_packet,
        op_req_quit,
        op_req_complex,
        op_req_parallelizable,
        op_req_send_now
    };

    virtual parse_result_t parse_request(event_t *event) = 0;

    void fill_value(byte *buf, unsigned int size, data_transferred_callback *cb);
    void write_value(const byte *buf, unsigned int size, data_transferred_callback *cb);

    void request_complete();
    
    conn_fsm_t *conn_fsm;

    // The data provider that's currently getting data from us and providing it to the key-value
    // store, if there is such a data provider at the moment
    rh_data_provider_t *data_provider;

private:
    int request_count;   // The number of et_request_completes to send to the conn_fsm
    void on_cpu_switch();
};

/* The buffered_data_provider_t provides data from an internal buffer. */

struct buffered_data_provider_t :
    public data_provider_t
{
    /* We copy the data we are given, not refer to it. */
    buffered_data_provider_t(void *buf, size_t length)
        : buffer(malloc(length)), length(length)
    {
        memcpy(buffer, buf, length);
    }
    ~buffered_data_provider_t() {
        free(buffer);
    }
    size_t get_size() {
        return length;
    }
    void get_value(buffer_group_t *bg, data_provider_t::done_callback_t *cb) {
        if (bg) {
            int pos = 0;
            for (int i = 0; i < (signed)bg->buffers.size(); i++) {
                memcpy(bg->buffers[i].data, (char*)buffer + pos, bg->buffers[i].size);
                pos += bg->buffers[i].size;
            }
            assert(pos == (signed)length);
        }
        cb->have_provided_value();
    }
private:
    void *buffer;
    size_t length;
};

/* The rh_data_provider_t is a data_provider_t that gets its data by reading from a
request handler. In general it is used for big values that would be expensive to buffer
in memory. The disadvantage is that it requires going back and forth between the core
where the cache is and the core where the request handler is. */

class rh_data_provider_t :
    public data_provider_t,
    public cpu_message_t,
    public home_cpu_mixin_t,
    public data_transferred_callback
{
private:
    typedef buffer_t<IO_BUFFER_SIZE> iobuf_t;

    enum {
        unused,   // mode = unused before either get_value() or consume() has been called
        fill,   // Fill some buffers with the data from the socket
        consume   // Discard the data from the socket
    } mode;

    int requestor_cpu;   // CPU that callback must be called on
    data_provider_t::done_callback_t *cb;

    bool completed;
    bool success;

    request_handler_t *rh;   // Request handler for us to read from
    size_t length;   // Our length

    buffer_group_t *bg;   // Buffers to read into in fill mode
    int bg_seg, bg_seg_pos;   // Segment we're currently reading into and how many bytes are in it

    char *dummy_buf;   // Dummy buf used in consume mode
    size_t bytes_consumed;

public:
    rh_data_provider_t(request_handler_t *rh, uint32_t length)
        : mode(unused), rh(rh), length(length)
    {
    }
    
    ~rh_data_provider_t() {
        assert(mode != unused);
        if (dummy_buf) {
            assert(mode == consume);
            delete[] dummy_buf;
        }
    }
    
    size_t get_size() {
        return length;
    }
    
    void get_value(buffer_group_t *b, data_provider_t::done_callback_t *c) {
        assert(mode == unused);
        if (b) {
            mode = fill;
            dummy_buf = NULL;
            bg = b;
            bg_seg = bg_seg_pos = 0;
        } else {
            mode = consume;
            dummy_buf = new char[IO_BUFFER_SIZE];
            bytes_consumed = 0;
        }
        requestor_cpu = get_cpu_id();
        completed = false;
        cb = c;
        
        // call_later_on_this_cpu rather than on_cpu_switch() because if we call on_cpu_switch()
        // immediately then we might try to read from the conn_fsm before the request handler has
        // even returned from parse_request(), and the conn_fsm will be confused.
        if (continue_on_cpu(home_cpu, this)) call_later_on_this_cpu(this);
    }

    void on_cpu_switch() {
        if (!completed) {
            assert_cpu();
            step();
        } else {
            assert(get_cpu_id() == requestor_cpu);
            if (success) cb->have_provided_value();
            else cb->have_failed();
        }
    }

    void on_data_transferred() {
        step();
    }

    void fill_complete(bool _success) {
        success = _success;
        completed = true;
        if (continue_on_cpu(requestor_cpu, this)) {
            call_later_on_this_cpu(this);
        }
    }
private:
    void step() {
        assert(!completed);
        switch (mode) {
            case unused:
                fail("WTF");
            case fill:
                do_fill();
                break;
            case consume:
                do_consume();
                break;
        }
    }

    void do_consume() {
        if (bytes_consumed < length) {
            size_t bytes_to_transfer = std::min(IO_BUFFER_SIZE, (long int)length - (long int)bytes_consumed);
            bytes_consumed += bytes_to_transfer;
            rh->fill_value(dummy_buf, bytes_to_transfer, this);
            return;
        }

        if (bytes_consumed == length) {
            rh->data_provider = this;
        }
    }

    void do_fill() {
        if (bg_seg < (signed)bg->buffers.size()) {
            buffer_group_t::buffer_t *bg_buf = &bg->buffers[bg_seg];
            uint16_t start_pos = bg_seg_pos;
            uint16_t bytes_to_transfer = std::min(bg_buf->size - start_pos, (ssize_t)length);
            bg_seg_pos += bytes_to_transfer;
            if (bg_seg_pos == (signed)bg_buf->size) {
                bg_seg++;
                bg_seg_pos = 0;
            }
            rh->fill_value((char*)bg_buf->data + start_pos, bytes_to_transfer, this);
            return;
        } else {
            rh->data_provider = this;
        }
    }
};

#endif // __REQUEST_HANDLER_HPP__
