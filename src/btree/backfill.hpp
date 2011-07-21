#ifndef __BTREE_BACKFILL_HPP__
#define __BTREE_BACKFILL_HPP__

#include "utils.hpp"
#include "store.hpp"

#include "buffer_cache/buffer_cache.hpp"

// Run backfilling at a reduced priority
#define BACKFILL_CACHE_PRIORITY 10

class btree_slice_t;

struct backfill_atom_t {
    store_key_t key;
    boost::shared_ptr<data_provider_t> value;
    mcflags_t flags;
    exptime_t exptime;
    repli_timestamp_t recency;
    cas_t cas_or_zero;
};

// How to use this class: Send deletion_key calls first, then call
// done_deletion_keys, then send on_keyvalues, then send done().
class backfill_callback_t {
public:
    virtual void on_keyvalue(backfill_atom_t atom) = 0;
    virtual void done_backfill() = 0;
protected:
    virtual ~backfill_callback_t() { }
};

/* `btree_backfill()` is guaranteed to find all changes whose timestamps are greater than
or equal than `since_when` but which reached the tree before `btree_backfill()` was called.
It may also find changes that happened before `since_when`. */

void btree_backfill(btree_slice_t *slice, repli_timestamp_t since_when, boost::shared_ptr<cache_account_t> backfill_account, backfill_callback_t *callback, order_token_t token);


#endif  // __BTREE_BACKFILL_HPP__
