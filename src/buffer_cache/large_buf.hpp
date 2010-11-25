#ifndef __LARGE_BUF_HPP__
#define __LARGE_BUF_HPP__

#include "buffer_cache/buffer_cache.hpp"
#include "config/args.hpp"
#include "btree/node.hpp"
#include "conn_fsm.hpp"
#include "request.hpp"

#define NUM_SEGMENTS(total_size, seg_size) ( ( ((total_size)-1) / (seg_size) ) + 1 )

#define MAX_LARGE_BUF_SEGMENTS (NUM_SEGMENTS((MAX_VALUE_SIZE), (4*KILOBYTE)))

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
    block_magic_t magic;
    //uint32_t size; // TODO: Put the size here instead of in the btree value.
    uint16_t num_segments;
    uint16_t first_block_offset; // For prepend.
    block_id_t blocks[MAX_LARGE_BUF_SEGMENTS];

    static block_magic_t expected_magic;
};

struct large_buf_segment {
    block_magic_t magic;

    static block_magic_t expected_magic;
};

class large_buf_t
{
private:
    block_id_t index_block_id;
    buf_t *index_buf;
    uint32_t size; // XXX possibly unnecessary?
    access_t access;
    large_buf_available_callback_t *callback;

    transaction_t *transaction;
    size_t effective_segment_block_size;

    uint16_t num_acquired;
    buf_t *bufs[MAX_LARGE_BUF_SEGMENTS];

public: // XXX Should this be private?
    enum state_t {
        not_loaded,
        loading,
        loaded,
        deleted,
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
    
private:
    buf_t *allocate_segment(block_id_t *id);
};

// TODO: Rename this.
struct segment_block_available_callback_t :
    public block_available_callback_t
{
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

#endif // __LARGE_BUF_HPP__
