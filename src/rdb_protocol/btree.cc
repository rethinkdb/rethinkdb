// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/btree.hpp"

#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/variant.hpp>

#include "btree/backfill.hpp"
#include "btree/concurrent_traversal.hpp"
#include "btree/erase_range.hpp"
#include "btree/get_distribution.hpp"
#include "btree/operations.hpp"
#include "btree/parallel_traversal.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/buffer_group_stream.hpp"
#include "containers/archive/vector_stream.hpp"
#include "containers/scoped.hpp"
#include "rdb_protocol/blob_wrapper.hpp"

#include "rdb_protocol/func.hpp"
#include "rdb_protocol/lazy_json.hpp"
#include "rdb_protocol/transform_visitors.hpp"

value_sizer_t<rdb_value_t>::value_sizer_t(block_size_t bs) : block_size_(bs) { }

const rdb_value_t *value_sizer_t<rdb_value_t>::as_rdb(const void *p) {
    return reinterpret_cast<const rdb_value_t *>(p);
}

int value_sizer_t<rdb_value_t>::size(const void *value) const {
    return as_rdb(value)->inline_size(block_size_);
}

bool value_sizer_t<rdb_value_t>::fits(const void *value, int length_available) const {
    return btree_value_fits(block_size_, length_available, as_rdb(value));
}

int value_sizer_t<rdb_value_t>::max_possible_size() const {
    return blob::btree_maxreflen;
}

block_magic_t value_sizer_t<rdb_value_t>::leaf_magic() {
    block_magic_t magic = { { 'r', 'd', 'b', 'l' } };
    return magic;
}

block_magic_t value_sizer_t<rdb_value_t>::btree_leaf_magic() const {
    return leaf_magic();
}

block_size_t value_sizer_t<rdb_value_t>::block_size() const { return block_size_; }

bool btree_value_fits(block_size_t bs, int data_length, const rdb_value_t *value) {
    return blob::ref_fits(bs, data_length, value->value_ref(), blob::btree_maxreflen);
}

void rdb_get(const store_key_t &store_key, btree_slice_t *slice, transaction_t *txn, superblock_t *superblock, point_read_response_t *response) {
    keyvalue_location_t<rdb_value_t> kv_location;
    find_keyvalue_location_for_read(txn, superblock, store_key.btree_key(), &kv_location, slice->root_eviction_priority, &slice->stats);

    if (!kv_location.value.has()) {
        response->data.reset(new ql::datum_t(ql::datum_t::R_NULL));
    } else {
        response->data = get_data(kv_location.value.get(), txn);
    }
}

void kv_location_delete(keyvalue_location_t<rdb_value_t> *kv_location, const store_key_t &key,
                        btree_slice_t *slice, repli_timestamp_t timestamp, transaction_t *txn,
                        rdb_modification_info_t *mod_info_out) {
    guarantee(kv_location->value.has());


    if (mod_info_out) {
        guarantee(mod_info_out->deleted.second.empty());

        block_size_t block_size = txn->get_cache()->get_block_size();
        mod_info_out->deleted.second.assign(kv_location->value->value_ref(),
            kv_location->value->value_ref() +
                kv_location->value->inline_size(block_size));
    }

    kv_location->value.reset();
    null_key_modification_callback_t<rdb_value_t> null_cb;
    apply_keyvalue_change(txn, kv_location, key.btree_key(), timestamp, false, &null_cb, &slice->root_eviction_priority);
}

void kv_location_set(keyvalue_location_t<rdb_value_t> *kv_location, const store_key_t &key,
                     counted_t<const ql::datum_t> data,
                     btree_slice_t *slice, repli_timestamp_t timestamp, transaction_t *txn,
                     rdb_modification_info_t *mod_info_out) {
    scoped_malloc_t<rdb_value_t> new_value(blob::btree_maxreflen);
    bzero(new_value.get(), blob::btree_maxreflen);

    // TODO unnecessary copies they must go away.
    write_message_t wm;
    wm << data;
    vector_stream_t stream;
    int res = send_write_message(&stream, &wm);
    guarantee_err(res == 0, "Serialization for json data failed... this shouldn't happen.\n");

    // TODO more copies, good lord
    std::string sered_data(stream.vector().begin(), stream.vector().end());

    rdb_blob_wrapper_t blob(txn->get_cache()->get_block_size(),
                new_value->value_ref(), blob::btree_maxreflen,
                txn, sered_data);

    block_size_t block_size = txn->get_cache()->get_block_size();
    if (mod_info_out) {
        guarantee(mod_info_out->added.second.empty());
        mod_info_out->added.second.assign(new_value->value_ref(),
            new_value->value_ref() + new_value->inline_size(block_size));
    }

    if (kv_location->value.has() && mod_info_out) {
        guarantee(mod_info_out->deleted.second.empty());
        mod_info_out->deleted.second.assign(kv_location->value->value_ref(),
            kv_location->value->value_ref() + kv_location->value->inline_size(block_size));
    }

    // Actually update the leaf, if needed.
    kv_location->value = std::move(new_value);
    null_key_modification_callback_t<rdb_value_t> null_cb;
    apply_keyvalue_change(txn, kv_location, key.btree_key(), timestamp, false, &null_cb, &slice->root_eviction_priority);
    //                                                                  ^^^^^ That means the key isn't expired.
}

void kv_location_set(keyvalue_location_t<rdb_value_t> *kv_location, const store_key_t &key,
                     const std::vector<char> &value_ref,
                     btree_slice_t *slice, repli_timestamp_t timestamp, transaction_t *txn) {
    scoped_malloc_t<rdb_value_t> new_value(
            value_ref.data(), value_ref.data() + value_ref.size());

    // Clear the blob in the leaf if it existed
    if (kv_location->value.has()) {
        blob_t old_blob(txn->get_cache()->get_block_size(),
                    kv_location->value->value_ref(), blob::btree_maxreflen);
        old_blob.clear(txn);
    }

    // Actually update the leaf, if needed.
    kv_location->value = std::move(new_value);
    null_key_modification_callback_t<rdb_value_t> null_cb;
    apply_keyvalue_change(txn, kv_location, key.btree_key(), timestamp, false, &null_cb, &slice->root_eviction_priority);
    //                                                                  ^^^^^ That means the key isn't expired.
}

