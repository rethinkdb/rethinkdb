#ifndef BTREE_ITERATION_ITERATION_HPP_
#define BTREE_ITERATION_ITERATION_HPP_

#include "btree/iteration.hpp"

#include "perfmon.hpp"
#include "buffer_cache/buffer_cache.hpp"

extern perfmon_counter_t
    leaf_iterators,
    slice_leaves_iterators;

template <class Value>
leaf_iterator_t<Value>::leaf_iterator_t(const leaf_node_t *_leaf, leaf::live_iter_t _iter, buf_lock_t *_lock, const boost::shared_ptr< value_sizer_t<void> >& _sizer, transaction_t *_transaction, btree_stats_t *_stats) :
    leaf(_leaf), iter(_iter), lock(_lock), sizer(_sizer), transaction(_transaction), stats(_stats) {

    rassert(leaf != NULL);
    rassert(lock != NULL);
}

template <class V>
boost::optional<key_value_pair_t<V> > leaf_iterator_t<V>::next() {
    const btree_key_t *k = iter.get_key(leaf);

    if (!k) {
        done();
        return boost::none;
    } else {
        const V *value = static_cast<const V *>(iter.get_value(leaf));
        iter.step(leaf);
        stats->pm_keys_read.record();
        return boost::make_optional(key_value_pair_t<V>(sizer.get(), store_key_t(k->size, k->contents), value));
    }
}

template <class Value>
void leaf_iterator_t<Value>::prefetch() {
    /* this space intentionally left blank */
}

template <class Value>
leaf_iterator_t<Value>::~leaf_iterator_t() {
    done();
}

template <class Value>
void leaf_iterator_t<Value>::done() {
    if (lock) {
        on_thread_t th(transaction->home_thread());
        delete lock;
        lock = NULL;
    }
}

template <class Value>
slice_leaves_iterator_t<Value>::slice_leaves_iterator_t(const boost::shared_ptr< value_sizer_t<void> >& _sizer, transaction_t *_transaction, boost::scoped_ptr<superblock_t> &_superblock, int _slice_home_thread,
        const btree_key_t *_left, btree_stats_t *_stats) 
    : sizer(_sizer), transaction(_transaction), slice_home_thread(_slice_home_thread), left(_left),
      traversal_state(), started(false), nevermore(false), stats(_stats)
{
    superblock.swap(_superblock);
}

template <class Value>
boost::optional<leaf_iterator_t<Value>*> slice_leaves_iterator_t<Value>::next() {
    if (nevermore)
        return boost::none;

    boost::optional<leaf_iterator_t<Value>*> leaf = !started ? get_first_leaf() : get_next_leaf();
    if (!leaf)
        done();
    return leaf;
}

template <class Value>
void slice_leaves_iterator_t<Value>::prefetch() {
    // Not implemented yet
}

template <class Value>
slice_leaves_iterator_t<Value>::~slice_leaves_iterator_t() {
    done();
}

template <class Value>
void slice_leaves_iterator_t<Value>::done() {
    on_thread_t th(transaction->home_thread());
    while (!traversal_state.empty()) {
        delete traversal_state.back().lock;
        traversal_state.pop_back();
    }
    superblock.reset();
    nevermore = true;
}

template <class Value>
boost::optional<leaf_iterator_t<Value>*> slice_leaves_iterator_t<Value>::get_first_leaf() {
    on_thread_t mover(slice_home_thread); // Move to the slice's thread.

    started = true;
    block_id_t root_id = superblock->get_root_block_id();
    rassert(root_id != SUPERBLOCK_ID);

    if (root_id == NULL_BLOCK_ID) {
        superblock->release();
        return boost::none;
    }

    // TODO: Can we do all this without having buf_lock being a pointer?
    buf_lock_t *buf_lock = new buf_lock_t(transaction, root_id, rwi_read);
    superblock->release();

    // read root node
    const node_t *node = reinterpret_cast<const node_t *>(buf_lock->get_data_read());
    rassert(node != NULL);

    while (node::is_internal(node)) {
        const internal_node_t *i_node = reinterpret_cast<const internal_node_t *>(node);

        // push the i_node onto the traversal_state stack
        int index = internal_node::impl::get_offset_index(i_node, left);
        rassert(index >= 0);
        if (index >= i_node->npairs) {
            // this subtree has all the keys smaller than `left`, move to
            // the leaf in the next subtree
            delete buf_lock;
            return get_next_leaf();
        }
        traversal_state.push_back(internal_node_state(i_node, index, buf_lock));

        block_id_t child_id = get_child_id(i_node, index);

        buf_lock = new buf_lock_t(transaction, child_id, rwi_read);
        node = reinterpret_cast<const node_t *>(buf_lock->get_data_read());
    }

    rassert(node != NULL);
    rassert(node::is_leaf(node));
    rassert(buf_lock != NULL);

    const leaf_node_t *l_node = reinterpret_cast<const leaf_node_t *>(node);
    leaf::live_iter_t iter = leaf::iter_for_inclusive_lower_bound(l_node, left);

    if (iter.get_key(l_node)) {
        return boost::make_optional(new leaf_iterator_t<Value>(l_node, iter, buf_lock, sizer, transaction, stats));
    } else {
        // there's nothing more in this leaf, we might as well move to the next leaf
        delete buf_lock;
        return get_next_leaf();
    }
}

template <class Value>
boost::optional<leaf_iterator_t<Value>*> slice_leaves_iterator_t<Value>::get_next_leaf() {
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
            on_thread_t th(slice_home_thread);
            delete state.lock;  // this also releases the memory used by state.node
        }
    }
    return boost::none;
}

