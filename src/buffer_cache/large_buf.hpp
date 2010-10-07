#ifndef __LARGE_BUF_HPP__
#define __LARGE_BUF_HPP__

#include "buffer_cache/buffer_cache.hpp"
#include "config/args.hpp"
#include "btree/node.hpp"
#include "conn_fsm.hpp"
#include "request.hpp"

#define NUM_SEGMENTS(total_size, seg_size) ( ( ((total_size)-1) / (seg_size) ) + 1 )

//#define MAX_LARGE_BUF_SEGMENTS ((((MAX_VALUE_SIZE) - 1) / (BTREE_BLOCK_SIZE)) + 1)
#define MAX_LARGE_BUF_SEGMENTS (NUM_SEGMENTS((MAX_VALUE_SIZE), (BTREE_USABLE_BLOCK_SIZE)))

class large_buf_t;

struct large_buf_available_callback_t :
    public intrusive_list_node_t<large_buf_available_callback_t> {
    virtual ~large_buf_available_callback_t() {}
    virtual void on_large_buf_available(large_buf_t *large_buf) = 0;
};

struct large_value_completed_callback {
    virtual void on_large_value_completed(bool success) = 0;
    virtual ~large_value_completed_callback() {}
};

// Must be smaller than a buf.
struct large_buf_index {
    //uint32_t size; // TODO: Put the size here instead of in the btree value.
    uint16_t num_segments;
    uint16_t first_block_offset; // For prepend.
    block_id_t blocks[MAX_LARGE_BUF_SEGMENTS];
};

class large_buf_t :
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, large_buf_t> {
private:
    block_id_t index_block_id;
    buf_t *index_buf;
    uint32_t size; // XXX possibly unnecessary?
    access_t access;
    large_buf_available_callback_t *callback;

    transaction_t *transaction;

    uint16_t num_acquired;
    buf_t *bufs[MAX_LARGE_BUF_SEGMENTS];

public: // XXX Should this be private?
    enum state_t {
        not_loaded,
        loading,
        loaded,
        released
    };
    state_t state;

// TODO: Take care of private methods and friend classes and all that.
public:
    large_buf_t(transaction_t *txn);
    ~large_buf_t();

    void allocate(uint32_t _size);
    void acquire(block_id_t _index_block, uint32_t _size, access_t _access, large_buf_available_callback_t *_callback);

    void append(uint32_t extra_size);
    void prepend(uint32_t extra_size);
    void fill_at(uint32_t pos, const byte *data, uint32_t fill_size);

    void unappend(uint32_t extra_size);
    void unprepend(uint32_t extra_size);

    uint16_t pos_to_ix(uint32_t pos);
    uint16_t pos_to_seg_pos(uint32_t pos);

    void mark_deleted();
    void release();

    block_id_t get_index_block_id();
    const large_buf_index *get_index();
    large_buf_index *get_index_write();
    uint16_t get_num_segments();

    uint16_t segment_size(int ix);

    const byte *get_segment(int num, uint16_t *seg_size);
    byte *get_segment_write(int num, uint16_t *seg_size);

    void on_block_available(buf_t *buf);

    void index_acquired(buf_t *buf);
    void segment_acquired(buf_t *buf, uint16_t ix);
};

// TODO: Rename this.
struct segment_block_available_callback_t : public block_available_callback_t,
                                            public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, segment_block_available_callback_t> {
    large_buf_t *owner;

    bool is_index_block;
    uint16_t ix;

    segment_block_available_callback_t(large_buf_t *owner)
        : owner(owner), is_index_block(true) {}
    
    segment_block_available_callback_t(large_buf_t *owner, uint16_t ix)
        : owner(owner), is_index_block(false), ix(ix) {}

    void on_block_available(buf_t *buf) {
        if (is_index_block) {
            owner->index_acquired(buf);
        } else {
            owner->segment_acquired(buf, ix);
        }
        delete this;
    }
};

// TODO: These two are sickeningly similar now. Merge them.

// TODO: Some of the code in here belongs in large_buf_t.

