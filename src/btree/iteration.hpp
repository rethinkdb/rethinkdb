#ifndef __BTREE_ITERATION_HPP__
#define __BTREE_ITERATION_HPP__

#include <boost/shared_ptr.hpp>
#include "containers/iterators.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "btree/slice.hpp"

struct leaf_iterator_t : public one_way_iterator_t<key_with_data_provider_t> {
    leaf_iterator_t(const leaf_node_t *leaf, int index, buf_lock_t *lock, const boost::shared_ptr<transactor_t>& transactor);

    boost::optional<key_with_data_provider_t> next();
    void prefetch();
    virtual ~leaf_iterator_t();
private:
    void done();
    key_with_data_provider_t pair_to_key_with_data_provider(const btree_leaf_pair* pair);

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
class slice_leaves_iterator_t : public one_way_iterator_t<leaf_iterator_t*> {
    struct internal_node_state {
        internal_node_state(const internal_node_t *node, int index, buf_lock_t *lock)
            : node(node), index(index), lock(lock) { }

        const internal_node_t *node;
        int index;
        buf_lock_t *lock;
    };
public:
    slice_leaves_iterator_t(const boost::shared_ptr<transactor_t>& transactor, btree_slice_t *slice, store_key_t *start, store_key_t *end, bool left_open, bool right_open);

    boost::optional<leaf_iterator_t*> next();
    void prefetch();
    virtual ~slice_leaves_iterator_t();
private:
    void done();

    boost::optional<leaf_iterator_t*> get_first_leaf();
    boost::optional<leaf_iterator_t*> get_next_leaf();
    boost::optional<leaf_iterator_t*> get_leftmost_leaf(block_id_t node_id);
    block_id_t get_child_id(const internal_node_t *i_node, int index) const;

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

class slice_keys_iterator_t : public one_way_iterator_t<key_with_data_provider_t> {
public:
    slice_keys_iterator_t(const boost::shared_ptr<transactor_t>& transactor, btree_slice_t *slice, store_key_t *start, store_key_t *end, bool left_open, bool right_open);
    virtual ~slice_keys_iterator_t();

    boost::optional<key_with_data_provider_t> next();
    void prefetch();
private:
    boost::optional<key_with_data_provider_t> get_first_value();
    boost::optional<key_with_data_provider_t> get_next_value();

    boost::optional<key_with_data_provider_t> validate_return_value(key_with_data_provider_t &pair) const;

    void done();

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


#endif // __BTREE_ITERATION_HPP__
