#ifndef __SERVER_NESTED_DEMO_REDIS_SET_VALUES_HPP__
#define	__SERVER_NESTED_DEMO_REDIS_SET_VALUES_HPP__

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include "server/nested_demo/redis_utils.hpp"
#include "btree/iteration.hpp"
#include "config/args.hpp"

struct redis_set_value_t {
    block_id_t nested_root;
    uint32_t size;

public:
    int inline_size(UNUSED block_size_t bs) const {
        return sizeof(nested_root) + sizeof(size);
    }

    int64_t value_size() const {
        return 0;
    }

    const char *value_ref() const { return NULL; }
    char *value_ref() { return NULL; }


    /* Some operations that you can do on a hash (resembling redis commands)... */

    // For checking if a member is too long
    bool does_member_fit(const std::string &member) const {
        return member.length() <= MAX_KEY_SIZE;
    }

    bool sismember(value_sizer_t<redis_set_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &member) const;

    int scard() const;

    // returns true if a member has been deleted (i.e. existed in the set)
    bool srem(value_sizer_t<redis_set_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &member);

    // Returns true iff the member was new
    bool sadd(value_sizer_t<redis_set_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &member);

    // Deletes all elements from the subtree. Must be called before deleting the set value in the super tree!
    void clear(value_sizer_t<redis_set_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, int slice_home_thread);

    // Provides an iterator over all members
    boost::shared_ptr<one_way_iterator_t<std::string> > smembers(value_sizer_t<redis_set_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, int slice_home_thread) const;

    /*
     Set operations can be implemented on top of smembers, and this is how:
     intersection of S1, S2, ..., Sn:
        get iterators i1, ..., in by calling smembers on S1, ..., Sn respectively
        create a merge_ordered_data_iterator_t m and add i1, ..., in as mergees
        put m into a repetition_filter_iterator_t r with the n_repetitions argument being n
        r then provides the intersection result.
     union of S1, S2, ..., Sn:
        get iterators i1, ..., in by calling smembers on S1, ..., Sn respectively
        create a merge_ordered_data_iterator_t m and add i1, ..., in as mergees
        put m into a unique_filter_iterator_t u
        u then provides the union result.
     difference S1 - S2:
        get iterators i1, i2 by calling smembers on S1 and S2 respectively
        create a diff_filter_iterator_t d around i1 and i2
        d then provides the difference.

     
     Please note that set operations that span multiple sets might not be allowed
     to simultaneously hold a lock on mutliple redis_set_value_t keys in the
     main b-tree, as that might cause deadlocks. You either have to be very
     careful, or use a snapshot. For those operations that immediately store
     the result of the operation into a target set (like SINTERSTORE), you can
     also just generate the resulting set in memory first and then store
     it in a second operation. Alternatively, if the destination set is new,
     you can create the destination redis_set_value_t without actually inserting
     it into the main btree yet. Once all result members are in the destination
     set, you release the input sets and actually put the destination set into
     the main btree. Be careful however to use the same write transaction for that,
     as you would otherwise get problems with flushes writing the nested btree nodes, before
     there actually is a way to find it through the main btree.
     */


private:
    // Transforms from key_value_pair_t<redis_nested__empty_value_t> to key strings for
    // cleaner outside-facing iterator interface
    std::string transform_value(const key_value_pair_t<redis_nested_empty_value_t> &pair) const {
        return pair.key;
    }
} __attribute__((__packed__));

template <>
class value_sizer_t<redis_set_value_t> {
public:
    value_sizer_t<redis_set_value_t>(block_size_t bs) : block_size_(bs) { }

    int size(const redis_set_value_t *value) const {
        return value->inline_size(block_size_);
    }

    bool fits(const redis_set_value_t *value, int length_available) const {
        return size(value) <= length_available;
    }

    int max_possible_size() const {
        // It's of constant size...
        return sizeof(redis_set_value_t);
    }

    block_magic_t btree_leaf_magic() const {
        block_magic_t magic = { { 'l', 'r', 'e', 's' } }; // Leaf REdis Set
        return magic;
    }

    block_size_t block_size() const { return block_size_; }

protected:
    // The block size.  It's convenient for leaf node code and for
    // some subclasses, too.
    block_size_t block_size_;
};

#endif	/* __SERVER_NESTED_DEMO_REDIS_SET_VALUES_HPP__ */