class fill_large_value_msg_t : public cpu_message_t,
                               public data_transferred_callback,
                               public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, fill_large_value_msg_t> {
private:
    typedef buffer_t<IO_BUFFER_SIZE> iobuf_t;

    enum {
        fill, // Fill a large value
        consume // Consume from the socket.
    } mode;

    iobuf_t *buf;

    bool completed;
    bool success;

    large_buf_t *large_value;
    request_callback_t *req;
    large_value_completed_callback *cb;
    uint32_t pos;
    uint32_t length;

public:
    fill_large_value_msg_t(large_buf_t *large_value, request_callback_t *req, large_value_completed_callback *cb, uint32_t pos, uint32_t length)
        : mode(fill), buf(NULL), completed(false), success(false), large_value(large_value), req(req), cb(cb), pos(pos), length(length) {
        //assert(pos + length < large_value->size); // XXX
    }

    fill_large_value_msg_t(request_callback_t *req, large_value_completed_callback *cb, uint32_t length)
        : mode(consume), buf(new iobuf_t()), completed(false), req(req), cb(cb), length(length) {}

    ~fill_large_value_msg_t() {
        if (buf) {
            assert(mode == consume);
            delete buf;
        }
    }

    void on_cpu_switch() {
        if (!completed) {
            step();
        } else {
            cb->on_large_value_completed(success);
            delete this;
        }
    }

    void on_data_transferred() {
        step();
    }

    void fill_complete(bool _success) {
        success = _success;
        completed = true;
        if (continue_on_cpu(return_cpu, this)) {
            call_later_on_this_cpu(this);
        }
    }
private:
    void step() {
        assert(!completed);
        switch (mode) {
            case fill:
                do_fill();
                break;
            case consume:
                do_consume();
                break;
        }
    }

    void do_consume() {
        if (length > 0) {
            uint32_t bytes_to_transfer = std::min((uint32_t) iobuf_t::size, length);
            length -= bytes_to_transfer;
            req->rh->fill_value((byte *) buf, bytes_to_transfer, this);
            return;
        }

        if (length == 0) {
            req->rh->fill_lv_msg = this;
            //req->rh->conn_fsm->dummy_sock_event();
            //if (continue_on_cpu(return_cpu, this)) on_cpu_switch();
        }
    }

    void do_fill() {
        //bool fill_success;
        //while (next_segment < large_value->get_num_segments()) {
        while (length > 0) {
            uint16_t ix = large_value->pos_to_ix(pos);
            uint16_t seg_pos = large_value->pos_to_seg_pos(pos);
            uint16_t seg_len;
            byte *buf = large_value->get_segment_write(ix, &seg_len);
            uint16_t bytes_to_transfer = std::min((uint32_t) seg_len - seg_pos, length);
            assert(seg_pos + bytes_to_transfer <= BTREE_USABLE_BLOCK_SIZE);
            pos += bytes_to_transfer;
            length -= bytes_to_transfer;
            req->rh->fill_value(buf + seg_pos, bytes_to_transfer, this);
            return;
            //fill_success = req->rh->fill_value(buf, seg_len, this);
            //if (!fill_success) break;
            //next_segment++;
        }

        //assert(next_segment <= large_value->get_num_segments());
        //if (next_segment == large_value->get_num_segments()) {
        if (length == 0) {
            req->rh->fill_lv_msg = this;
            //req->rh->conn_fsm->dummy_sock_event();
        }
    }
};


// TODO: Some of the code in here belongs in large_buf_t.
class write_large_value_msg_t : public cpu_message_t,
                                public data_transferred_callback,
                                public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, write_large_value_msg_t> {
private:
    large_buf_t *large_value;
    request_callback_t *req;
    unsigned int next_segment;
    large_value_completed_callback *cb;
public:
    enum {
        ready,
        reading,
        completed,
    } state;

public:
    write_large_value_msg_t(large_buf_t *large_value, request_callback_t *req, large_value_completed_callback *cb)
        : large_value(large_value), req(req), next_segment(0), cb(cb), state(ready) {
fprintf(stderr, "write_lv_msg constructed on cpu %d\n", get_cpu_id());
    }

    void on_cpu_switch() {
        switch (state) {
            case ready:
                req->on_fsm_ready();
                break;
            case reading:
                read_segments();
                break;
            case completed:
                cb->on_large_value_completed(true); // XXX
                delete this;
                break;
        }
    }

    void begin_write() {
        state = reading;
        on_cpu_switch();
    }

    void on_data_transferred() {
        next_segment++;
        read_segments();
    }
private:
    void read_segments() {
        assert(state == reading);
        uint16_t seg_len;
        const byte *buf;
        if (next_segment < large_value->get_num_segments()) {
            buf = large_value->get_segment(next_segment, &seg_len);
            req->rh->write_value(buf, seg_len, this);
            return;
        }

        assert(next_segment <= large_value->get_num_segments());
        if (next_segment == large_value->get_num_segments()) {
            req->on_fsm_ready();
            state = completed;
            
            // continue_on_cpu() returns true if we are already on that cpu
fprintf(stderr, "read_segments done; continuing on cpu %d (this cpu: %u)\n", return_cpu, get_cpu_id());
            //if (continue_on_cpu(return_cpu, this)) on_cpu_switch();
            if (continue_on_cpu(return_cpu, this)) call_later_on_this_cpu(this);
        }
    }
};

#endif // __LARGE_BUF_HPP__
