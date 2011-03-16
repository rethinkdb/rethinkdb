#ifndef __BTREE_BACKFILL_HPP__
#define __BTREE_BACKFILL_HPP__

#include "utils2.hpp"
#include "store.hpp"

class btree_slice_t;

struct backfill_atom_t {
    store_key_t key;
    unique_ptr_t<data_provider_t> value;
    mcflags_t flags;
    exptime_t exptime;
    repli_timestamp recency;
    cas_t cas_or_zero;
};

class backfill_callback_t {
public:
    virtual void on_keyvalue(backfill_atom_t atom) = 0;
    virtual void done() = 0;
protected:
    ~backfill_callback_t() { }
};

void spawn_btree_backfill(btree_slice_t *slice, repli_timestamp since_when, backfill_callback_t *callback);



#endif  // __BTREE_BACKFILL_HPP__
