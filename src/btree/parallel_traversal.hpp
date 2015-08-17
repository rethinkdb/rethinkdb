// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BTREE_PARALLEL_TRAVERSAL_HPP_
#define BTREE_PARALLEL_TRAVERSAL_HPP_

#include <utility>
#include <vector>

#include "concurrency/access.hpp"
#include "btree/traversal_progress.hpp"
#include "btree/types.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/interruptor.hpp"
#include "concurrency/signal.hpp"
#include "containers/scoped.hpp"

class buf_lock_t;
class buf_parent_t;
struct btree_superblock_t;
class traversal_state_t;
class parent_releaser_t;
class btree_slice_t;
struct btree_key_t;
struct internal_node_t;
class superblock_t;

struct lock_in_line_callback_t {
public:
    lock_in_line_callback_t() { }
    virtual void on_in_line() = 0;
protected:
    virtual ~lock_in_line_callback_t() { }
private:
    DISABLE_COPYING(lock_in_line_callback_t);
};


// HEY: Make this an abstract class, have two separate implementation
// classes for the "forced block id" case and the internal node case.
// (Hint: I don't really care.)
class ranged_block_ids_t : public single_threaded_countable_t<ranged_block_ids_t> {
public:
    ranged_block_ids_t(block_size_t bs, const internal_node_t *node,
                       const btree_key_t *left_exclusive_or_null,
                       const btree_key_t *right_inclusive_or_null,
                       int _level)
        : node_(bs.value()),
          left_exclusive_or_null_(left_exclusive_or_null),
          right_inclusive_or_null_(right_inclusive_or_null),
          level(_level)
    {
        memcpy(node_.get(), node, bs.value());
    }
    ranged_block_ids_t(block_id_t forced_block_id,
                       const btree_key_t *left_exclusive_or_null,
                       const btree_key_t *right_inclusive_or_null,
                       int _level)
        : node_(),
          forced_block_id_(forced_block_id),
          left_exclusive_or_null_(left_exclusive_or_null),
          right_inclusive_or_null_(right_inclusive_or_null),
          level(_level)
    { }

    int num_block_ids() const;
    void get_block_id_and_bounding_interval(int index,
                                            block_id_t *block_id_out,
                                            const btree_key_t **left_excl_bound_out,
                                            const btree_key_t **right_incl_bound_out) const;

    int get_level();

private:
    scoped_malloc_t<internal_node_t> node_;
    block_id_t forced_block_id_;
    const btree_key_t *left_exclusive_or_null_;
    const btree_key_t *right_inclusive_or_null_;
    int level;

    DISABLE_COPYING(ranged_block_ids_t);
};

class interesting_children_callback_t : public lock_in_line_callback_t {
public:
    // Call these function in filter_interesting_children.
    void receive_interesting_child(int child_index);
    void no_more_interesting_children();

    interesting_children_callback_t(
            traversal_state_t *_state,
            parent_releaser_t *_releaser,
            int _level,
            const counted_t<ranged_block_ids_t> &_ids_source)
        : state(_state), releaser(_releaser),
          level(_level), acquisition_countdown(1), ids_source(_ids_source) { }

private:
    void on_in_line();
    void decr_acquisition_countdown();

    traversal_state_t *state;
    parent_releaser_t *releaser;
    int level;
    int acquisition_countdown;
    counted_t<ranged_block_ids_t> ids_source;

    DISABLE_COPYING(interesting_children_callback_t);
};

class parallel_traversal_progress_t;

struct btree_traversal_helper_t {
    btree_traversal_helper_t()
        : progress(NULL)
    { }

    //Before any of these other functions are called the helper gets a chance
    //to look at the stat block and possibly record some values of interest
    //Notice the values in the stat block do not reflect changes which are
    //still traversing the tree at the time this is called. Also notice that
    //this value may be null if the stat block has not been allocated yet and
    //this traversal is read only (which prohibits us from allocating it)
    virtual void read_stat_block(buf_lock_t *) { }

    // This is free to call mark_deleted.
    virtual void process_a_leaf(buf_lock_t *leaf_node_buf,
                                const btree_key_t *left_exclusive_or_null,
                                const btree_key_t *right_inclusive_or_null,
                                signal_t *interruptor,
                                int *population_change_out) THROWS_ONLY(interrupted_exc_t) = 0;

    virtual void postprocess_internal_node(buf_lock_t *internal_node_buf) = 0;

    virtual void filter_interesting_children(
            buf_parent_t parent,
            ranged_block_ids_t *ids_source,
            interesting_children_callback_t *cb) = 0;

    virtual access_t btree_superblock_mode() = 0;
    virtual access_t btree_node_mode() = 0;


    virtual ~btree_traversal_helper_t() { }

    parallel_traversal_progress_t *progress;
};

void btree_parallel_traversal(
        superblock_t *superblock,
        btree_traversal_helper_t *helper,
        signal_t *interruptor,
        release_superblock_t release_superblock = release_superblock_t::RELEASE)
        THROWS_ONLY(interrupted_exc_t);


class parallel_traversal_progress_t : public traversal_progress_t {
public:
    parallel_traversal_progress_t() : height(-1) { }

    enum action_t {
        LEARN,
        ACQUIRE,
        RELEASE
    };

    enum node_type_t {
        UNKNOWN,
        INTERNAL,
        LEAF
    };

    void inform(int level, action_t, node_type_t);

    progress_completion_fraction_t guess_completion() const;

private:
    std::vector<int> learned; //How many nodes at each level we believe exist
    std::vector<int> acquired; //How many nodes at each level we've acquired
    std::vector<int> released; //How many nodes at each level we've released

    int height; //The height we've learned the tree has. Or -1 if we're still unsure;

    DISABLE_COPYING(parallel_traversal_progress_t);
};


#endif  // BTREE_PARALLEL_TRAVERSAL_HPP_