// QL2 This implements UPDATE, REPLACE, and part of DELETE and INSERT (each is
// just a different function passed to this function).
void rdb_replace_and_return_superblock(
    btree_slice_t *slice,
    repli_timestamp_t timestamp,
    transaction_t *txn,
    superblock_t *superblock,
    const std::string &primary_key,
    const store_key_t &key,
    ql::map_wire_func_t *f,
    return_vals_t return_vals,
    ql::env_t *ql_env,
    promise_t<superblock_t *> *superblock_promise_or_null,
    Datum *response_out,
    rdb_modification_info_t *mod_info) THROWS_NOTHING {
    ql::datum_ptr_t resp(ql::datum_t::R_OBJECT);
    try {
        keyvalue_location_t<rdb_value_t> kv_location;
        find_keyvalue_location_for_write(
            txn, superblock, key.btree_key(), &kv_location,
            &slice->root_eviction_priority, &slice->stats, superblock_promise_or_null);

        bool started_empty, ended_empty;
        counted_t<const ql::datum_t> old_val;
        if (!kv_location.value.has()) {
            // If there's no entry with this key, pass NULL to the function.
            started_empty = true;
            old_val = make_counted<ql::datum_t>(ql::datum_t::R_NULL);
        } else {
            // Otherwise pass the entry with this key to the function.
            started_empty = false;
            old_val = get_data(kv_location.value.get(), txn);
            guarantee(old_val->get(primary_key, ql::NOTHROW).has());
        }
        guarantee(old_val.has());
        if (return_vals == RETURN_VALS) {
            bool conflict = resp.add("old_val", old_val)
                         || resp.add("new_val", old_val); // changed below
            guarantee(!conflict);
        }

        counted_t<const ql::datum_t> new_val
            = f->compile_wire_func()->call(ql_env, old_val)->as_datum();
        if (return_vals == RETURN_VALS) {
            bool conflict = resp.add("new_val", new_val, ql::CLOBBER);
            guarantee(conflict); // We set it to `old_val` previously.
        }
        if (new_val->get_type() == ql::datum_t::R_NULL) {
            ended_empty = true;
        } else if (new_val->get_type() == ql::datum_t::R_OBJECT) {
            ended_empty = false;
            counted_t<const ql::datum_t> pk = new_val->get(primary_key, ql::NOTHROW);
            rcheck_target(
                new_val, ql::base_exc_t::GENERIC,
                pk.has(),
                strprintf("Inserted object must have primary key `%s`:\n%s",
                          primary_key.c_str(), new_val->print().c_str()));
            rcheck_target(
                new_val, ql::base_exc_t::GENERIC,
                key.compare(store_key_t(pk->print_primary())) == 0,
                (started_empty
                 ? strprintf("Primary key `%s` cannot be changed (null -> %s)",
                             primary_key.c_str(), new_val->print().c_str())
                 : strprintf("Primary key `%s` cannot be changed (%s -> %s)",
                             primary_key.c_str(),
                             old_val->print().c_str(), new_val->print().c_str())));
        } else {
            rfail_typed_target(
                new_val, "Inserted value must be an OBJECT (got %s):\n%s",
                new_val->get_type_name().c_str(), new_val->print().c_str());
        }

        // We use `conflict` below to store whether or not there was a key
        // conflict when constructing the stats object.  It defaults to `true`
        // so that we fail an assertion if we never update the stats object.
        bool conflict = true;

        // Figure out what operation we're doing (based on started_empty,
        // ended_empty, and the result of the function call) and then do it.
        if (started_empty) {
            if (ended_empty) {
                conflict = resp.add("skipped", make_counted<ql::datum_t>(1.0));
            } else {
                conflict = resp.add("inserted", make_counted<ql::datum_t>(1.0));
                r_sanity_check(new_val->get(primary_key, ql::NOTHROW).has());
                kv_location_set(&kv_location, key, new_val,
                                slice, timestamp, txn,
                                mod_info);
                guarantee(mod_info->deleted.second.empty());
                guarantee(!mod_info->added.second.empty());
                mod_info->added.first = new_val;
            }
        } else {
            if (ended_empty) {
                conflict = resp.add("deleted", make_counted<ql::datum_t>(1.0));
                kv_location_delete(&kv_location, key, slice, timestamp, txn, mod_info);
                guarantee(!mod_info->deleted.second.empty());
                guarantee(mod_info->added.second.empty());
                mod_info->deleted.first = old_val;
            } else {
                r_sanity_check(*old_val->get(primary_key) == *new_val->get(primary_key));
                if (*old_val == *new_val) {
                    conflict = resp.add("unchanged",
                                         make_counted<ql::datum_t>(1.0));
                } else {
                    conflict = resp.add("replaced", make_counted<ql::datum_t>(1.0));
                    r_sanity_check(new_val->get(primary_key, ql::NOTHROW).has());
                    kv_location_set(&kv_location, key, new_val,
                                    slice, timestamp, txn, mod_info);
                    guarantee(!mod_info->deleted.second.empty());
                    guarantee(!mod_info->added.second.empty());
                    mod_info->added.first = new_val;
                    mod_info->deleted.first = old_val;
                }
            }
        }
        guarantee(!conflict); // message never added twice
    } catch (const ql::base_exc_t &e) {
        std::string msg = e.what();
        bool b = resp.add("errors", make_counted<ql::datum_t>(1.0))
            || resp.add("first_error", make_counted<ql::datum_t>(std::move(msg)));
        guarantee(!b);
    } catch (const interrupted_exc_t &e) {
        std::string msg = strprintf("interrupted (%s:%d)", __FILE__, __LINE__);
        bool b = resp.add("errors", make_counted<ql::datum_t>(1.0))
            || resp.add("first_error", make_counted<ql::datum_t>(std::move(msg)));
        guarantee(!b);
        // We don't rethrow because we're in a coroutine.  Theoretically the
        // above message should never make it back to a user because the calling
        // function will also be interrupted, but we document where it comes
        // from to aid in future debugging if that invariant becomes violated.
    }
    resp->write_to_protobuf(response_out);
}

