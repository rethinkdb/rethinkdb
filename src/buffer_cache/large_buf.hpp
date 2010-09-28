#ifndef __LARGE_BUF_HPP__
#define __LARGE_BUF_HPP__

#include "buffer_cache/buffer_cache.hpp"
#include "arch/resource.hpp"
#include "config/args.hpp"
#include "btree/node.hpp"
#include "conn_fsm.hpp"
#include "request.hpp"

#define NUM_SEGMENTS(total_size, segment_size) ( ( ((total_size)-1) / (segment_size) ) + 1 )

//#define MAX_LARGE_BUF_SEGMENTS ((((MAX_VALUE_SIZE) - 1) / (BTREE_BLOCK_SIZE)) + 1)
#define MAX_LARGE_BUF_SEGMENTS (NUM_SEGMENTS((MAX_VALUE_SIZE), (BTREE_BLOCK_SIZE)))

class large_buf_t;

struct large_buf_available_callback_t :
    public intrusive_list_node_t<large_buf_available_callback_t>
{
    virtual ~large_buf_available_callback_t() {}
    virtual void on_large_buf_available(large_buf_t *large_buf) = 0;
};

struct large_value_read_callback {
    virtual void on_large_value_read() = 0;
    virtual ~large_value_read_callback() {}
};

struct large_value_completed_callback {
    virtual void on_large_value_completed(bool success) = 0;
    virtual ~large_value_completed_callback() {}
};

// Must be smaller than a buf.
struct large_buf_index {
    // TODO: Put the size here instead of in the btree value.
    uint32_t size;
    uint16_t num_segments;
    uint16_t first_block_offset; // For prepend.
    block_id_t blocks[MAX_LARGE_BUF_SEGMENTS];
};

class large_buf_t :
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, large_buf_t>
{
private:
    block_id_t index_block; // TODO: Rename index_block_id.
    buf_t *index_buf;
    //const uint32_t size; // XXX possibly unnecessary?
    uint32_t size; // XXX possibly unnecessary?
    access_t access;
    large_buf_available_callback_t *callback;

    transaction_t *transaction;

    uint16_t num_acquired;
    buf_t *bufs[MAX_LARGE_BUF_SEGMENTS];

    // TODO: get_index()

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

    void append(uint32_t length);
    void prepend(uint32_t length);

    void mark_deleted();
    void release();
    void set_dirty();

    block_id_t get_index_block_id();
    large_buf_index *get_index();
    uint16_t get_num_segments();
    byte *get_segment(int num, uint16_t *segment_size);

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
    }
};

// TODO: These two are sickeningly similar now. Merge them.

// TODO: Some of the code in here belongs in large_buf_t.

class read_large_value_msg_t : public cpu_message_t,
                               public data_transferred_callback,
                               public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, read_large_value_msg_t>
{
public:
    bool completed;
    bool success;

private:
    large_buf_t *large_value;
    request_callback_t *req;
    large_value_completed_callback *cb;
    unsigned int next_segment;

public:
    read_large_value_msg_t(large_buf_t *large_value, request_callback_t *req, large_value_completed_callback *cb)
        : completed(false), success(false), large_value(large_value), req(req), cb(cb), next_segment(0) {}

    void on_cpu_switch() {
        if (completed) {
            cb->on_large_value_completed(success);
            delete this;
        } else {
            fill_segments();
        }
    }

    void on_data_transferred() {
        next_segment++;
        fill_segments();
    }
private:
    void fill_segments() {
        assert(!completed);
        uint16_t segment_length;
        byte *buf;
        //bool read_success;
        while (next_segment < large_value->get_num_segments()) {
            buf = large_value->get_segment(next_segment, &segment_length);
            req->rh->read_value(buf, segment_length, this);
            //break;
            return;
            //read_success = rh->read_value(buf, segment_length, this);
            //if (!read_success) break;
            //next_segment++;
        }

        assert(next_segment <= large_value->get_num_segments());
        if (next_segment == large_value->get_num_segments()) {
            req->rh->read_lv_msg = this;
            req->rh->conn_fsm->dummy_sock_event();
        }
    }
};


// TODO: Some of the code in here belongs in large_buf_t.
class write_large_value_msg_t : public cpu_message_t,
                                public data_transferred_callback,
                                public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, write_large_value_msg_t>
{
private:
    large_buf_t *large_value;
    request_callback_t *req;
    unsigned int next_segment;
    large_value_read_callback *read_cb;
    large_value_completed_callback *completed_cb;
public:
    enum {
        ready,
        reading,
        completed,
    } state;

public:
    write_large_value_msg_t(large_buf_t *large_value, request_callback_t *req, large_value_read_callback *read_cb, large_value_completed_callback *completed_cb)
        : large_value(large_value), req(req), next_segment(0), read_cb(read_cb), completed_cb(completed_cb), state(ready) {}

    void on_cpu_switch() {
        switch (state) {
            case ready:
                req->on_fsm_ready();
                break;
            case reading:
                read_segments();
                break;
            case completed:
                completed_cb->on_large_value_completed(true); // XXX
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
        uint16_t segment_length;
        byte *buf;
        if (next_segment < large_value->get_num_segments()) {
            buf = large_value->get_segment(next_segment, &segment_length);
            req->rh->write_value(buf, segment_length, this);
            return;
        }

        assert(next_segment <= large_value->get_num_segments());
        if (next_segment == large_value->get_num_segments()) {
            req->on_fsm_ready();
            state = completed;
            
            // continue_on_cpu() returns true if we are already on that cpu
            if (continue_on_cpu(return_cpu, this)) on_cpu_switch();
        }
    }
};

#endif // __LARGE_BUF_HPP__
