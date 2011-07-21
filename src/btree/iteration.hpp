#ifndef __BTREE_ITERATION_HPP__
#define __BTREE_ITERATION_HPP__

#include "errors.hpp"
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>

#include "containers/iterators.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "btree/operations.hpp"
#include "store.hpp"


template <class Value>
struct key_value_pair_t {
    std::string key;
    boost::shared_array<char> value;

    // TODO! This is just a hack, because it doesn't compare value.
    // We should really use a specialized comparator in the iterators that need this.
    bool operator==(const key_value_pair_t &k) const {
        return key == k.key;
    }
    bool operator<(const key_value_pair_t &k) const {
        return key < k.key;
    }

    key_value_pair_t(value_sizer_t<Value> *sizer, const std::string& _key, const Value *_value) : key(_key) {
        int size = sizer->size(reinterpret_cast<const Value *>(_value));
        value.reset(new char[size]);
        memcpy(value.get(), _value, size);
    }
};

/* leaf_iterator_t returns the keys of a btree leaf node one by one.
 * When it's done, it releases (deletes) the buf_lock object.
 *
 * TODO: should the buf_lock be released by the caller instead?
 */
template <class Value>
struct leaf_iterator_t : public one_way_iterator_t<key_value_pair_t<Value> > {
    leaf_iterator_t(const leaf_node_t *leaf, int index, buf_lock_t *lock, const boost::shared_ptr<value_sizer_t<Value> >& sizer, const boost::shared_ptr<transaction_t>& transaction);

    boost::optional<key_value_pair_t<Value> > next();
    void prefetch();
    virtual ~leaf_iterator_t();
private:
    void done();
    key_with_data_provider_t pair_to_key_with_data_provider(const btree_leaf_pair<Value>* pair);

    const leaf_node_t *leaf;
    int index;
    buf_lock_t *lock;
    boost::shared_ptr<value_sizer_t<Value> > sizer;
    boost::shared_ptr<transaction_t> transaction;
};

/* slice_leaves_iterator_t finds the first leaf that contains the given key (or
 * the next key, if left_open is true). It returns that leaf iterator (which
 * also contains the lock), however it doesn't release the leaf lock itself
 * (it's done by the leaf iterator).
 *
 * slice_leaves_iterator_t maintains internal state by locking some internal
 * nodes and unlocking them as iteration progresses. Currently this locking is
 * done in DFS manner, as described in the btree/rget.cc file comment.
 */
template <class Value>
class slice_leaves_iterator_t : public one_way_iterator_t<leaf_iterator_t<Value>*> {
    struct internal_node_state {
        internal_node_state(const internal_node_t *node, int index, buf_lock_t *lock)
            : node(node), index(index), lock(lock) { }

        const internal_node_t *node;
        int index;
        buf_lock_t *lock;
    };
public:
    slice_leaves_iterator_t(const boost::shared_ptr<value_sizer_t<Value> >& sizer, const boost::shared_ptr<transaction_t>& transaction, boost::scoped_ptr<superblock_t> &superblock, int slice_home_thread, rget_bound_mode_t left_mode, const btree_key_t *left_key, rget_bound_mode_t right_mode, const btree_key_t *right_key);

    boost::optional<leaf_iterator_t<Value>*> next();
    void prefetch();
    virtual ~slice_leaves_iterator_t();
private:
    void done();

    boost::optional<leaf_iterator_t<Value>*> get_first_leaf();
    boost::optional<leaf_iterator_t<Value>*> get_next_leaf();
    boost::optional<leaf_iterator_t<Value>*> get_leftmost_leaf(block_id_t node_id);
    block_id_t get_child_id(const internal_node_t *i_node, int index) const;

    boost::shared_ptr<value_sizer_t<Value> > sizer;
    boost::shared_ptr<transaction_t> transaction;
    boost::scoped_ptr<superblock_t> superblock;
    int slice_home_thread;
    rget_bound_mode_t left_mode;
    const btree_key_t *left_key;
    rget_bound_mode_t right_mode;
    const btree_key_t *right_key;

    std::list<internal_node_state> traversal_state;
    bool started;
    bool nevermore;
};

/* slice_keys_iterator_t combines slice_leaves_iterator_t and leaf_iterator_t to allow you
 * to iterate through the keys of a particular slice in order.
 *
 * Use merge_ordered_data_iterator_t class to funnel multiple slice_keys_iterator_t instances,
 * e.g. to get a range query for all the slices.
 */
template <class Value>
class slice_keys_iterator_t : public one_way_iterator_t<key_value_pair_t<Value> > {
public:
    /* Cannot assume that 'start' and 'end' will remain valid after the constructor returns! */
    slice_keys_iterator_t(const boost::shared_ptr<value_sizer_t<Value> >& sizer, const boost::shared_ptr<transaction_t>& transaction, boost::scoped_ptr<superblock_t> &superblock, int slice_home_thread, rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key);
    virtual ~slice_keys_iterator_t();

    boost::optional<key_value_pair_t<Value> > next();
    void prefetch();
private:
    boost::optional<key_value_pair_t<Value> > get_first_value();
    boost::optional<key_value_pair_t<Value> > get_next_value();

    boost::optional<key_value_pair_t<Value> > validate_return_value(key_value_pair_t<Value> &pair) const;

    void done();

    boost::shared_ptr<value_sizer_t<Value> > sizer;
    boost::shared_ptr<transaction_t> transaction;
    boost::scoped_ptr<superblock_t> superblock;
    int slice_home_thread;
    rget_bound_mode_t left_mode;
    btree_key_buffer_t left_key;
    rget_bound_mode_t right_mode;
    btree_key_buffer_t right_key;
    std::string left_str;
    std::string right_str;

    bool no_more_data;
    leaf_iterator_t<Value> *active_leaf;
    slice_leaves_iterator_t<Value> *leaves_iterator;
};

#include "btree/iteration.tcc"


#endif // __BTREE_ITERATION_HPP__
