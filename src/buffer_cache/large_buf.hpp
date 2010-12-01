#ifndef __LARGE_BUF_HPP__
#define __LARGE_BUF_HPP__

#include "buffer_cache/buffer_cache.hpp"
#include "config/args.hpp"
#include "btree/node.hpp"
#include "conn_fsm.hpp"

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

// struct large_buf_ref is defined in buffer_cache/types.hpp.

struct large_buf_internal {
    block_magic_t magic;
    block_id_t kids[];

    static const block_magic_t expected_magic;
};

struct large_buf_leaf {
    block_magic_t magic;
    byte buf[];

    static const block_magic_t expected_magic;
};

struct buftree_t {
    buf_t *buf;
    std::vector<buftree_t *> children;
};

struct tree_available_callback_t;

class large_buf_t
{
private:
    large_buf_ref root_ref;
    buftree_t *root;
    access_t access;
    large_buf_available_callback_t *callback;

    transaction_t *transaction;
    size_t cache_block_size;

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

    void allocate(int64_t _size);
    //    void acquire(block_id_t _index_block, uint32_t _size, access_t _access, large_buf_available_callback_t *_callback);
    void acquire(large_buf_ref root_ref_, access_t access_, large_buf_available_callback_t *callback_);

    void append(int64_t extra_size);
    void prepend(int64_t extra_size);
    void fill_at(int64_t pos, const byte *data, int64_t fill_size);

    void unappend(int64_t extra_size);
    void unprepend(int64_t extra_size);

    uint16_t pos_to_ix(uint32_t pos);
    uint16_t pos_to_seg_pos(uint32_t pos);

    void mark_deleted();
    void release();

    const large_buf_ref& get_root_ref() const;
    //    block_id_t get_index_block_id();
    //    const large_buf_index *get_index();
    //    large_buf_index *get_index_write();

    // TODO look at calls to this function, make sure people don't use
    // uint16_t.
    int64_t get_num_segments();

    uint16_t segment_size(int ix);

    const byte *get_segment(int num, uint16_t *seg_size);
    byte *get_segment_write(int num, uint16_t *seg_size);

    void on_block_available(buf_t *buf);

    void index_acquired(buf_t *buf);
    void segment_acquired(buf_t *buf, uint16_t ix);
    void buftree_acquired(buftree_t *tr);

    friend struct acquire_buftree_fsm_t;

private:
    int64_t num_leaf_bytes() const;
    int64_t num_internal_kids() const;
    int64_t max_offset(int levels) const;
    int num_levels(int64_t last_offset) const;

    buftree_t *allocate_buftree(block_id_t *block_id, int64_t size, int64_t offset, int levels);
    buftree_t *acquire_buftree(block_id_t block_id, int64_t offset, int64_t size, int levels, tree_available_callback_t *cb);
    void acquire_slice(large_buf_ref root_ref_, access_t access_, int64_t slice_offset, int64_t slice_size, large_buf_available_callback_t *callback_);
    void fill_tree_at(buftree_t *tr, int64_t pos, const byte *data, int64_t fill_size, int levels);
    buftree_t *add_level(buftree_t *tr, block_id_t id, block_id_t *new_id);
    void allocate_part_of_tree(buftree_t *tr, int64_t offset, int64_t size, int levels);
    void walk_tree_structure(buftree_t *tr, int64_t offset, int64_t size, int levels, void (*bufdoer)(buf_t *), void (*buftree_cleaner)(buftree_t *));
    void delete_tree_structure(buftree_t *tr, int64_t offset, int64_t size, int levels);
    void only_mark_deleted_tree_structure(buftree_t *tr, int64_t offset, int64_t size, int levels);
    void release_tree_structure(buftree_t *tr, int64_t offset, int64_t size, int levels);
    buf_t *get_segment_buf(int ix, uint16_t *seg_size, uint16_t *seg_offset);
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
