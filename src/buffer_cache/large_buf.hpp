#ifndef __LARGE_BUF_HPP__
#define __LARGE_BUF_HPP__

#include "buffer_cache/buffer_cache.hpp"
#include "config/args.hpp"
// TODO: separate buffer_group_t from data_provider.hpp
#include "data_provider.hpp"

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
#ifndef NDEBUG
    int level;  // a positive number
#endif
    buf_t *buf;
    std::vector<buftree_t *> children;
};

struct tree_available_callback_t;

class large_buf_t {
private:
    large_buf_ref *root_ref;
    lbref_limit_t root_ref_limit;
    std::vector<buftree_t *> roots;
    access_t access;
    int num_to_acquire;
    large_buf_available_callback_t *callback;

    transaction_t *txn;
    block_size_t block_size() const { return txn->cache->get_block_size(); }

public:
#ifndef NDEBUG

    enum state_t {
        not_loaded,
        loading,
        loaded,
        deleted,
        released
    };
    state_t state;

    int64_t num_bufs;
#endif

    // TODO: Make appropriate private methods and friend classes and all that.

    explicit large_buf_t(transaction_t *txn);
    ~large_buf_t();

    // This is a COMPLETE HACK
    void HACK_root_ref(large_buf_ref *alternate_root_ref) {
        rassert(0 == memcmp(alternate_root_ref->block_ids, root_ref->block_ids, root_ref->refsize(block_size(), root_ref_limit) - sizeof(large_buf_ref)));
        root_ref = alternate_root_ref;
    }

    void allocate(int64_t _size, large_buf_ref *refout, lbref_limit_t ref_limit);
    void acquire(large_buf_ref *root_ref_, lbref_limit_t ref_limit_, access_t access_, large_buf_available_callback_t *callback_);
    void acquire_rhs(large_buf_ref *root_ref_, lbref_limit_t ref_limit_, access_t access_, large_buf_available_callback_t *callback_);
    void acquire_lhs(large_buf_ref *root_ref_, lbref_limit_t ref_limit_, access_t access_, large_buf_available_callback_t *callback_);
    void acquire_for_delete(large_buf_ref *root_ref_, lbref_limit_t ref_limit_, large_buf_available_callback_t *callback_);

    // refsize_adjustment_out parameter forces callers to recognize
    // that the size may change, so hopefully they'll update their
    // btree_value size field appropriately.
    void append(int64_t extra_size, int *refsize_adjustment_out);
    void prepend(int64_t extra_size, int *refsize_adjustment_out);
    void fill_at(int64_t pos, const void *data, int64_t fill_size);
    void read_at(int64_t pos, void *data_out, int64_t read_size);
    void bufs_at(int64_t pos, int64_t read_size, bool use_read_mode, buffer_group_t *bufs_out);

    void unappend(int64_t extra_size, int *refsize_adjustment_out);
    void unprepend(int64_t extra_size, int *refsize_adjustment_out);

    void mark_deleted();

    // TODO:  Stop being a bad programmer and start knowing what thread you're on.
    void ensure_thread() const { txn->ensure_thread(); }

    int64_t get_num_segments();

    uint16_t segment_size(int64_t ix);

    const byte *get_segment(int64_t num, uint16_t *seg_size);
    byte *get_segment_write(int64_t num, uint16_t *seg_size);

    void on_block_available(buf_t *buf);

    void index_acquired(buf_t *buf);
    void segment_acquired(buf_t *buf, uint16_t ix);
    void buftree_acquired(buftree_t *tr, int index);

    friend struct acquire_buftree_fsm_t;

    static int64_t bytes_per_leaf(block_size_t block_size);
    static int64_t compute_max_offset(block_size_t block_size, int levels);
    static int compute_num_sublevels(block_size_t block_size, int64_t end_offset, lbref_limit_t ref_limit);

    static int compute_large_buf_ref_num_inlined(block_size_t block_size, int64_t end_offset, lbref_limit_t ref_limit);

    int num_ref_inlined() const;

#ifndef NDEBUG
    bool root_ref_is(const large_buf_ref *ref) const {
        rassert(state == loading || state == loaded);
        return root_ref == ref;
    }
#endif

private:
    int64_t num_leaf_bytes() const;
    static int64_t kids_per_internal(block_size_t block_size);
    int64_t num_internal_kids() const;
    int64_t max_offset(int levels) const;
    // TODO: do we ever use anything for end_offset other than root_ref's?
    int num_sublevels(int64_t end_offset) const;

    buftree_t *allocate_buftree(int64_t size, int64_t offset, int levels, block_id_t *block_id);
    buftree_t *acquire_buftree(block_id_t block_id, int64_t offset, int64_t size, int levels, tree_available_callback_t *cb);
    void acquire_slice(large_buf_ref *root_ref_, lbref_limit_t ref_limit_, access_t access_, int64_t slice_offset, int64_t slice_size, large_buf_available_callback_t *callback_, bool should_load_leaves_ = true);

    void trees_bufs_at(const std::vector<buftree_t *>& trees, int sublevels, int64_t pos, int64_t read_size, bool use_read_mode, buffer_group_t *bufs_out);
    void tree_bufs_at(buftree_t *tr, int levels, int64_t pos, int64_t read_size, bool use_read_mode, buffer_group_t *bufs_out);

    void adds_level(block_id_t *ids
#ifndef NDEBUG
                    , int nextlevels
#endif
                    );
    void allocate_part_of_tree(buftree_t *tr, int64_t offset, int64_t size, int levels);
    void allocates_part_of_tree(std::vector<buftree_t *> *ptrs, block_id_t *block_ids, int64_t offset, int64_t size, int64_t sublevels);
    buftree_t *walk_tree_structure(buftree_t *tr, int64_t offset, int64_t size, int levels, void (*bufdoer)(large_buf_t *, buf_t *), buftree_t *(*buftree_cleaner)(buftree_t *));
    void walk_tree_structures(std::vector<buftree_t *> *trs, int64_t offset, int64_t size, int sublevels, void (*bufdoer)(large_buf_t *, buf_t *), buftree_t *(*buftree_cleaner)(buftree_t *));
    void delete_tree_structures(std::vector<buftree_t *> *trees, int64_t offset, int64_t size, int sublevels);
    void only_mark_deleted_tree_structures(std::vector<buftree_t *> *trees, int64_t offset, int64_t size, int sublevels);
    void release_tree_structures(std::vector<buftree_t *> *trs, int64_t offset, int64_t size, int sublevels);
    buf_t *get_segment_buf(int64_t ix, uint16_t *seg_size, uint16_t *seg_offset);
    void removes_level(block_id_t *ids, int copyees);
    int try_shifting(std::vector<buftree_t *> *trs, block_id_t *block_ids, int64_t offset, int64_t size, int64_t stepsize);

    void lv_release();

    DISABLE_COPYING(large_buf_t);
};

#endif // __LARGE_BUF_HPP__
