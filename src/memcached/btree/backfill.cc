#include "memcached/btree/backfill.hpp"

#include "btree/parallel_traversal.hpp"
#include "memcached/btree/btree_data_provider.hpp"
#include "memcached/btree/node.hpp"
#include "memcached/btree/value.hpp"

class agnostic_memcached_backfill_callback_t : public agnostic_backfill_callback_t {
public:
    explicit agnostic_memcached_backfill_callback_t(backfill_callback_t *cb) : cb_(cb) { }

    void on_delete_range(const btree_key_t *low, const btree_key_t *high) {
        cb_->on_delete_range(low, high);
    }

    void on_deletion(const btree_key_t *key, repli_timestamp_t recency) {
        cb_->on_deletion(key, recency);
    }

    void on_pair(transaction_t *txn, repli_timestamp_t recency, const btree_key_t *key, const void *val) {
        const memcached_value_t *value = static_cast<const memcached_value_t *>(val);
        boost::intrusive_ptr<data_buffer_t> data_provider = value_to_data_buffer(value, txn);
        backfill_atom_t atom;
        atom.key.assign(key->size, key->contents);
        atom.value = data_provider;
        atom.flags = value->mcflags();
        atom.exptime = value->exptime();
        atom.recency = recency;
        atom.cas_or_zero = value->has_cas() ? value->cas() : 0;
        cb_->on_keyvalue(atom);
    }

    backfill_callback_t *cb_;
};

void memcached_backfill(btree_slice_t *slice, const key_range_t& key_range, repli_timestamp_t since_when, backfill_callback_t *callback,
                    transaction_t *txn, got_superblock_t& superblock, traversal_progress_t *p) {
    agnostic_memcached_backfill_callback_t agnostic_cb(callback);
    value_sizer_t<memcached_value_t> sizer(slice->cache()->get_block_size());
    do_agnostic_btree_backfill(&sizer, slice, key_range, since_when, &agnostic_cb, txn, superblock, p);
}
