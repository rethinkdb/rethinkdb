#ifndef __BUFFER_CACHE_TYPES_HPP__
#define __BUFFER_CACHE_TYPES_HPP__

#include <stdint.h>

#include "serializer/types.hpp"

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

template <class T> class scoped_malloc;

// HEY: This is kind of fsck-specific, maybe it belongs somewhere else.
class block_getter_t {
public:
    virtual bool get_block(block_id_t, scoped_malloc<char>& block_out) = 0;
protected:
    virtual ~block_getter_t() { }
};


// This line is hereby labeled BLAH.

// Keep this part below synced up with buffer_cache.hpp.

#ifndef MOCK_CACHE_CHECK

class mc_cache_t;
class mc_buf_t;
class mc_transaction_t;
class mc_cache_account_t;

#if !defined(VALGRIND) && !defined(NDEBUG)

template <class inner_cache_type> class scc_cache_t;
template <class inner_cache_type> class scc_buf_t;
template <class inner_cache_type> class scc_transaction_t;

typedef scc_cache_t<mc_cache_t> cache_t;
typedef scc_buf_t<mc_cache_t> buf_t;
typedef scc_transaction_t<mc_cache_t> transaction_t;
typedef mc_cache_account_t cache_account_t;

#else

// scc_cache_t is way too slow under valgrind and makes automated
// tests run forever.
typedef mc_cache_t cache_t;
typedef mc_buf_t buf_t;
typedef mc_transaction_t transaction_t;
typedef mc_cache_account_t cache_account_t;

#endif  // !defined(VALGRIND) && !defined(NDEBUG)

#else

class mock_cache_t;
template <class inner_cache_type> class scc_cache_t;
template <class inner_cache_type> class scc_buf_t;
template <class inner_cache_type> class scc_transaction_t;
class mock_cache_account_t;

typedef scc_cache_t<mock_cache_t> cache_t;
typedef scc_buf_t<mock_cache_t> buf_t;
typedef scc_transaction_t<mock_cache_t> transaction_t;
typedef mock_cache_account_t cache_account_t;

#endif // MOCK_CACHE_CHECK

// Don't put anything down here, put it above the line labeled "BLAH".

#endif /* __BUFFER_CACHE_TYPES_HPP__ */
