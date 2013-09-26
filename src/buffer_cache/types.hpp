// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef BUFFER_CACHE_TYPES_HPP_
#define BUFFER_CACHE_TYPES_HPP_

#include <limits.h>
#include <stdint.h>

#include "containers/archive/archive.hpp"
#include "serializer/types.hpp"

// WRITE_DURABILITY_INVALID is an invalid value, notably it can't be serialized.
enum write_durability_t { WRITE_DURABILITY_INVALID, WRITE_DURABILITY_SOFT, WRITE_DURABILITY_HARD };
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(write_durability_t, int8_t, WRITE_DURABILITY_SOFT, WRITE_DURABILITY_HARD);


enum buffer_cache_order_mode_t {
    buffer_cache_order_mode_check,
    buffer_cache_order_mode_ignore
};

struct eviction_priority_t {
    int priority;
};

const eviction_priority_t INITIAL_ROOT_EVICTION_PRIORITY = { 100 };
const eviction_priority_t DEFAULT_EVICTION_PRIORITY = { INT_MAX / 2 };
// TODO: Get rid of FAKE_EVICTION_PRIORITY.  It's just the default
// eviction priority, with the connotation the code using is is doing
// something stupid and needs to be fixed.
const eviction_priority_t FAKE_EVICTION_PRIORITY = { INT_MAX / 2 };

const eviction_priority_t ZERO_EVICTION_PRIORITY = { 0 };

inline eviction_priority_t incr_priority(eviction_priority_t p) {
    eviction_priority_t ret;
    ret.priority = p.priority + (p.priority < DEFAULT_EVICTION_PRIORITY.priority);
    return ret;
}

inline eviction_priority_t decr_priority(eviction_priority_t p) {
    eviction_priority_t ret;
    ret.priority = p.priority - (p.priority > 0);
    return ret;
}

inline bool operator==(eviction_priority_t x, eviction_priority_t y) {
    return x.priority == y.priority;
}

inline bool operator<(eviction_priority_t x, eviction_priority_t y) {
    return x.priority < y.priority;
}

typedef uint32_t block_magic_comparison_t;

struct block_magic_t {
    char bytes[sizeof(block_magic_comparison_t)];

    bool operator==(const block_magic_t &other) const {
        union {
            block_magic_t x;
            block_magic_comparison_t n;
        } u, v;

        u.x = *this;
        v.x = other;

        return u.n == v.n;
    }

    bool operator!=(const block_magic_t &other) const {
        return !(operator==(other));
    }
};

void debug_print(printf_buffer_t *buf, block_magic_t magic);

// HEY: put this somewhere else.
class get_subtree_recencies_callback_t {
public:
    virtual void got_subtree_recencies() = 0;
protected:
    virtual ~get_subtree_recencies_callback_t() { }
};

template <class T> class scoped_malloc_t;



// Keep this part below synced up with buffer_cache.hpp.

class mc_cache_t;
class mc_buf_lock_t;
class mc_transaction_t;
class mc_cache_account_t;

#if !defined(VALGRIND) && !defined(NDEBUG)

template <class inner_cache_type> class scc_cache_t;
template <class inner_cache_type> class scc_buf_lock_t;
template <class inner_cache_type> class scc_transaction_t;

typedef scc_cache_t<mc_cache_t> cache_t;
typedef scc_buf_lock_t<mc_cache_t> buf_lock_t;
typedef scc_transaction_t<mc_cache_t> transaction_t;
typedef mc_cache_account_t cache_account_t;

#else  // !defined(VALGRIND) && !defined(NDEBUG)

// scc_cache_t is way too slow under valgrind and makes automated
// tests run forever.
typedef mc_cache_t cache_t;
typedef mc_buf_lock_t buf_lock_t;
typedef mc_transaction_t transaction_t;
typedef mc_cache_account_t cache_account_t;

#endif  // !defined(VALGRIND) && !defined(NDEBUG)


#endif /* BUFFER_CACHE_TYPES_HPP_ */