void rdb_replace(btree_slice_t *slice,
                 repli_timestamp_t timestamp,
                 transaction_t *txn,
                 superblock_t *superblock,
                 const std::string &primary_key,
                 const store_key_t &key,
                 ql::map_wire_func_t *f,
                 return_vals_t return_vals,
                 ql::env_t *ql_env,
                 Datum *response_out,
                 rdb_modification_info_t *mod_info) {
    rdb_replace_and_return_superblock(
        slice, timestamp, txn, superblock, primary_key,
        key, f, return_vals, ql_env, NULL, response_out, mod_info);
}

struct slice_timestamp_txn_replace_t {
    slice_timestamp_txn_replace_t(btree_slice_t *_slice, repli_timestamp_t _timestamp,
                                  transaction_t *_txn, const point_replace_t *_replace)
        : slice(_slice), timestamp(_timestamp), txn(_txn), replace(_replace) { }

    btree_slice_t *slice;
    repli_timestamp_t timestamp;
    transaction_t *txn;
    const point_replace_t *replace;
};

void do_a_replace_from_batched_replace(auto_drainer_t::lock_t,
                                       fifo_enforcer_sink_t *batched_replaces_fifo_sink,
                                       const fifo_enforcer_write_token_t &batched_replaces_fifo_token,
                                       slice_timestamp_txn_replace_t sttr,
                                       superblock_t *superblock,
                                       ql::env_t *ql_env,
                                       promise_t<superblock_t *> *superblock_promise_or_null,
                                       Datum *response_out,
                                       rdb_modification_report_cb_t *sindex_cb) {
    fifo_enforcer_sink_t::exit_write_t exiter(batched_replaces_fifo_sink, batched_replaces_fifo_token);

    ql::map_wire_func_t f = sttr.replace->f;
    rdb_modification_report_t mod_report(sttr.replace->key);
    rdb_replace_and_return_superblock(sttr.slice, sttr.timestamp, sttr.txn, superblock,
                                      sttr.replace->primary_key, sttr.replace->key, &f,
                                      NO_RETURN_VALS, ql_env, superblock_promise_or_null,
                                      response_out, &mod_report.info);

    exiter.wait();
    sindex_cb->on_mod_report(mod_report);
}

// The int64_t in replaces is ignored -- that's used for preserving order
// through sharding/unsharding.  We're not about to repack a new vector just to
// call this function.
void rdb_batched_replace(const std::vector<std::pair<int64_t, point_replace_t> > &replaces,
                         btree_slice_t *slice, repli_timestamp_t timestamp,
                         transaction_t *txn, scoped_ptr_t<superblock_t> *superblock, ql::env_t *ql_env,
                         batched_replaces_response_t *response_out,
                         rdb_modification_report_cb_t *sindex_cb) {
    fifo_enforcer_source_t batched_replaces_fifo_source;
    fifo_enforcer_sink_t batched_replaces_fifo_sink;

    // Note the destructor ordering: We have to drain write operations before
    // destructing the batched_replaces_fifo_sink, because the coroutines being
    // drained use said fifo.
    auto_drainer_t drainer;

    // Note the destructor ordering: We release the superblock before draining on all the write operations.
    scoped_ptr_t<superblock_t> current_superblock(superblock->release());

    response_out->point_replace_responses.resize(replaces.size());
    for (size_t i = 0; i < replaces.size(); ++i) {
        // Pass out the int64_t for shard/unshard reordering.
        response_out->point_replace_responses[i].first = replaces[i].first;

        // Pass out the point_replace_response_t.
        promise_t<superblock_t *> superblock_promise;
        coro_t::spawn(boost::bind(&do_a_replace_from_batched_replace,
                                  auto_drainer_t::lock_t(&drainer),
                                  &batched_replaces_fifo_sink,
                                  batched_replaces_fifo_source.enter_write(),
                                  slice_timestamp_txn_replace_t(slice, timestamp, txn, &replaces[i].second),
                                  current_superblock.release(),
                                  ql_env,
                                  &superblock_promise,
                                  &response_out->point_replace_responses[i].second,
                                  sindex_cb));

        current_superblock.init(superblock_promise.wait());
    }
}

void rdb_set(const store_key_t &key, counted_t<const ql::datum_t> data, bool overwrite,
             btree_slice_t *slice, repli_timestamp_t timestamp,
             transaction_t *txn, superblock_t *superblock, point_write_response_t *response_out,
             rdb_modification_info_t *mod_info) {
    keyvalue_location_t<rdb_value_t> kv_location;
    find_keyvalue_location_for_write(txn, superblock, key.btree_key(), &kv_location,
                                     &slice->root_eviction_priority, &slice->stats);
    const bool had_value = kv_location.value.has();

    /* update the modification report */
    if (kv_location.value.has()) {
        mod_info->deleted.first = get_data(kv_location.value.get(), txn);
    }

    mod_info->added.first = data;

    if (overwrite || !had_value) {
        kv_location_set(&kv_location, key, data, slice, timestamp, txn, mod_info);
        guarantee(mod_info->deleted.second.empty() == !had_value &&
                  !mod_info->added.second.empty());
    }
    response_out->result = (had_value ? DUPLICATE : STORED);
}

class agnostic_rdb_backfill_callback_t : public agnostic_backfill_callback_t {
public:
    agnostic_rdb_backfill_callback_t(rdb_backfill_callback_t *cb, const key_range_t &kr) : cb_(cb), kr_(kr) { }

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
        const rdb_value_t *value = static_cast<const rdb_value_t *>(val);

        rdb_protocol_details::backfill_atom_t atom;
        atom.key.assign(key->size, key->contents);
        atom.value = get_data(value, txn);
        atom.recency = recency;
        cb_->on_keyvalue(atom, interruptor);
    }

    void on_sindexes(const std::map<std::string, secondary_index_t> &sindexes, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        cb_->on_sindexes(sindexes, interruptor);
    }

    rdb_backfill_callback_t *cb_;
    key_range_t kr_;
};

void rdb_backfill(btree_slice_t *slice, const key_range_t& key_range,
        repli_timestamp_t since_when, rdb_backfill_callback_t *callback,
        transaction_t *txn, superblock_t *superblock,
        buf_lock_t *sindex_block,
        parallel_traversal_progress_t *p, signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    agnostic_rdb_backfill_callback_t agnostic_cb(callback, key_range);
    value_sizer_t<rdb_value_t> sizer(slice->cache()->get_block_size());
    do_agnostic_btree_backfill(&sizer, slice, key_range, since_when, &agnostic_cb, txn, superblock, sindex_block, p, interruptor);
}

