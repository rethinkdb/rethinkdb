// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef BUFFER_CACHE_TYPES_HPP_
#define BUFFER_CACHE_TYPES_HPP_

#include <limits.h>
#include <stdint.h>

#include "concurrency/cond_var.hpp"
#include "containers/archive/archive.hpp"
#include "serializer/types.hpp"

// WRITE_DURABILITY_INVALID is an invalid value, notably it can't be serialized.
enum write_durability_t { WRITE_DURABILITY_INVALID, WRITE_DURABILITY_SOFT, WRITE_DURABILITY_HARD };
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(write_durability_t, int8_t, WRITE_DURABILITY_SOFT, WRITE_DURABILITY_HARD);

// sync_callback_t is a cond_t that gets pulsed when on_sync is called
// sufficiently many times.  It is incorrect to call it more than that many
// times.  It is incorrect to call it less than that many times (because you'll
// block a waiter indefinitely).  The default number of times on_sync must be
// called is 1.  The number can be increased.  Every call to on_sync decreases
// the number by 1.  When the number reaches 0, cond_t::pulse() is called and
// waiters will be signaled.
//
// Some operations need to wait on multiple disk acks, you see.
//
// For many operations that take a sync_callback_t, specifying a null
// sync_callback_t * will mean "don't try to quickly sync this operation to
// disk."  Whether you pass a null or non-null sync_callback_t * has an actual
// effect on the writeback's behavior.
class sync_callback_t : private cond_t, public intrusive_list_node_t<sync_callback_t> {
    // We privately inherit from cond_t so that nobody can call pulse() or other
    // unrighteous functions.
public:
    sync_callback_t() : necessary_sync_count_(1) {}

    // Waits for all the on_syncs!  Calls wait().
    ~sync_callback_t();

    // This is called when the data has been properly written (and fdatasynced) to disk.
    void on_sync();

    // Remember that necessary_sync_count starts at 1, so if you want to sync on
    // N things (instead of just 1), you probably want to increase the sync
    // count by N-1.
    void increase_necessary_sync_count(size_t amount);

    using cond_t::wait;

    // Needed by some things like calls to wait_interruptable, because cond_t is
    // privately inherited.  This is perfectly safe, because you can't pulse a
    // signal_t, and certainly not a const signal_t.
    const signal_t *as_signal() const { return this; }

private:
    size_t necessary_sync_count_;
};


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

template <class T> class scoped_malloc_t;

// HEY: This is kind of fsck-specific, maybe it belongs somewhere else.
class block_getter_t {
public:
    virtual bool get_block(block_id_t, scoped_malloc_t<char> *block_out) = 0;
protected:
    virtual ~block_getter_t() { }
};



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
