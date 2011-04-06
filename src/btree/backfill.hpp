#ifndef __BTREE_BACKFILL_HPP__
#define __BTREE_BACKFILL_HPP__

#include "utils2.hpp"
#include "store.hpp"
#include "replication/delete_queue.hpp"

class btree_slice_t;

struct backfill_atom_t {
    store_key_t key;
    unique_ptr_t<data_provider_t> value;
    mcflags_t flags;
    exptime_t exptime;
    repli_timestamp recency;
    cas_t cas_or_zero;
};

// How to use this class: Send deletion_key calls first, then call
// done_deletion_keys, then send on_keyvalues, then send done().
class backfill_callback_t : public replication::deletion_key_stream_receiver_t {
public:
    virtual void add_dual_backfiller_hold() = 0;
    virtual void on_keyvalue(backfill_atom_t atom) = 0;
    virtual void done(repli_timestamp oper_start_timestamp) = 0;
protected:
    virtual ~backfill_callback_t() { }
};

void btree_backfill(btree_slice_t *slice, repli_timestamp since_when, backfill_callback_t *callback);



#endif  // __BTREE_BACKFILL_HPP__