void rdb_delete(const store_key_t &key, btree_slice_t *slice,
                repli_timestamp_t timestamp, transaction_t *txn,
                superblock_t *superblock, point_delete_response_t *response,
                rdb_modification_info_t *mod_info) {
    keyvalue_location_t<rdb_value_t> kv_location;
    find_keyvalue_location_for_write(txn, superblock, key.btree_key(), &kv_location, &slice->root_eviction_priority, &slice->stats);
    bool exists = kv_location.value.has();

    /* Update the modification report. */
    if (exists) {
        mod_info->deleted.first = get_data(kv_location.value.get(), txn);
    }

    if (exists) kv_location_delete(&kv_location, key, slice, timestamp, txn, mod_info);
    guarantee(!mod_info->deleted.second.empty() && mod_info->added.second.empty());
    response->result = (exists ? DELETED : MISSING);
}

void rdb_value_deleter_t::delete_value(transaction_t *_txn, void *_value) {
    rdb_blob_wrapper_t blob(_txn->get_cache()->get_block_size(),
                static_cast<rdb_value_t *>(_value)->value_ref(), blob::btree_maxreflen);
    blob.clear(_txn);
}

void rdb_value_non_deleter_t::delete_value(transaction_t *, void *) { }

class sindex_key_range_tester_t : public key_tester_t {
public:
    explicit sindex_key_range_tester_t(const key_range_t &key_range)
        : key_range_(key_range) { }

    bool key_should_be_erased(const btree_key_t *key) {
        std::string pk = ql::datum_t::extract_primary(
            key_to_unescaped_str(store_key_t(key)));

        return key_range_.contains_key(store_key_t(pk));
    }
private:
    key_range_t key_range_;
};

typedef btree_store_t<rdb_protocol_t>::sindex_access_t sindex_access_t;
typedef btree_store_t<rdb_protocol_t>::sindex_access_vector_t sindex_access_vector_t;

void sindex_erase_range(const key_range_t &key_range,
        transaction_t *txn, const sindex_access_t *sindex_access, auto_drainer_t::lock_t,
        signal_t *interruptor, bool release_superblock) THROWS_NOTHING {

    value_sizer_t<rdb_value_t> rdb_sizer(sindex_access->btree->cache()->get_block_size());
    value_sizer_t<void> *sizer = &rdb_sizer;

    rdb_value_non_deleter_t deleter;

    sindex_key_range_tester_t tester(key_range);

    try {
        btree_erase_range_generic(sizer, sindex_access->btree, &tester,
                &deleter, NULL, NULL, txn, sindex_access->super_block.get(), interruptor, release_superblock);
    } catch (const interrupted_exc_t &) {
        // We were interrupted. That's fine nothing to be done about it.
    }
}

/* Spawns a coro to carry out the erase range for each sindex. */
void spawn_sindex_erase_ranges(
        const sindex_access_vector_t *sindex_access,
        const key_range_t &key_range,
        transaction_t *txn,
        auto_drainer_t *drainer,
        auto_drainer_t::lock_t,
        bool release_superblock,
        signal_t *interruptor) {
    for (auto it = sindex_access->begin(); it != sindex_access->end(); ++it) {
        coro_t::spawn_sometime(boost::bind(
                    &sindex_erase_range, key_range, txn, &*it,
                    auto_drainer_t::lock_t(drainer), interruptor,
                    release_superblock));
    }
}

void rdb_erase_range(btree_slice_t *slice, key_tester_t *tester,
                     const key_range_t &key_range,
                     transaction_t *txn, superblock_t *superblock,
                     btree_store_t<rdb_protocol_t> *store,
                     write_token_pair_t *token_pair,
                     signal_t *interruptor) {
    /* This is guaranteed because the way the keys are calculated below would
     * lead to a single key being deleted even if the range was empty. */
    guarantee(!key_range.is_empty());

    /* Dispatch the erase range to the sindexes. */
    sindex_access_vector_t sindex_superblocks;
    {
        scoped_ptr_t<buf_lock_t> sindex_block;
        store->acquire_sindex_block_for_write(
            token_pair, txn, &sindex_block, superblock->get_sindex_block_id(),
            interruptor);

        store->aquire_post_constructed_sindex_superblocks_for_write(
                sindex_block.get(), txn, &sindex_superblocks);

        mutex_t::acq_t acq;
        store->lock_sindex_queue(sindex_block.get(), &acq);

        write_message_t wm;
        wm << rdb_sindex_change_t(rdb_erase_range_report_t(key_range));
        store->sindex_queue_push(wm, &acq);
    }

    {
        auto_drainer_t sindex_erase_drainer;
        spawn_sindex_erase_ranges(&sindex_superblocks, key_range, txn,
                &sindex_erase_drainer, auto_drainer_t::lock_t(&sindex_erase_drainer),
                true, /* release the superblock */ interruptor);

        /* Notice, when we exit this block we destruct the sindex_erase_drainer
         * which means we'll wait until all of the sindex_erase_ranges finish
         * executing. This is an important detail because the sindexes only
         * store references to their data. They don't actually store a full
         * copy of the data themselves. The call to btree_erase_range_generic
         * is the one that will actually erase the data and if we were to make
         * that call before the indexes were finished erasing we would have
         * reference to data which didn't actually exist and another process
         * could read that data. */
        /* TL;DR it's very important that we make sure all of the coros spawned
         * by spawn_sindex_erase_ranges complete before we proceed past this
         * point. */
    }

    /* Twiddle some keys to get the in the form we want. Notice these are keys
     * which will be made  exclusive and inclusive as their names suggest
     * below. At the point of construction they aren't. */
    store_key_t left_key_exclusive(key_range.left);
    store_key_t right_key_inclusive(key_range.right.key);

    bool left_key_supplied = left_key_exclusive.decrement();
    bool right_key_supplied = !key_range.right.unbounded;
    if (right_key_supplied) {
        right_key_inclusive.decrement();
    }

    /* Now left_key_exclusive and right_key_inclusive accurately reflect their
     * names. */

    /* We need these structures to perform the erase range. */
    value_sizer_t<rdb_value_t> rdb_sizer(slice->cache()->get_block_size());
    value_sizer_t<void> *sizer = &rdb_sizer;

    rdb_value_deleter_t deleter;

    btree_erase_range_generic(sizer, slice, tester, &deleter,
        left_key_supplied ? left_key_exclusive.btree_key() : NULL,
        right_key_supplied ? right_key_inclusive.btree_key() : NULL,
        txn, superblock, interruptor);

    // auto_drainer_t is destructed here so this waits for other coros to finish.
}

