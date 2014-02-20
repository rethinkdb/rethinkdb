// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "memcached/memcached_btree/backfill.hpp"

#include "btree/backfill.hpp"
#include "btree/operations.hpp"
#include "btree/parallel_traversal.hpp"
#include "buffer_cache/alt/alt.hpp"
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

    void on_pairs(buf_parent_t parent, const std::vector<repli_timestamp_t> &recencies,
                  const std::vector<const btree_key_t *> &keys,
                  const std::vector<const void *> &vals,
                  signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {

        std::vector<backfill_atom_t> chunk_atoms;
        chunk_atoms.reserve(keys.size());
        size_t current_chunk_size = 0;

        for (size_t i = 0; i < keys.size(); ++i) {
            rassert(kr_.contains_key(keys[i]->contents, keys[i]->size));
            const memcached_value_t *value = static_cast<const memcached_value_t *>(vals[i]);
            counted_t<data_buffer_t> data_provider = value_to_data_buffer(value, parent);

            chunk_atoms.resize(i+1);
            chunk_atoms[i].key.assign(keys[i]->size, keys[i]->contents);
            chunk_atoms[i].value = data_provider;
            chunk_atoms[i].flags = value->mcflags();
            chunk_atoms[i].exptime = value->exptime();
            chunk_atoms[i].recency = recencies[i];
            chunk_atoms[i].cas_or_zero = value->has_cas() ? value->cas() : 0;
            // We only count the variably sized fields `key` and `value`.
            // But that is ok, we don't have to comply with BACKFILL_MAX_KVPAIRS_SIZE
            // that strictly.
            current_chunk_size += static_cast<size_t>(chunk_atoms[i].key.size())
                                  + static_cast<size_t>(chunk_atoms[i].value->size());

            if (current_chunk_size >= BACKFILL_MAX_KVPAIRS_SIZE) {
                // To avoid flooding the receiving node with overly large chunks
                // (which could easily make it run out of memory in extreme
                // cases), pass on what we have got so far. Then continue
                // with the remaining values.
                cb_->on_keyvalues(std::move(chunk_atoms), interruptor);
                chunk_atoms = std::vector<backfill_atom_t>();
                chunk_atoms.reserve(keys.size() - (i+1));
                current_chunk_size = 0;
            }
        }
        if (!chunk_atoms.empty()) {
            // Pass on the final chunk
            cb_->on_keyvalues(std::move(chunk_atoms), interruptor);
        }
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

void memcached_backfill(const key_range_t& key_range,
                        repli_timestamp_t since_when, backfill_callback_t *callback,
                        superblock_t *superblock,
                        buf_lock_t *sindex_block,
                        parallel_traversal_progress_t *p,
                        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    agnostic_memcached_backfill_callback_t agnostic_cb(callback, key_range);
    value_sizer_t<memcached_value_t> sizer(superblock->cache()->get_block_size());
    do_agnostic_btree_backfill(&sizer, key_range, since_when,
                               &agnostic_cb, superblock, sindex_block, p,
                               interruptor);
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
