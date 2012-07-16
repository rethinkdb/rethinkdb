#ifndef MEMCACHED_BTREE_NODE_HPP_
#define MEMCACHED_BTREE_NODE_HPP_

#include <string>

#include "btree/node.hpp"
#include "memcached/btree/value.hpp"

template <>
class value_sizer_t<memcached_value_t> : public value_sizer_t<void> {
public:
    explicit value_sizer_t<memcached_value_t>(block_size_t bs) : block_size_(bs) { }

    static const memcached_value_t *as_memcached(const void *p) {
        return reinterpret_cast<const memcached_value_t *>(p);
    }

    int size(const void *value) const {
        return as_memcached(value)->inline_size(block_size_);
    }

    bool fits(const void *value, int length_available) const {
        return btree_value_fits(block_size_, length_available, as_memcached(value));
    }

    bool deep_fsck(block_getter_t *getter, const void *value, int length_available, std::string *msg_out) const {
        if (!fits(value, length_available)) {
            *msg_out = "value does not fit in length_available";
            return false;
        }

        return blob::deep_fsck(getter, block_size_, as_memcached(value)->value_ref(), blob::btree_maxreflen, msg_out);
    }

    int max_possible_size() const {
        return MAX_MEMCACHED_VALUE_SIZE;
    }

    static block_magic_t leaf_magic() {
        block_magic_t magic = { { 'l', 'e', 'a', 'f' } };
        return magic;
    }

    block_magic_t btree_leaf_magic() const {
        return leaf_magic();
    }

    block_size_t block_size() const { return block_size_; }

private:
    // The block size.  It's convenient for leaf node code and for
    // some subclasses, too.
    block_size_t block_size_;

    DISABLE_COPYING(value_sizer_t<memcached_value_t>);
};

typedef value_sizer_t<memcached_value_t> memcached_value_sizer_t;

inline void valuecpy(block_size_t bs, memcached_value_t *dest, const memcached_value_t *src) {
    memcpy(dest, src, src->inline_size(bs));
}

#endif /* MEMCACHED_BTREE_NODE_HPP_ */