// This is actually a kind of misleading name. This function estimates the size of a datum,
// not a whole rget, though it is used for that purpose (by summing up these responses).
size_t estimate_rget_response_size(const counted_t<const ql::datum_t> &datum) {
    return serialized_size(datum);
}

class rdb_rget_depth_first_traversal_callback_t
    : public concurrent_traversal_callback_t {
public:
    /* This constructor does a traversal on the primary btree, it's not to be
     * used with sindexes. The constructor below is for use with sindexes. */
    rdb_rget_depth_first_traversal_callback_t(
        transaction_t *txn,
        ql::env_t *_ql_env,
        const rdb_protocol_details::transform_t &_transform,
        boost::optional<rdb_protocol_details::terminal_t> _terminal,
        const key_range_t &range,
        sorting_t _sorting,
        rget_read_response_t *_response)
        : bad_init(false),
          transaction(txn),
          response(_response),
          cumulative_size(0),
          ql_env(_ql_env),
          transform(_transform),
          terminal(_terminal),
          sorting(_sorting)
    {
        init(range);
    }

    /* This constructor is used if you're doing a secondary index get, it takes
     * an extra key_range_t (_primary_key_range) which is used to filter out
     * unwanted results. The reason you can get unwanted results is is
     * oversharding. When we overshard multiple logical shards are stored in
     * the same physical btree_store_t, this is transparent with all other
     * operations but their sindex values get mixed together and you wind up
     * with multiple copies of each. This constructor will filter out the
     * duplicates. This was issue #606. */
    rdb_rget_depth_first_traversal_callback_t(
        transaction_t *txn,
        ql::env_t *_ql_env,
        const rdb_protocol_details::transform_t &_transform,
        boost::optional<rdb_protocol_details::terminal_t> _terminal,
        const key_range_t &range,
        const key_range_t &_primary_key_range,
        sorting_t _sorting,
        ql::map_wire_func_t _sindex_function,
        sindex_multi_bool_t _sindex_multi,
        sindex_range_t _sindex_range,
        rget_read_response_t *_response)
        : bad_init(false),
          transaction(txn),
          response(_response),
          cumulative_size(0),
          ql_env(_ql_env),
          transform(_transform),
          terminal(_terminal),
          sorting(_sorting),
          primary_key_range(_primary_key_range),
          sindex_range(_sindex_range),
          sindex_multi(_sindex_multi)
    {
        sindex_function = _sindex_function.compile_wire_func();
        init(range);
    }

    void init(const key_range_t &range) {
        try {
            if (forward(sorting)) {
                response->last_considered_key = range.left;
            } else {
                guarantee(backward(sorting));
                if (!range.right.unbounded) {
                    response->last_considered_key = range.right.key;
                } else {
                    response->last_considered_key = store_key_t::max();
                }
            }

            if (terminal) {
                query_language::terminal_initialize(&*terminal, &response->result);
            }
        } catch (const ql::exc_t &e2) {
            /* Evaluation threw so we're not going to be accepting any more requests. */
            response->result = e2;
            bad_init = true;
        } catch (const ql::datum_exc_t &e2) {
            /* Evaluation threw so we're not going to be accepting any more requests. */
            terminal_exception(e2, *terminal, &response->result);
            bad_init = true;
        }
    }

    bool handle_pair(scoped_key_value_t &&keyvalue,
                     concurrent_traversal_fifo_enforcer_signal_t waiter)
        THROWS_ONLY(interrupted_exc_t) {
        store_key_t store_key(keyvalue.key());
        if (bad_init) {
            return false;
        }
        if (primary_key_range) {
            std::string pk = ql::datum_t::extract_primary(
                    key_to_unescaped_str(store_key));
            if (!primary_key_range->contains_key(store_key_t(pk))) {
                return true;
            }
        }
        try {
            lazy_json_t first_value(static_cast<const rdb_value_t *>(keyvalue.value()),
                                    transaction);
            first_value.get();

            keyvalue.reset();

            waiter.wait_interruptible();

            if ((response->last_considered_key < store_key && forward(sorting)) ||
                (response->last_considered_key > store_key && backward(sorting))) {
                response->last_considered_key = store_key;
            }

            std::vector<lazy_json_t> data;
            data.push_back(first_value);

            counted_t<const ql::datum_t> sindex_value;
            if (sindex_function) {
                sindex_value = sindex_function->call(ql_env, first_value.get())->as_datum();
                guarantee(sindex_range);
                guarantee(sindex_multi);

                if (sindex_multi == MULTI &&
                    sindex_value->get_type() == ql::datum_t::R_ARRAY) {
                        boost::optional<uint64_t> tag = ql::datum_t::extract_tag(key_to_unescaped_str(store_key));
                        guarantee(tag);
                        guarantee(sindex_value->size() > tag);
                        sindex_value = sindex_value->get(*tag);
                }
                if (!sindex_range->contains(sindex_value)) {
                    return true;
                }
            }

            // Apply transforms to the data
            {
                rdb_protocol_details::transform_t::iterator it;
                for (it = transform.begin(); it != transform.end(); ++it) {
                    try {
                        std::vector<counted_t<const ql::datum_t> > tmp;

                        for (auto jt = data.begin(); jt != data.end(); ++jt) {
                            query_language::transform_apply(
                                ql_env, jt->get(), &*it, &tmp);
                        }
                        data.clear();
                        for (auto jt = tmp.begin(); jt != tmp.end(); ++jt) {
                            data.push_back(lazy_json_t(*jt));
                        }
                    } catch (const ql::datum_exc_t &e2) {
                        /* Evaluation threw so we're not going to be accepting any
                           more requests. */
                        transform_exception(e2, *it, &response->result);
                        return false;
                    }
                }
            }

            if (!terminal) {
                typedef rget_read_response_t::stream_t stream_t;
                stream_t *stream = boost::get<stream_t>(&response->result);
                guarantee(stream);
                for (auto it = data.begin(); it != data.end(); ++it) {
                    counted_t<const ql::datum_t> datum = it->get();
                    if (sorting != UNORDERED && sindex_value) {
                        stream->push_back(rdb_protocol_details::rget_item_t(
                                    store_key, sindex_value, datum));
                    } else {
                        stream->push_back(rdb_protocol_details::rget_item_t(
                                              store_key, datum));
                    }

                    cumulative_size += estimate_rget_response_size(datum);
                }
                return cumulative_size < rget_max_chunk_size;
            } else {
                try {
                    for (auto jt = data.begin(); jt != data.end(); ++jt) {
                        query_language::terminal_apply(
                            ql_env, *jt, &*terminal, &response->result);
                    }
                    return true;
                } catch (const ql::datum_exc_t &e2) {
                    /* Evaluation threw so we're not going to be accepting any
                       more requests. */
                    terminal_exception(e2, *terminal, &response->result);
                    return false;
                }
            }
        } catch (const ql::exc_t &e2) {
            /* Evaluation threw so we're not going to be accepting any more requests. */
            response->result = e2;
            return false;
        }

    }
    bool bad_init;
    transaction_t *transaction;
    rget_read_response_t *response;
    size_t cumulative_size;
    ql::env_t *ql_env;
    rdb_protocol_details::transform_t transform;
    boost::optional<rdb_protocol_details::terminal_t> terminal;
    sorting_t sorting;

    /* Only present if we're doing a sindex read.*/
    boost::optional<key_range_t> primary_key_range;
    boost::optional<sindex_range_t> sindex_range;
    counted_t<ql::func_t> sindex_function;
    boost::optional<sindex_multi_bool_t> sindex_multi;
};

