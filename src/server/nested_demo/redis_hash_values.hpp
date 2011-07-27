#ifndef __SERVER_NESTED_DEMO_REDIS_HASH_VALUES_HPP__
#define	__SERVER_NESTED_DEMO_REDIS_HASH_VALUES_HPP__

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include "server/nested_demo/redis_utils.hpp"
#include "btree/iteration.hpp"

struct redis_hash_value_t {
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

    boost::optional<std::string> hget(value_sizer_t<redis_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &field) const;

    bool hexists(value_sizer_t<redis_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &field) const;

    int hlen() const;

    // In contrast to the redis command, this deletes only a single field. Call multiple times if necessary.
    // returns true if a field has been deleted (i.e. existed in the hash)
    bool hdel(value_sizer_t<redis_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &field);

    // Returns true iff the field was new
    bool hset(value_sizer_t<redis_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &field, const std::string &value);

    // Deletes all elements from the subtree. Must be called before deleting the hash value in the super tree!
    void clear(value_sizer_t<redis_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, int slice_home_thread);

    // Provides an iterator over field->value pairs
    boost::shared_ptr<one_way_iterator_t<std::pair<std::string, std::string> > > hgetall(value_sizer_t<redis_hash_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, int slice_home_thread) const;


private:
    // Transforms from key_value_pair_t<redis_nested_string_value_t> to pairs of key and value for
    // cleaner outside-facing iterator interface
    std::pair<std::string, std::string> transform_value(const key_value_pair_t<redis_nested_string_value_t> &pair) const {
        const redis_nested_string_value_t *value = reinterpret_cast<redis_nested_string_value_t*>(pair.value.get());
        return std::pair<std::string, std::string>(pair.key, std::string(value->contents, value->length));
    }
} __attribute__((__packed__));

template <>
class value_sizer_t<redis_hash_value_t> {
public:
    value_sizer_t<redis_hash_value_t>(block_size_t bs) : block_size_(bs) { }

    int size(const redis_hash_value_t *value) const {
        return value->inline_size(block_size_);
    }

    bool fits(UNUSED const redis_hash_value_t *value, UNUSED int length_available) const {
        // It's of constant size...
        return true;
    }

    int max_possible_size() const {
        // It's of constant size...
        return sizeof(redis_hash_value_t);
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

#endif	/* __SERVER_NESTED_DEMO_REDIS_HASH_VALUES_HPP__ */
