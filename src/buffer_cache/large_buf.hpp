#ifndef __LARGE_BUF_HPP__
#define __LARGE_BUF_HPP__

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "buffer_cache/buffer_cache.hpp"
#include "buffer_cache/transactor.hpp"
#include "config/args.hpp"

#include "containers/buffer_group.hpp"

class large_buf_t;

// TODO: why does this inherit from intrusive_list_node_t?
struct large_buf_available_callback_t :
    public intrusive_list_node_t<large_buf_available_callback_t> {
    virtual void on_large_buf_available(large_buf_t *large_buf) = 0;
protected:
    virtual ~large_buf_available_callback_t() {}
};

// struct large_buf_ref is defined in buffer_cache/types.hpp.

// Disk format struct.
struct large_buf_internal {
    block_magic_t magic;
    block_id_t kids[];

    static const block_magic_t expected_magic;
};

// Disk format struct.
struct large_buf_leaf {
    block_magic_t magic;
    char buf[];

    static const block_magic_t expected_magic;
};

struct buftree_t;

struct tree_available_callback_t {
    // responsible for calling delete this
    virtual void on_available(buftree_t *tr, int index) = 0;
    virtual ~tree_available_callback_t() {}
};



class large_buf_t : public tree_available_callback_t {
public:
    // The large_buf_ref (which resides in a btree_modify_oper_t,
    // typically) that this large_buf_t is attached to.  This is not
    // _truly_ const, see HACK_root_ref.
    large_buf_ref *const root_ref;

private:
    // An upper bound of the size of the root_ref.  For example, btree
    // leaf nodes probably still give a ref_limit of
    // MAX_IN_NODE_VALUE_SIZE, which is probably still 250 bytes (and
    // that doesn't have to be a multiple of sizeof(block_id_t))
    lbref_limit_t const root_ref_limit;

    // Tells what rights we have to the bufs we're loading or have
    // loaded.
    access_t const access;

    // The transaction in which this large_buf_t's lifetime exists.
    boost::shared_ptr<transactor_t> const txor;

    // When we allocate or acquire a large buffer, we create a tree of
    // butrees, parallel to the on-disk tree, where each node holds
    // the corresponding buf_t object.  Completely unloaded subtrees
    // will either have a NULL entry or no corresponding entry.  (The
    // same goes for the vector in buftree_t::children, which works
    // the same way.)
    std::vector<buftree_t *> roots;

    // Sometimes we are busy loading subtrees.  In this "loading"
    // state (see state below), this tells how many subtrees we have
    // left to load, and when it hits 0, the large buf is available.
    int num_to_acquire;

    // Called (and reset to NULL) once the large buf is available.
    large_buf_available_callback_t *callback;

    // A large_buf_t does not have a precise idea of what subset of it
    // is acquired.  You need to navigate roots and its descendants
    // _completely_ to see which nodes are acquired.  This may change
    // shortly.

public:
#ifndef NDEBUG
    enum state_t {
        not_loaded,
        loading,
        loaded,
        deleted,
    };
    state_t state;

    int64_t num_bufs;
#endif

    explicit large_buf_t(const boost::shared_ptr<transactor_t>& txor, large_buf_ref *root_ref, lbref_limit_t ref_limit, access_t access);
    ~large_buf_t();

    // This is a COMPLETE HACK
    void HACK_root_ref(large_buf_ref *alternate_root_ref) {
        rassert(0 == memcmp(alternate_root_ref->block_ids, root_ref->block_ids, root_ref->refsize(block_size(), root_ref_limit) - sizeof(large_buf_ref)));
        const_cast<large_buf_ref *&>(root_ref) = alternate_root_ref;
    }

    void allocate(int64_t _size);

    void acquire_slice(int64_t slice_offset, int64_t slice_size, large_buf_available_callback_t *callback);
    void acquire_for_delete(large_buf_available_callback_t *callback);
    void acquire_for_unprepend(int64_t extra_size, large_buf_available_callback_t *callback);

    void co_enqueue(const boost::shared_ptr<transactor_t>& txor, large_buf_ref *root_ref, lbref_limit_t ref_limit, int64_t amount_to_dequeue, void *buf, int64_t n);



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
    void ensure_thread(const thread_saver_t& saver) const { (*txor)->ensure_thread(saver); }

    void on_block_available(buf_t *buf);

    void index_acquired(buf_t *buf);
    void segment_acquired(buf_t *buf, uint16_t ix);
    void on_available(buftree_t *tr, int index);

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

    void do_acquire_slice(int64_t slice_offset, int64_t slice_size, large_buf_available_callback_t *callback_, bool should_load_leaves_);

    void trees_bufs_at(const std::vector<buftree_t *>& trees, int sublevels, int64_t pos, int64_t read_size, bool use_read_mode, buffer_group_t *bufs_out);
    void tree_bufs_at(buftree_t *tr, int levels, int64_t pos, int64_t read_size, bool use_read_mode, buffer_group_t *bufs_out);

    void adds_level(block_id_t *ids, int num_roots
#ifndef NDEBUG
                    , int nextlevels
#endif
                    );
    void allocate_part_of_tree(buftree_t *tr, int64_t offset, int64_t size, int levels);
    void allocates_part_of_tree(std::vector<buftree_t *> *ptrs, block_id_t *block_ids, int64_t offset, int64_t size, int64_t sublevels);
    buftree_t *walk_tree_structure(buftree_t *tr, int64_t offset, int64_t size, int levels, void (*bufdoer)(large_buf_t *, buf_t *, bool, bool), buftree_t *(*buftree_cleaner)(buftree_t *, bool, bool));
    void walk_tree_structures(std::vector<buftree_t *> *trs, int64_t offset, int64_t size, int sublevels, void (*bufdoer)(large_buf_t *, buf_t *, bool, bool), buftree_t *(*buftree_cleaner)(buftree_t *, bool, bool));
    void delete_tree_structures(std::vector<buftree_t *> *trees, int64_t offset, int64_t size, int sublevels);
    void only_mark_deleted_tree_structures(std::vector<buftree_t *> *trees, int64_t offset, int64_t size, int sublevels);
    void release_tree_structures(std::vector<buftree_t *> *trs, int64_t offset, int64_t size, int sublevels);
    void removes_level(block_id_t *ids, int copyees);
    int try_shifting(std::vector<buftree_t *> *trs, block_id_t *block_ids, int64_t offset, int64_t size, int64_t stepsize);

    block_size_t block_size() const { return (*txor)->cache->get_block_size(); }

    void lv_release();

    DISABLE_COPYING(large_buf_t);
};

#endif // __LARGE_BUF_HPP__