class result_finalizer_visitor_t : public boost::static_visitor<void> {
public:
    void operator()(const rget_read_response_t::stream_t &) const { }
    void operator()(const ql::exc_t &) const { }
    void operator()(const ql::datum_exc_t &) const { }
    void operator()(const std::vector<ql::wire_datum_map_t> &) const { }
    void operator()(const rget_read_response_t::empty_t &) const { }
    void operator()(const rget_read_response_t::vec_t &) const { }
    void operator()(const counted_t<const ql::datum_t> &) const { }

    void operator()(ql::wire_datum_map_t &dm) const {  // NOLINT(runtime/references)
        dm.finalize();
    }
};

void rdb_rget_slice(btree_slice_t *slice, const key_range_t &range,
                    transaction_t *txn, superblock_t *superblock,
                    ql::env_t *ql_env,
                    const rdb_protocol_details::transform_t &transform,
                    const boost::optional<rdb_protocol_details::terminal_t> &terminal,
                    sorting_t sorting,
                    rget_read_response_t *response) {
    rdb_rget_depth_first_traversal_callback_t callback(
            txn, ql_env, transform, terminal, range, sorting, response);
    btree_concurrent_traversal(slice, txn, superblock, range, &callback,
            (forward(sorting) ? FORWARD : BACKWARD));

    if (callback.cumulative_size >= rget_max_chunk_size) {
        response->truncated = true;
    } else {
        response->truncated = false;
    }

    boost::apply_visitor(result_finalizer_visitor_t(), response->result);
}

void rdb_rget_secondary_slice(btree_slice_t *slice, const sindex_range_t &sindex_range,
                    transaction_t *txn, superblock_t *superblock, ql::env_t *ql_env,
                    const rdb_protocol_details::transform_t &transform,
                    const boost::optional<rdb_protocol_details::terminal_t> &terminal,
                    const key_range_t &pk_range,
                    sorting_t sorting,
                    const ql::map_wire_func_t &sindex_func,
                    sindex_multi_bool_t sindex_multi,
                    rget_read_response_t *response) {
    rdb_rget_depth_first_traversal_callback_t callback(txn, ql_env, transform, terminal,
            sindex_range.to_region().inner, pk_range,
            sorting, sindex_func, sindex_multi, sindex_range, response);

    btree_concurrent_traversal(slice, txn, superblock, sindex_range.to_region().inner,
            &callback, (forward(sorting) ? FORWARD : BACKWARD));

    if (callback.cumulative_size >= rget_max_chunk_size) {
        response->truncated = true;
    } else {
        response->truncated = false;
    }

    boost::apply_visitor(result_finalizer_visitor_t(), response->result);
}

void rdb_distribution_get(btree_slice_t *slice, int max_depth, const store_key_t &left_key,
                          transaction_t *txn, superblock_t *superblock, distribution_read_response_t *response) {
    int64_t key_count_out;
    std::vector<store_key_t> key_splits;
    get_btree_key_distribution(slice, txn, superblock, max_depth, &key_count_out, &key_splits);

    int64_t keys_per_bucket;
    if (key_splits.size() == 0) {
        keys_per_bucket = key_count_out;
    } else  {
        keys_per_bucket = std::max<int64_t>(key_count_out / key_splits.size(), 1);
    }
    response->key_counts[left_key] = keys_per_bucket;

    for (std::vector<store_key_t>::iterator it  = key_splits.begin();
                                            it != key_splits.end();
                                            ++it) {
        response->key_counts[*it] = keys_per_bucket;
    }
}

static const int8_t HAS_VALUE = 0;
static const int8_t HAS_NO_VALUE = 1;

void rdb_modification_info_t::rdb_serialize(write_message_t &msg) const {  // NOLINT(runtime/references)
    if (!deleted.first.get()) {
        guarantee(deleted.second.empty());
        msg << HAS_NO_VALUE;
    } else {
        msg << HAS_VALUE;
        msg << deleted;
    }

    if (!added.first.get()) {
        guarantee(added.second.empty());
        msg << HAS_NO_VALUE;
    } else {
        msg << HAS_VALUE;
        msg << added;
    }
}