template <class Value>
boost::optional<leaf_iterator_t<Value>*> slice_leaves_iterator_t<Value>::get_leftmost_leaf(block_id_t node_id) {
    on_thread_t mover(slice_home_thread); // Move to the slice's thread.

    // TODO: Why is there a buf_lock_t pointer?  This is not how
    // buf_lock_t works.  Just use a buf_lock_t pointer then.
    buf_lock_t *buf_lock = new buf_lock_t(transaction, node_id, rwi_read);
    const node_t *node = reinterpret_cast<const node_t *>(buf_lock->get_data_read());

    while (node::is_internal(node)) {
        const internal_node_t *i_node = reinterpret_cast<const internal_node_t *>(node);
        rassert(i_node->npairs > 0);

        // push the i_node onto the traversal_state stack
        const int leftmost_child_index = 0;
        traversal_state.push_back(internal_node_state(i_node, leftmost_child_index, buf_lock));

        block_id_t child_id = get_child_id(i_node, leftmost_child_index);

        buf_lock = new buf_lock_t(transaction, child_id, rwi_read);
        node = reinterpret_cast<const node_t *>(buf_lock->get_data_read());
    }
    rassert(node != NULL);
    rassert(node::is_leaf(node));
    rassert(buf_lock != NULL);

    const leaf_node_t *l_node = reinterpret_cast<const leaf_node_t *>(node);
    return boost::make_optional(new leaf_iterator_t<Value>(l_node, leaf::iter_for_whole_leaf(l_node), buf_lock, sizer, transaction, stats));
}

template <class Value>
block_id_t slice_leaves_iterator_t<Value>::get_child_id(const internal_node_t *i_node, int index) const {
    block_id_t child_id = internal_node::get_pair_by_index(i_node, index)->lnode;
    rassert(child_id != NULL_BLOCK_ID);
    rassert(child_id != SUPERBLOCK_ID);

    return child_id;
}

template <class Value>
slice_keys_iterator_t<Value>::slice_keys_iterator_t(const boost::shared_ptr< value_sizer_t<void> >& _sizer, transaction_t *_transaction, boost::scoped_ptr<superblock_t> &_superblock, int _slice_home_thread,
        const key_range_t &_range, btree_stats_t *_stats) :
    sizer(_sizer), transaction(_transaction), slice_home_thread(_slice_home_thread),
    range(_range), left_buffer(range.left), no_more_data(false), active_leaf(NULL), leaves_iterator(NULL), stats(_stats)
{
    superblock.swap(_superblock);
}

template <class Value>
slice_keys_iterator_t<Value>::~slice_keys_iterator_t() {
    done();
}

template <class Value>
boost::optional<key_value_pair_t<Value> > slice_keys_iterator_t<Value>::next() {
    if (no_more_data)
        return boost::none;

    boost::optional<key_value_pair_t<Value> > result = active_leaf == NULL ? get_first_value() : get_next_value();
    if (!result) {
        done();
    }
    return result;
}

template <class Value>
void slice_keys_iterator_t<Value>::prefetch() {
}

template <class Value>
boost::optional<key_value_pair_t<Value> > slice_keys_iterator_t<Value>::get_first_value() {
    rassert(superblock);
    leaves_iterator = new slice_leaves_iterator_t<Value>(sizer, transaction, superblock, slice_home_thread, left_buffer.key(), stats);

    // get the first leaf with our left key (or something greater)
    boost::optional<leaf_iterator_t<Value>*> first = leaves_iterator->next();
    if (!first)
        return boost::none;

    active_leaf = first.get();
    rassert(active_leaf != NULL);

    // get a value from the leaf
    boost::optional<key_value_pair_t<Value> > first_pair = active_leaf->next();
    if (!first_pair) {
        return first_pair;  // returns empty result
    }

    key_value_pair_t<Value> pair = first_pair.get();
    rassert(pair.key >= range.left);

    return validate_return_value(pair);
}

template <class Value>
boost::optional<key_value_pair_t<Value> > slice_keys_iterator_t<Value>::get_next_value() {
    rassert(leaves_iterator != NULL);
    rassert(active_leaf != NULL);

    // get a value from the leaf
    boost::optional<key_value_pair_t<Value> > current_pair = active_leaf->next();
    if (!current_pair) {
        delete active_leaf;
        active_leaf = NULL;

        boost::optional<leaf_iterator_t<Value>*> leaf = leaves_iterator->next();
        if (!leaf) {
            return boost::none;
        }

        active_leaf = leaf.get();
        current_pair = active_leaf->next();
        if (!current_pair) {
            return boost::none;
        }
    }

    return validate_return_value(current_pair.get());
}

template <class Value>
boost::optional<key_value_pair_t<Value> > slice_keys_iterator_t<Value>::validate_return_value(key_value_pair_t<Value> &pair) const {
    if (range.contains_key(pair.key)) {
        return boost::make_optional(pair);
    } else {
        return boost::none;
    }
}

template <class Value>
void slice_keys_iterator_t<Value>::done() {
    if (active_leaf) {
        delete active_leaf;
        active_leaf = NULL;
    }
    if (leaves_iterator) {
        delete leaves_iterator;
        leaves_iterator = NULL;
    }
    {
        on_thread_t th(transaction->home_thread());
        superblock.reset();
    }

    no_more_data = true;
}

#endif  // BTREE_ITERATION_ITERATION_HPP_
