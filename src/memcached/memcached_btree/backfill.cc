// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "memcached/memcached_btree/backfill.hpp"

#include "btree/backfill.hpp"
#include "btree/parallel_traversal.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "containers/printf_buffer.hpp"
#include "memcached/memcached_btree/btree_data_provider.hpp"
#include "memcached/memcached_btree/node.hpp"
#include "memcached/memcached_btree/value.hpp"

class agnostic_memcached_backfill_callback_t : public agnostic_backfill_callback_t {
public:
    agnostic_memcached_backfill_callback_t(backfill_callback_t *cb, const key_range_t &kr) : cb_(cb), kr_(kr) { }

    void on_delete_range(const key_range_t &range, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        rassert(kr_.is_superset(range));
        cb_->on_delete_range(range, interruptor);
    }

    void on_deletion(const btree_key_t *key, repli_timestamp_t recency, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        rassert(kr_.contains_key(key->contents, key->size));
        cb_->on_deletion(key, recency, interruptor);
    }

    void on_pair(transaction_t *txn, repli_timestamp_t recency, const btree_key_t *key, const void *val, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        rassert(kr_.contains_key(key->contents, key->size));
        const memcached_value_t *value = static_cast<const memcached_value_t *>(val);
        counted_t<data_buffer_t> data_provider = value_to_data_buffer(value, txn);
        backfill_atom_t atom;
        atom.key.assign(key->size, key->contents);
        atom.value = data_provider;
        atom.flags = value->mcflags();
        atom.exptime = value->exptime();
        atom.recency = recency;
        atom.cas_or_zero = value->has_cas() ? value->cas() : 0;
        cb_->on_keyvalue(atom, interruptor);
    }

    void on_sindexes(const std::map<std::string, secondary_index_t> &, signal_t *) THROWS_ONLY(interrupted_exc_t) {
        //Not implemented.
        //Right now this is a noop because memcached doesn't support secondary
        //indexes. We don't crash here because we still want the memcached unit
        //tests to work.
    }

    backfill_callback_t *cb_;
    key_range_t kr_;
};

void memcached_backfill(btree_slice_t *slice, const key_range_t& key_range, repli_timestamp_t since_when, backfill_callback_t *callback,
                    transaction_t *txn, superblock_t *superblock, buf_lock_t *sindex_block, parallel_traversal_progress_t *p,
                    signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    agnostic_memcached_backfill_callback_t agnostic_cb(callback, key_range);
    value_sizer_t<memcached_value_t> sizer(slice->cache()->get_block_size());
    do_agnostic_btree_backfill(&sizer, slice, key_range, since_when, &agnostic_cb, txn, superblock, sindex_block, p, interruptor);
}


void debug_print(printf_buffer_t *buf, const backfill_atom_t& atom) {
    buf->appendf("bf_atom{key=");
    debug_print(buf, atom.key);
    buf->appendf(", value=");
    debug_print(buf, atom.value);
    buf->appendf(", recency=");
    debug_print(buf, atom.recency);
    buf->appendf(", ...}");
}