archive_result_t rdb_modification_info_t::rdb_deserialize(read_stream_t *s) {
    archive_result_t res;

    int8_t has_value;
    res = deserialize(s, &has_value);
    if (res) { return res; }

    if (has_value == HAS_VALUE) {
        res = deserialize(s, &deleted);
        if (res) { return res; }
    }

    res = deserialize(s, &has_value);
    if (res) { return res; }

    if (has_value == HAS_VALUE) {
        res = deserialize(s, &added);
        if (res) { return res; }
    }

    return ARCHIVE_SUCCESS;
}

RDB_IMPL_ME_SERIALIZABLE_2(rdb_modification_report_t, primary_key, info);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_erase_range_report_t, range_to_erase);

rdb_modification_report_cb_t::rdb_modification_report_cb_t(
        btree_store_t<rdb_protocol_t> *store,
        write_token_pair_t *token_pair,
        transaction_t *txn, block_id_t sindex_block_id,
        auto_drainer_t::lock_t lock)
    : store_(store), token_pair_(token_pair),
      txn_(txn), sindex_block_id_(sindex_block_id),
      lock_(lock)
{ }

rdb_modification_report_cb_t::~rdb_modification_report_cb_t() {
    if (token_pair_->sindex_write_token.has()) {
        token_pair_->sindex_write_token.reset();
    }
}

void rdb_modification_report_cb_t::on_mod_report(
        const rdb_modification_report_t &mod_report) {
    if (!sindex_block_.has()) {
        // Don't allow interruption here, or we may end up with inconsistent data
        cond_t dummy_interruptor;
        store_->acquire_sindex_block_for_write(
            token_pair_, txn_, &sindex_block_,
            sindex_block_id_, &dummy_interruptor);

        store_->aquire_post_constructed_sindex_superblocks_for_write(
                sindex_block_.get(), txn_, &sindexes_);
    }

    mutex_t::acq_t acq;
    store_->lock_sindex_queue(sindex_block_.get(), &acq);

    write_message_t wm;
    wm << rdb_sindex_change_t(mod_report);
    store_->sindex_queue_push(wm, &acq);

    rdb_update_sindexes(sindexes_, &mod_report, txn_);
}

typedef btree_store_t<rdb_protocol_t>::sindex_access_vector_t sindex_access_vector_t;

void compute_keys(const store_key_t &primary_key, counted_t<const ql::datum_t> doc,
                  ql::map_wire_func_t *mapping, sindex_multi_bool_t multi, ql::env_t *env,
                  std::vector<store_key_t> *keys_out) {
    guarantee(keys_out->empty());
    counted_t<const ql::datum_t> index =
        mapping->compile_wire_func()->call(env, doc)->as_datum();

    if (multi == MULTI && index->get_type() == ql::datum_t::R_ARRAY) {
        for (uint64_t i = 0; i < index->size(); ++i) {
            keys_out->push_back(
                store_key_t(index->get(i, ql::THROW)->print_secondary(primary_key, i)));
        }
    } else {
        keys_out->push_back(store_key_t(index->print_secondary(primary_key)));
    }
}

/* Used below by rdb_update_sindexes. */
void rdb_update_single_sindex(
        const btree_store_t<rdb_protocol_t>::sindex_access_t *sindex,
        const rdb_modification_report_t *modification,
        transaction_t *txn,
        auto_drainer_t::lock_t) {
    // Note if you get this error it's likely that you've passed in a default
    // constructed mod_report. Don't do that.  Mod reports should always be passed
    // to a function as an output parameter before they're passed to this
    // function.
    guarantee(modification->primary_key.size() != 0);

    ql::map_wire_func_t mapping;
    sindex_multi_bool_t multi = MULTI;
    vector_read_stream_t read_stream(&sindex->sindex.opaque_definition);
    int success = deserialize(&read_stream, &mapping);
    guarantee(success == ARCHIVE_SUCCESS, "Corrupted sindex description.");
    success = deserialize(&read_stream, &multi);
    guarantee(success == ARCHIVE_SUCCESS, "Corrupted sindex description.");

    // TODO we just use a NULL environment here. People should not be able
    // to do anything that requires an environment like gets from other
    // tables etc. but we don't have a nice way to disallow those things so
    // for now we pass null and it will segfault if an illegal sindex
    // mapping is passed.
    cond_t non_interruptor;
    ql::env_t env(&non_interruptor);

    superblock_t *super_block = sindex->super_block.get();

    if (modification->info.deleted.first) {
        guarantee(!modification->info.deleted.second.empty());
        try {
            counted_t<const ql::datum_t> deleted = modification->info.deleted.first;

            std::vector<store_key_t> keys;

            compute_keys(modification->primary_key, deleted, &mapping, multi, &env, &keys);

            for (auto it = keys.begin(); it != keys.end(); ++it) {
                promise_t<superblock_t *> return_superblock_local;
                {
                    keyvalue_location_t<rdb_value_t> kv_location;

                    find_keyvalue_location_for_write(txn, super_block,
                                                     it->btree_key(),
                                                     &kv_location,
                                                     &sindex->btree->root_eviction_priority,
                                                     &sindex->btree->stats,
                                                     &return_superblock_local);

                    if (kv_location.value.has()) {
                        kv_location_delete(&kv_location, *it,
                            sindex->btree, repli_timestamp_t::distant_past, txn, NULL);
                    }
                    // The keyvalue location gets destroyed here.
                }
                super_block = return_superblock_local.wait();
            }
        } catch (const ql::base_exc_t &) {
            // Do nothing (it wasn't actually in the index).
        }
    }

    if (modification->info.added.first) {
        try {
            counted_t<const ql::datum_t> added = modification->info.added.first;

            std::vector<store_key_t> keys;

            compute_keys(modification->primary_key, added, &mapping, multi, &env, &keys);

            for (auto it = keys.begin(); it != keys.end(); ++it) {
                promise_t<superblock_t *> return_superblock_local;
                {
                    keyvalue_location_t<rdb_value_t> kv_location;

                    find_keyvalue_location_for_write(txn, super_block,
                                                     it->btree_key(),
                                                     &kv_location,
                                                     &sindex->btree->root_eviction_priority,
                                                     &sindex->btree->stats,
                                                     &return_superblock_local);

                    kv_location_set(&kv_location, *it,
                                    modification->info.added.second, sindex->btree,
                                    repli_timestamp_t::distant_past, txn);
                    // The keyvalue location gets destroyed here.
                }
                super_block = return_superblock_local.wait();
            }
        } catch (const ql::base_exc_t &) {
            // Do nothing (we just drop the row from the index).
        }
    }
}

