#include "btree/rget.hpp"
#include "btree/rget_internal.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "arch/linux/coroutines.hpp"
#include "buffer_cache/buf_lock.hpp"
#include <boost/shared_ptr.hpp>

/*
 * Possible rget designs:
 * 1. Depth-first search through the B-tree, then iterating through leaves (and maintaining a stack
 *    with some data to be able to backtrack).
 * 2. Breadth-first search, by maintaining a queue of blocks and releasing the lock on the block
 *    when we extracted the IDs of its children.
 * 3. Hybrid of 1 and 2: maintain a deque and use it as a queue, like in 2, thus releasing the locks
 *    for the top of the B-tree quickly, however when the deque reaches some size, start using it as
 *    a stack in depth-first search (but not quite in a usual way; see the note below).
 *
 * Problems of 1: we have to lock the whole path from the root down to the current node, which works
 * fine with small rgets (when max_results is low), but causes unnecessary amounts of locking (and
 * probably copy-on-writes, once we implement them).
 *
 * Problem of 2: while it doesn't hold unnecessary locks to the top (close to root) levels of the
 * B-tree, it may try to lock too much at once if the rget query effectively spans too many blocks
 * (e.g. when we try to rget the whole database).
 *
 * Hybrid approach seems to be the best choice here, because we hold the locks as low (far from the
 * root) in the tree as possible, while minimizing their number by doing a depth-first search from
 * some level.
 *
 * Note (on hybrid implementation):
 * If the deque approach is used, it is important to note that all the nodes in the current level
 * are in a reversed order when we decide to switch to popping from the stack:
 *
 *      P       Lets assume that we have node P in our deque, P is locked: [P]
 *    /   \     We remove P from the deque, lock its children, and push them back to the deque: [A, B]
 *   A     B    Now we can release the P lock.
 *  /|\   /.\   Next, we remove A, lock its children, and push them back to the deque: [B, c, d, e]
 * c d e .....  We release the A lock.
 * ..... ...... 
 *              At this point we decide that we need to do a depth-first search (to limit the number
 * of locked nodes), and start to use deque as a stack. However since we want
 * an inorder traversal, not the reversed inorder, we can't pop from the end of
 * the deque, we need to pop node 'c' instead of 'e', then (once we're done
 * with its depth-first search) do 'd', and then do 'e'.
 *
 * There are several possible approaches, one of them is putting markers in the deque in
 * between the nodes of different B-tree levels, another (probably a better one) is maintaining a
 * deque of deques, where the inner deques contain the nodes from the current B-tree level.
 *
 *
 * Initially the DFS design is implemented, since it's the simplest solution, which also is a good
 * fit for small rgets (the most popular use-case).
 */

struct leaf_iterator_t : public ordered_data_iterator<key_with_data_provider_t> {
    leaf_iterator_t(const leaf_node_t *leaf, int index, buf_lock_t *lock, boost::shared_ptr<transactor_t> transactor) :
        leaf(leaf), index(index), lock(lock), transactor(transactor) {

        rassert(leaf != NULL);
        rassert(lock != NULL);
    }

    boost::optional<key_with_data_provider_t> next() {
        rassert(index >= 0);
        if (index >= leaf->npairs)
            return boost::none;

        return boost::make_optional(pair_to_key_with_data_provider(leaf::get_pair_by_index(leaf, index++)));
    }
    void prefetch() { /* this space intentionally left blank */ }
    virtual ~leaf_iterator_t() {
        delete lock;
    }
private:

    key_with_data_provider_t pair_to_key_with_data_provider(const btree_leaf_pair* pair) {
        rget_value_provider_t *data_provider = rget_value_provider_t::create_provider(pair->value(), transactor); 
        return key_with_data_provider_t(key_to_str(&pair->key), pair->value()->mcflags(),
            boost::shared_ptr<data_provider_t>(data_provider));
    }

    const leaf_node_t *leaf;
    int index;
    buf_lock_t *lock;
    boost::shared_ptr<transactor_t> transactor;
};

