#ifndef __BUFFER_CACHE_TYPES_HPP__
#define __BUFFER_CACHE_TYPES_HPP__

#include <stdint.h>

typedef uint32_t block_magic_comparison_t;

struct block_magic_t {
    char bytes[sizeof(block_magic_comparison_t)];

    bool operator==(const block_magic_t& other) const {
        union {
            block_magic_t x;
            block_magic_comparison_t n;
        } u, v;

        u.x = *this;
        v.x = other;

        return u.n == v.n;
    }

    bool operator!=(const block_magic_t& other) const {
        return !(operator==(other));
    }
};

// HEY: put this somewhere else.
class get_subtree_recencies_callback_t {
public:
    virtual void got_subtree_recencies() = 0;
protected:
    virtual ~get_subtree_recencies_callback_t() { }
};


#endif /* __BUFFER_CACHE_TYPES_HPP__ */
