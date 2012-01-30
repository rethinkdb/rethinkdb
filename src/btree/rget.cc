#include "btree/rget.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#include "btree/btree_data_provider.hpp"
#include "btree/iteration.hpp"
#include "containers/iterators.hpp"
#include "arch/runtime/runtime.hpp"

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
 * Currently the DFS design is implemented, since it's the simplest solution, also it is a good
 * fit for small rgets (the most popular use-case).
 *
 *
 * Most of the implementation now resides in btree/iteration.{hpp,cc}.
 * Actual merging of the slice iterators is done in server/key_value_store.cc.
 */

bool is_not_expired(key_value_pair_t<memcached_value_t>& pair) {
    const memcached_value_t *value = reinterpret_cast<const memcached_value_t *>(pair.value.get());
    return !value->expired();
}

key_with_data_buffer_t pair_to_key_with_data_buffer(transaction_t *txn, key_value_pair_t<memcached_value_t>& pair) {
    on_thread_t th(txn->home_thread());
    boost::intrusive_ptr<data_buffer_t> data_provider(value_to_data_buffer(reinterpret_cast<memcached_value_t *>(pair.value.get()), txn));
    return key_with_data_buffer_t(pair.key, reinterpret_cast<memcached_value_t *>(pair.value.get())->mcflags(), data_provider);
}

template <class T>
struct transaction_holding_iterator_t : public one_way_iterator_t<T> {
    transaction_holding_iterator_t(boost::scoped_ptr<transaction_t>& txn, one_way_iterator_t<T> *ownee)
        : txn_(), ownee_(ownee) {
        txn_.swap(txn);
    }

    ~transaction_holding_iterator_t() {
        delete ownee_;

        on_thread_t th(txn_->home_thread());
        txn_.reset();
    }

    typename boost::optional<T> next() {
        return ownee_->next();
    }

    void prefetch() {
        ownee_->prefetch();
    }

private:
    boost::scoped_ptr<transaction_t> txn_;
    one_way_iterator_t<T> *ownee_;

    DISABLE_COPYING(transaction_holding_iterator_t);
};

rget_result_t btree_rget_slice(btree_slice_t *slice, sequence_group_t *seq_group, rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key, order_token_t token) {
    slice->pre_begin_transaction_sink_.check_out(token);
    UNUSED order_token_t begin_transaction_token = slice->pre_begin_transaction_read_mode_source_.check_in(token.tag() + "+begin_transaction_token").with_read_mode();

    transaction_t *transaction = new transaction_t(slice->cache(), seq_group, rwi_read, 0, repli_timestamp_t::distant_past);
    boost::scoped_ptr<transaction_t> txn(transaction);

    transaction->set_token(slice->post_begin_transaction_checkpoint_.check_through(token).with_read_mode());

    boost::shared_ptr<value_sizer_t<memcached_value_t> > sizer = boost::make_shared<memcached_value_sizer_t>(transaction->get_cache()->get_block_size());
    transaction->snapshot();

    // Get the superblock of the slice
    slice->assert_thread();
    buf_lock_t sb_buf(transaction, SUPERBLOCK_ID, rwi_read);
    // TODO MERGE make eviction priorities sane.  Make the numbers
    // sane, make buf_lock_t take them in the constructor.
    sb_buf->set_eviction_priority(ZERO_EVICTION_PRIORITY);
    boost::scoped_ptr<superblock_t> superblock(new real_superblock_t(sb_buf));

    return boost::shared_ptr<one_way_iterator_t<key_with_data_buffer_t> >(
       new transaction_holding_iterator_t<key_with_data_buffer_t>(txn,
            new transform_iterator_t<key_value_pair_t<memcached_value_t>, key_with_data_buffer_t>(
                boost::bind(pair_to_key_with_data_buffer, transaction, _1),
                new filter_iterator_t<key_value_pair_t<memcached_value_t> >(
                    is_not_expired,
                    new slice_keys_iterator_t<memcached_value_t>(sizer, transaction, superblock, slice->home_thread(), left_mode, left_key, right_mode, right_key)))));
}