/* slice_leaves_iterator_t finds the first leaf that contains the given key (or
 * the next key, if left_open is true). It returns that a leaf iterator (which
 * also contains the lock), however it doesn't release the leaf lock itself.
 *
 * slice_leaves_iterator_t maintains internal state by locking some internal
 * nodes and unlocking them as iteration progresses.
 */
class slice_leaves_iterator_t : public ordered_data_iterator<leaf_iterator_t*> {
    struct internal_node_state {
        internal_node_state(const internal_node_t *node, int index, buf_lock_t *lock)
            : node(node), index(index), lock(lock) { }

        const internal_node_t *node;
        int index;
        buf_lock_t *lock;
    };
public:
    slice_leaves_iterator_t(boost::shared_ptr<transactor_t> transactor, btree_slice_t *slice, store_key_t *start, store_key_t *end, bool left_open, bool right_open) : 
        transactor(transactor), slice(slice), start(start), end(end), left_open(left_open), right_open(right_open), started(false), nevermore(false) { }

    boost::optional<leaf_iterator_t*> next() {
        if (nevermore)
            return boost::none;

        boost::optional<leaf_iterator_t*> leaf = !started ? get_first_leaf() : get_next_leaf();
        if (!leaf)
            done();
        return leaf;
    }
    void prefetch() {
    }
    virtual ~slice_leaves_iterator_t() {
        done();
    }
private:
    void done() {
        while (!traversal_state.empty()) {
            delete traversal_state.back().lock;
            traversal_state.pop_back();
        }
        nevermore = true;
    }

    boost::optional<leaf_iterator_t*> get_first_leaf() {
        on_thread_t mover(slice->home_thread); // Move to the slice's thread.

        started = true;
        buf_lock_t *buf_lock = new buf_lock_t(*transactor, block_id_t(SUPERBLOCK_ID), rwi_read);
        block_id_t root_id = ptr_cast<btree_superblock_t>(buf_lock->buf()->get_data_read())->root_block;
        rassert(root_id != SUPERBLOCK_ID);

        if (root_id == NULL_BLOCK_ID) {
            delete buf_lock;
            return boost::none;
        }

        {
            buf_lock_t tmp(*transactor, root_id, rwi_read);
            buf_lock->swap(tmp);
        }
        // read root node
        const node_t *node = ptr_cast<node_t>(buf_lock->buf()->get_data_read());
        rassert(node != NULL);

        while (node::is_internal(node)) {
            const internal_node_t *i_node = ptr_cast<internal_node_t>(node);

            // push the i_node onto the traversal_state stack
            int index = internal_node::impl::get_offset_index(i_node, start);
            rassert(index >= 0);
            if (index >= i_node->npairs) {
                // this subtree has all the keys smaller than 'start', move to the leaf in the next subtree
                delete buf_lock;
                return get_next_leaf();
            }
            traversal_state.push_back(internal_node_state(i_node, index, buf_lock));

            block_id_t child_id = get_child_id(i_node, index);

            buf_lock = new buf_lock_t(*transactor, child_id, rwi_read);
            node = ptr_cast<node_t>(buf_lock->buf()->get_data_read());
        }

        rassert(node != NULL);
        rassert(node::is_leaf(node));
        rassert(buf_lock != NULL);

        const leaf_node_t *l_node = ptr_cast<leaf_node_t>(node);
        int index = leaf::impl::get_offset_index(l_node, start);

        if (index < l_node->npairs) {
            return boost::make_optional(new leaf_iterator_t(l_node, index, buf_lock, transactor));
        } else {
            // there's nothing more in this leaf, we might as well move to the next leaf
            delete buf_lock;
            return get_next_leaf();
        }
    }

    boost::optional<leaf_iterator_t*> get_next_leaf() {
        while (traversal_state.size() > 0) {
            internal_node_state state = traversal_state.back();
            traversal_state.pop_back();

            rassert(state.index >= 0 && state.index < state.node->npairs);
            ++state.index;
            if (state.index < state.node->npairs) {
                traversal_state.push_back(state);

                // get next child and find its leftmost leaf
                block_id_t child_id = get_child_id(state.node, state.index);
                return get_leftmost_leaf(child_id);
            } else {
                delete state.lock;  // this also releases state.node
            }
        }
        return boost::none;
    }

