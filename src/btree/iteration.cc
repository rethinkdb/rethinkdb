#include "btree/iteration.hpp"
#include "btree/btree_data_provider.hpp"

leaf_iterator_t::leaf_iterator_t(const leaf_node_t *leaf, int index, buf_lock_t *lock, const boost::shared_ptr<transactor_t>& transactor) :
    leaf(leaf), index(index), lock(lock), transactor(transactor) {

    rassert(leaf != NULL);
    rassert(lock != NULL);
}

boost::optional<key_with_data_provider_t> leaf_iterator_t::next() {
    rassert(index >= 0);
    const btree_leaf_pair *pair;
    do {
        if (index >= leaf->npairs) {
            done();
            return boost::none;
        }
        pair = leaf::get_pair_by_index(leaf, index++);
    } while (pair->value()->expired());

    return boost::make_optional(pair_to_key_with_data_provider(pair));
}

void leaf_iterator_t::prefetch() {
    /* this space intentionally left blank */
}

leaf_iterator_t::~leaf_iterator_t() {
    done();
}

void leaf_iterator_t::done() {
    if (lock) {
        transactor->transaction()->ensure_thread();
        delete lock;
        lock = NULL;
    }
}

key_with_data_provider_t leaf_iterator_t::pair_to_key_with_data_provider(const btree_leaf_pair* pair) {
    value_data_provider_t *data_provider = value_data_provider_t::create(pair->value(), transactor); 
    return key_with_data_provider_t(key_to_str(&pair->key), pair->value()->mcflags(),
        boost::shared_ptr<data_provider_t>(data_provider));
}

slice_leaves_iterator_t::slice_leaves_iterator_t(const boost::shared_ptr<transactor_t>& transactor, btree_slice_t *slice,
    rget_bound_mode_t left_mode, const btree_key_t *left_key, rget_bound_mode_t right_mode, const btree_key_t *right_key) : 
        transactor(transactor), slice(slice),
        left_mode(left_mode), left_key(left_key), right_mode(right_mode), right_key(right_key),
        traversal_state(), started(false), nevermore(false) {
}

boost::optional<leaf_iterator_t*> slice_leaves_iterator_t::next() {
    if (nevermore)
        return boost::none;

    boost::optional<leaf_iterator_t*> leaf = !started ? get_first_leaf() : get_next_leaf();
    if (!leaf)
        done();
    return leaf;
}

void slice_leaves_iterator_t::prefetch() {
    // Not implemented yet
}

slice_leaves_iterator_t::~slice_leaves_iterator_t() {
    done();
}

void slice_leaves_iterator_t::done() {
    while (!traversal_state.empty()) {
        delete traversal_state.back().lock;
        traversal_state.pop_back();
    }
    nevermore = true;
}

boost::optional<leaf_iterator_t*> slice_leaves_iterator_t::get_first_leaf() {
    on_thread_t mover(slice->home_thread); // Move to the slice's thread.

    started = true;
    buf_lock_t *buf_lock = new buf_lock_t(*transactor, block_id_t(SUPERBLOCK_ID), rwi_read);
    block_id_t root_id = ptr_cast<btree_superblock_t>(buf_lock->buf()->get_data_read())->root_block;
    rassert(root_id != SUPERBLOCK_ID);

    if (root_id == NULL_BLOCK_ID) {
        delete buf_lock;
        return boost::none;
    }

    if (left_mode == rget_bound_none) {
        boost::optional<leaf_iterator_t*> leftmost_leaf = get_leftmost_leaf(root_id);
        delete buf_lock;
        return leftmost_leaf;
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
        int index = internal_node::impl::get_offset_index(i_node, left_key);
        rassert(index >= 0);
        if (index >= i_node->npairs) {
            // this subtree has all the keys smaller than 'left_key', move to the leaf in the next subtree
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
    int index = leaf::impl::get_offset_index(l_node, left_key);

    if (index < l_node->npairs) {
        return boost::make_optional(new leaf_iterator_t(l_node, index, buf_lock, transactor));
    } else {
        // there's nothing more in this leaf, we might as well move to the next leaf
        delete buf_lock;
        return get_next_leaf();
    }
}

boost::optional<leaf_iterator_t*> slice_leaves_iterator_t::get_next_leaf() {
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
            delete state.lock;  // this also releases the memory used by state.node
        }
    }
    return boost::none;
}

boost::optional<leaf_iterator_t*> slice_leaves_iterator_t::get_leftmost_leaf(block_id_t node_id) {
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

block_id_t slice_leaves_iterator_t::get_child_id(const internal_node_t *i_node, int index) const {
    block_id_t child_id = internal_node::get_pair_by_index(i_node, index)->lnode;
    rassert(child_id != NULL_BLOCK_ID);
    rassert(child_id != SUPERBLOCK_ID);

    return child_id;
}

slice_keys_iterator_t::slice_keys_iterator_t(const boost::shared_ptr<transactor_t>& transactor_, btree_slice_t *slice_,
        rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key) :
    transactor(transactor_), slice(slice_),
    left_mode(left_mode), left_key(left_key), right_mode(right_mode), right_key(right_key),
    left_str(key_to_str(left_key)), right_str(key_to_str(right_key)),
    no_more_data(false), active_leaf(NULL), leaves_iterator(NULL) { }

slice_keys_iterator_t::~slice_keys_iterator_t() {
    done();
}

boost::optional<key_with_data_provider_t> slice_keys_iterator_t::next() {
    if (no_more_data)
        return boost::none;

    boost::optional<key_with_data_provider_t> result = active_leaf == NULL ? get_first_value() : get_next_value();
    if (!result) {
        done();
    }
    return result;
}

void slice_keys_iterator_t::prefetch() {
}

boost::optional<key_with_data_provider_t> slice_keys_iterator_t::get_first_value() {
    leaves_iterator = new slice_leaves_iterator_t(transactor, slice, left_mode, left_key.key(), right_mode, right_key.key());

    // get the first leaf with our left key (or something greater)
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

    // skip the left key if left_mode == rget_bound_mode_open
    int compare_result = pair.key.compare(left_str);
    rassert(compare_result >= 0);
    if (left_mode == rget_bound_open && compare_result == 0) {
        return get_next_value();
    } else {
        return validate_return_value(pair);
    }
}

boost::optional<key_with_data_provider_t> slice_keys_iterator_t::get_next_value() {
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

boost::optional<key_with_data_provider_t> slice_keys_iterator_t::validate_return_value(key_with_data_provider_t &pair) const {
    if (right_mode == rget_bound_none)
        return boost::make_optional(pair);

    int compare_result = pair.key.compare(right_str);
    if (compare_result < 0 || (right_mode == rget_bound_closed && compare_result == 0)) {
        return boost::make_optional(pair);
    } else {
        return boost::none;
    }
}

void slice_keys_iterator_t::done() {
    if (active_leaf) {
        delete active_leaf;
        active_leaf = NULL;
    }
    if (leaves_iterator) {
        delete leaves_iterator;
        leaves_iterator = NULL;
    }
    no_more_data = true;
}