void rdb_update_sindexes(const sindex_access_vector_t &sindexes,
        const rdb_modification_report_t *modification,
        transaction_t *txn) {
    {
        auto_drainer_t drainer;

        for (sindex_access_vector_t::const_iterator it  = sindexes.begin();
                                                    it != sindexes.end();
                                                    ++it) {
            coro_t::spawn_sometime(boost::bind(
                        &rdb_update_single_sindex, &*it,
                        modification, txn, auto_drainer_t::lock_t(&drainer)));
        }
    }

    /* All of the sindex have been updated now it's time to actually clear the
     * deleted blob if it exists. */
    std::vector<char> ref_cpy(modification->info.deleted.second);
    if (modification->info.deleted.first) {
        ref_cpy.insert(ref_cpy.end(), blob::btree_maxreflen - ref_cpy.size(), 0);
        guarantee(ref_cpy.size() == static_cast<size_t>(blob::btree_maxreflen));

        rdb_value_deleter_t deleter;
        deleter.delete_value(txn, ref_cpy.data());
    }
}

void rdb_erase_range_sindexes(const sindex_access_vector_t &sindexes,
        const rdb_erase_range_report_t *erase_range,
        transaction_t *txn, signal_t *interruptor) {
    auto_drainer_t drainer;

    spawn_sindex_erase_ranges(&sindexes, erase_range->range_to_erase,
            txn, &drainer, auto_drainer_t::lock_t(&drainer),
            false, /* don't release the superblock */ interruptor);
}

class post_construct_traversal_helper_t : public btree_traversal_helper_t {
public:
    post_construct_traversal_helper_t(
            btree_store_t<rdb_protocol_t> *store,
            const std::set<uuid_u> &sindexes_to_post_construct,
            cond_t *interrupt_myself,
            signal_t *interruptor
            )
        : store_(store),
          sindexes_to_post_construct_(sindexes_to_post_construct),
          interrupt_myself_(interrupt_myself), interruptor_(interruptor)
    { }

    void process_a_leaf(transaction_t *txn, buf_lock_t *leaf_node_buf,
                        const btree_key_t *, const btree_key_t *,
                        signal_t *, int *) THROWS_ONLY(interrupted_exc_t) {
        write_token_pair_t token_pair;
        store_->new_write_token_pair(&token_pair);

        scoped_ptr_t<transaction_t> wtxn;
        btree_store_t<rdb_protocol_t>::sindex_access_vector_t sindexes;

        // If we get interrupted, post-construction will happen later, no need to
        //  guarantee that we touch the sindex tree now
        object_buffer_t<fifo_enforcer_sink_t::exit_write_t>::destruction_sentinel_t
            destroyer(&token_pair.sindex_write_token);

        try {
            scoped_ptr_t<real_superblock_t> superblock;

            // We want soft durability because having a partially constructed secondary index is
            // okay -- we wipe it and rebuild it, if it has not been marked completely
            // constructed.
            store_->acquire_superblock_for_write(
                rwi_write,
                repli_timestamp_t::distant_past,
                2,
                WRITE_DURABILITY_SOFT,
                &token_pair,
                &wtxn,
                &superblock,
                interruptor_);

            scoped_ptr_t<buf_lock_t> sindex_block;
            store_->acquire_sindex_block_for_write(
                &token_pair,
                wtxn.get(),
                &sindex_block,
                superblock->get_sindex_block_id(),
                interruptor_);

            store_->acquire_sindex_superblocks_for_write(
                    sindexes_to_post_construct_,
                    sindex_block.get(),
                    wtxn.get(),
                    &sindexes);

            if (sindexes.empty()) {
                interrupt_myself_->pulse_if_not_already_pulsed();
                return;
            }
        } catch (const interrupted_exc_t &e) {
            return;
        }

        const leaf_node_t *leaf_node = static_cast<const leaf_node_t *>(leaf_node_buf->get_data_read());

        for (auto it = leaf::begin(*leaf_node); it != leaf::end(*leaf_node); ++it) {
            /* Grab relevant values from the leaf node. */
            const btree_key_t *key = (*it).first;
            const void *value = (*it).second;
            guarantee(key);

            store_key_t pk(key);
            rdb_modification_report_t mod_report(pk);
            const rdb_value_t *rdb_value = static_cast<const rdb_value_t *>(value);
            block_size_t block_size = txn->get_cache()->get_block_size();
            mod_report.info.added = std::make_pair(get_data(rdb_value, txn),
                    std::vector<char>(rdb_value->value_ref(),
                        rdb_value->value_ref() + rdb_value->inline_size(block_size)));

            rdb_update_sindexes(sindexes, &mod_report, wtxn.get());
        }
    }

    void postprocess_internal_node(buf_lock_t *) { }

    void filter_interesting_children(UNUSED transaction_t *txn, ranged_block_ids_t *ids_source, interesting_children_callback_t *cb) {
        for (int i = 0, e = ids_source->num_block_ids(); i < e; ++i) {
            cb->receive_interesting_child(i);
        }
        cb->no_more_interesting_children();
    }

    access_t btree_superblock_mode() { return rwi_read; }
    access_t btree_node_mode() { return rwi_read; }

    btree_store_t<rdb_protocol_t> *store_;
    const std::set<uuid_u> &sindexes_to_post_construct_;
    cond_t *interrupt_myself_;
    signal_t *interruptor_;
};

void post_construct_secondary_indexes(
        btree_store_t<rdb_protocol_t> *store,
        const std::set<uuid_u> &sindexes_to_post_construct,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) {
    cond_t local_interruptor;

    wait_any_t wait_any(&local_interruptor, interruptor);

    post_construct_traversal_helper_t helper(store,
            sindexes_to_post_construct, &local_interruptor, interruptor);

    object_buffer_t<fifo_enforcer_sink_t::exit_read_t> read_token;
    store->new_read_token(&read_token);

    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;

    store->acquire_superblock_for_read(
        rwi_read,
        &read_token,
        &txn,
        &superblock,
        interruptor,
        true /* USE_SNAPSHOT */);

    btree_parallel_traversal(txn.get(), superblock.get(),
            store->btree.get(), &helper, &wait_any);
}