    boost::optional<leaf_iterator_t*> get_leftmost_leaf(block_id_t node_id) {
        on_thread_t mover(slice->home_thread); // Move to the slice's thread.

        buf_lock_t *buf_lock = new buf_lock_t(*transactor, node_id, rwi_read);
        const node_t *node = ptr_cast<node_t>(buf_lock->buf()->get_data_read());

        while (node::is_internal(node)) {
            const internal_node_t *i_node = ptr_cast<internal_node_t>(node);
            rassert(i_node->npairs > 0);

            // push the i_node onto the traversal_state stack
            const int leftmost_child_index = 0;
            traversal_state.push_back(internal_node_state(i_node, leftmost_child_index, buf_lock));

            block_id_t child_id = get_child_id(i_node, leftmost_child_index);

            buf_lock = new buf_lock_t(*transactor, child_id, rwi_read);
            node = ptr_cast<node_t>(buf_lock->buf()->get_data_read());
        }
        rassert(node != NULL);
        rassert(node::is_leaf(node));
        rassert(buf_lock != NULL);

        const leaf_node_t *l_node = ptr_cast<leaf_node_t>(node);
        return boost::make_optional(new leaf_iterator_t(l_node, 0, buf_lock, transactor));
    }

    block_id_t get_child_id(const internal_node_t *i_node, int index) const {
        block_id_t child_id = internal_node::get_pair_by_index(i_node, index)->lnode;
        rassert(child_id != NULL_BLOCK_ID);
        rassert(child_id != SUPERBLOCK_ID);

        return child_id;
    }

    boost::shared_ptr<transactor_t> transactor;
    btree_slice_t *slice;
    store_key_t *start;
    store_key_t *end;
    bool left_open;
    bool right_open;
    std::list<internal_node_state> traversal_state;
    volatile bool started;
    volatile bool nevermore;
};

class slice_keys_iterator : public ordered_data_iterator<key_with_data_provider_t> {
public:
    slice_keys_iterator(boost::shared_ptr<transactor_t> transactor, btree_slice_t *slice, store_key_t *start, store_key_t *end, bool left_open, bool right_open) :
        transactor(transactor), slice(slice), start(start), start_str(key_to_str(start)), end(end), end_str(key_to_str(end)), left_open(left_open), right_open(right_open),
        no_more_data(false), active_leaf(NULL), leaves_iterator(NULL) { }

    virtual ~slice_keys_iterator() {
        if (active_leaf) {
            delete active_leaf;
        }
        if (leaves_iterator) {
            delete leaves_iterator;
        }
    }
    boost::optional<key_with_data_provider_t> next() {
        if (no_more_data)
            return boost::none;

        boost::optional<key_with_data_provider_t> result = active_leaf == NULL ? get_first_value() : get_next_value();
        if (!result) {
            done();
        }
        return result;
    }
    void prefetch() {
    }
private:
    boost::optional<key_with_data_provider_t> get_first_value() {
        leaves_iterator = new slice_leaves_iterator_t(transactor, slice, start, end, left_open, right_open);

        // get the first leaf with our start key (or something greater)
        boost::optional<leaf_iterator_t*> first = leaves_iterator->next();
        if (!first)
            return boost::none;

        active_leaf = first.get();
        rassert(active_leaf != NULL);

        // get a value from the leaf
        boost::optional<key_with_data_provider_t> first_pair = active_leaf->next();
        if (!first_pair)
            return first_pair;  // returns empty result

        key_with_data_provider_t pair = first_pair.get();

        // skip the start key if left_open
        int compare_result = pair.key.compare(start_str);
        rassert(compare_result >= 0);
        if (left_open && compare_result == 0) {
            return get_next_value();
        } else {
            return validate_return_value(pair);
        }
    }

