#ifndef __SERVER_NESTED_DEMO_REDIS_SET_VALUES_HPP__
#define	__SERVER_NESTED_DEMO_REDIS_SET_VALUES_HPP__

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include "server/nested_demo/redis_utils.hpp"
#include "btree/iteration.hpp"

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
     TODO!
     
     Please note that set operations which store the result into a destination set
     must not simultaneously hold a lock on both redis_set_value_t keys in the
     main b-tree, as that might cause deadlocks. You either have to be extremely
     careful, or just generate the resulting set in memory first and then store
     it in a second operation. Alternatively, if the destination set is new,
     you can create the destination redis_set_value_t without actually inserting
     it into the main btree yet. Once all result members are in the destination
     set, you release the input sets and actually put the destination set into
     the main btree. Be careful however to use the same write transaction for that,
     as you would get problems with flushes writing the nested btree nodes, before
     there actually is a way to find it through the main btree.

     Actually, there are interesting problems about locking anyway for all those
     operations that involve multiple sets... Those probably have to take a
     snapshot, to ensure deadlock safety.
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

    bool fits(UNUSED const redis_set_value_t *value, UNUSED int length_available) const {
        // It's of constant size...
        return true;
    }

    int max_possible_size() const {
        // It's of constant size...
        return sizeof(redis_set_value_t);
    }

    block_magic_t btree_leaf_magic() const {
        block_magic_t magic = { { 'l', 'e', 'a', 'f' } }; // TODO!
        return magic;
    }

    block_size_t block_size() const { return block_size_; }

protected:
    // The block size.  It's convenient for leaf node code and for
    // some subclasses, too.
    block_size_t block_size_;
};

#endif	/* __SERVER_NESTED_DEMO_REDIS_SET_VALUES_HPP__ */
