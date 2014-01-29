// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef MEMCACHED_MEMCACHED_BTREE_BACKFILL_HPP_
#define MEMCACHED_MEMCACHED_BTREE_BACKFILL_HPP_

#include "btree/keys.hpp"
#include "containers/data_buffer.hpp"
#include "repli_timestamp.hpp"
#include "memcached/queries.hpp"

class btree_slice_t;
class parallel_traversal_progress_t;
class printf_buffer_t;
class superblock_t;

struct backfill_atom_t {
    store_key_t key;
    counted_t<data_buffer_t> value;
    mcflags_t flags;
    exptime_t exptime;
    repli_timestamp_t recency;
    cas_t cas_or_zero;

    backfill_atom_t() { }
    backfill_atom_t(const store_key_t& _key,
                    const counted_t<data_buffer_t>& _value,
                    const mcflags_t& _flags,
                    const exptime_t& _exptime,
                    const repli_timestamp_t& _recency,
                    const cas_t& _cas_or_zero) :
        key(_key),
        value(_value),
        flags(_flags),
        exptime(_exptime),
        recency(_recency),
        cas_or_zero(_cas_or_zero) { }
};

void debug_print(printf_buffer_t *buf, const backfill_atom_t& atom);


// How to use this class: Send on_delete_range calls before
// on_keyvalue calls for keys within that range.
class backfill_callback_t {
public:
    virtual void on_delete_range(const key_range_t &range, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;
    virtual void on_deletion(const btree_key_t *key, repli_timestamp_t recency, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;
    virtual void on_keyvalue(const backfill_atom_t& atom, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;
protected:
    virtual ~backfill_callback_t() { }
};

void memcached_backfill(btree_slice_t *slice,
                        const key_range_t& key_range,
                        repli_timestamp_t since_when,
                        backfill_callback_t *callback,
                        transaction_t *txn,
                        superblock_t *superblock,
                        buf_lock_t *sindex_block,
                        parallel_traversal_progress_t *p,
                        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

#endif /* MEMCACHED_MEMCACHED_BTREE_BACKFILL_HPP_ */