    boost::optional<key_with_data_provider_t> get_next_value() {
        rassert(leaves_iterator != NULL);
        rassert(active_leaf != NULL);

        // get a value from the leaf
        boost::optional<key_with_data_provider_t> current_pair = active_leaf->next();
        if (!current_pair) {
            delete active_leaf;
            active_leaf = NULL;

            boost::optional<leaf_iterator_t*> leaf = leaves_iterator->next();
            if (!leaf)
                return boost::none;

            active_leaf = leaf.get();
            current_pair = active_leaf->next();
            if (!current_pair)
                return boost::none;
        }

        return validate_return_value(current_pair.get());
    }

    boost::optional<key_with_data_provider_t> validate_return_value(key_with_data_provider_t &pair) const {
        int compare_result = pair.key.compare(end_str);
        if (compare_result < 0 || (!right_open && compare_result == 0)) {
            return boost::make_optional(pair);
        } else {
            return boost::none;
        }
    }

    void done() {
        // we don't delete the active_leaf here because someone may still be using its data
        if (leaves_iterator) {
            delete leaves_iterator;
            leaves_iterator = NULL;
        }
        no_more_data = true;
    }

    boost::shared_ptr<transactor_t> transactor;
    btree_slice_t *slice;
    store_key_t *start;
    std::string start_str;
    store_key_t *end;
    std::string end_str;
    bool left_open;
    bool right_open;

    bool no_more_data;
    leaf_iterator_t *active_leaf;
    slice_leaves_iterator_t *leaves_iterator;
};

store_t::rget_result_t btree_rget(btree_key_value_store_t *store, store_key_t *start, store_key_t *end, bool left_open, bool right_open, uint64_t max_results) {
    store_t::rget_result_t result;
    std::vector<boost::shared_ptr<transactor_t> > transactors;

    merge_ordered_data_iterator<key_with_data_provider_t,key_with_data_provider_t::less>::mergees_t ms;
    for (int s = 0; s < store->btree_static_config.n_slices; s++) {
        btree_slice_t *slice = store->slices[s];
        boost::shared_ptr<transactor_t> transactor = boost::shared_ptr<transactor_t>(new transactor_t(&slice->cache, rwi_read));
        transactors.push_back(transactor);
        ms.push_back(new slice_keys_iterator(transactor, slice, start, end, left_open, right_open));
    }

    merge_ordered_data_iterator<key_with_data_provider_t,key_with_data_provider_t::less> merge_iterator(ms);
    boost::optional<key_with_data_provider_t> pair;
    while (pair = merge_iterator.next()) {
        result.results.push_back(pair.get());

        if (result.results.size() == max_results)
            break;
    }

    for (merge_ordered_data_iterator<key_with_data_provider_t,key_with_data_provider_t::less>::mergees_t::iterator i = ms.begin(); i != ms.end(); i++) {
        delete *i;
    }
    return result;
}

store_t::rget_result_t btree_rget_slice_iterator(btree_slice_t *slice, store_key_t *start, store_key_t *end, bool left_open, bool right_open, uint64_t max_results) {
    return store_t::rget_result_t();
}

store_t::rget_result_t btree_rget_slice(btree_slice_t *slice, store_key_t *start, store_key_t *end, bool left_open, bool right_open, uint64_t max_results) {
    return store_t::rget_result_t();
}
/*
rget_large_value_provider_t::rget_large_value_provider_t(const btree_value *value, boost::shared_ptr<transactor_t> transactor)
    : transactor(transactor), large_value(new large_buf_t(transactor->transaction())) {

    co_acquire_large_value(large_value, value->lb_ref(), rwi_read);
    rassert(large_value->state == large_buf_t::loaded);
    rassert(large_value->get_root_ref().block_id == value.lb_ref().block_id);
}
*/

rget_large_value_provider_t::~rget_large_value_provider_t() {
    if (large_value) {
        delete large_value;
    }
}

rget_value_provider_t *rget_value_provider_t::create_provider(const btree_value *value, boost::shared_ptr<transactor_t> transactor) {
    if (value->is_large())
        return new rget_large_value_provider_t(value, transactor);
    else
        return new rget_small_value_provider_t(value);
}
