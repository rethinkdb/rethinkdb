// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/btree.hpp"

#include <functional>
#include <string>
#include <vector>

#include "btree/backfill.hpp"
#include "btree/concurrent_traversal.hpp"
#include "btree/erase_range.hpp"
#include "btree/get_distribution.hpp"
#include "btree/operations.hpp"
#include "btree/parallel_traversal.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/alt/alt_serialize_onto_blob.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/buffer_group_stream.hpp"
#include "containers/archive/varint.hpp"
#include "containers/archive/vector_stream.hpp"
#include "containers/scoped.hpp"
#include "rdb_protocol/blob_wrapper.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/lazy_json.hpp"
#include "rdb_protocol/shards.hpp"

rdb_value_sizer_t::rdb_value_sizer_t(max_block_size_t bs) : block_size_(bs) { }

const rdb_value_t *rdb_value_sizer_t::as_rdb(const void *p) {
    return reinterpret_cast<const rdb_value_t *>(p);
}

int rdb_value_sizer_t::size(const void *value) const {
    return as_rdb(value)->inline_size(block_size_);
}

bool rdb_value_sizer_t::fits(const void *value, int length_available) const {
    return btree_value_fits(block_size_, length_available, as_rdb(value));
}

int rdb_value_sizer_t::max_possible_size() const {
    return blob::btree_maxreflen;
}

block_magic_t rdb_value_sizer_t::leaf_magic() {
    block_magic_t magic = { { 'r', 'd', 'b', 'l' } };
    return magic;
}

block_magic_t rdb_value_sizer_t::btree_leaf_magic() const {
    return leaf_magic();
}

max_block_size_t rdb_value_sizer_t::block_size() const { return block_size_; }

bool btree_value_fits(max_block_size_t bs, int data_length, const rdb_value_t *value) {
    return blob::ref_fits(bs, data_length, value->value_ref(), blob::btree_maxreflen);
}

// Remember that secondary indexes and the main btree both point to the same rdb
// value -- you don't want to double-delete that value!
void actually_delete_rdb_value(buf_parent_t parent, void *value) {
    blob_t blob(parent.cache()->max_block_size(),
                static_cast<rdb_value_t *>(value)->value_ref(),
                blob::btree_maxreflen);
    blob.clear(parent);
}

void detach_rdb_value(buf_parent_t parent, const void *value) {
    // This const_cast is ok, since `detach_subtrees` is one of the operations
    // that does not actually change value.
    void *non_const_value = const_cast<void *>(value);
    blob_t blob(parent.cache()->max_block_size(),
                static_cast<rdb_value_t *>(non_const_value)->value_ref(),
                blob::btree_maxreflen);
    blob.detach_subtrees(parent);
}

void rdb_get(const store_key_t &store_key, btree_slice_t *slice,
             superblock_t *superblock, point_read_response_t *response,
             profile::trace_t *trace) {
    keyvalue_location_t kv_location;
    rdb_value_sizer_t sizer(superblock->cache()->max_block_size());
    find_keyvalue_location_for_read(&sizer, superblock,
                                    store_key.btree_key(), &kv_location,
                                    &slice->stats, trace);

    if (!kv_location.value.has()) {
        response->data.reset(new ql::datum_t(ql::datum_t::R_NULL));
    } else {
        response->data = get_data(static_cast<rdb_value_t *>(kv_location.value.get()),
                                  buf_parent_t(&kv_location.buf));
    }
}

void kv_location_delete(keyvalue_location_t *kv_location,
                        const store_key_t &key,
                        repli_timestamp_t timestamp,
                        const deletion_context_t *deletion_context,
                        rdb_modification_info_t *mod_info_out) {
    // Notice this also implies that buf is valid.
    guarantee(kv_location->value.has());

    // As noted above, we can be sure that buf is valid.
    const max_block_size_t block_size = kv_location->buf.cache()->max_block_size();

    if (mod_info_out != NULL) {
        guarantee(mod_info_out->deleted.second.empty());

        mod_info_out->deleted.second.assign(
                kv_location->value_as<rdb_value_t>()->value_ref(),
                kv_location->value_as<rdb_value_t>()->value_ref()
                + kv_location->value_as<rdb_value_t>()->inline_size(block_size));
    }

    // Detach/Delete
    deletion_context->in_tree_deleter()->delete_value(buf_parent_t(&kv_location->buf),
                                                      kv_location->value.get());

    kv_location->value.reset();
    rdb_value_sizer_t sizer(block_size);
    null_key_modification_callback_t null_cb;
    apply_keyvalue_change(&sizer, kv_location, key.btree_key(), timestamp,
            deletion_context->balancing_detacher(), &null_cb);
}

void kv_location_set(keyvalue_location_t *kv_location,
                     const store_key_t &key,
                     counted_t<const ql::datum_t> data,
                     repli_timestamp_t timestamp,
                     const deletion_context_t *deletion_context,
                     rdb_modification_info_t *mod_info_out) {
    scoped_malloc_t<rdb_value_t> new_value(blob::btree_maxreflen);
    memset(new_value.get(), 0, blob::btree_maxreflen);

    const max_block_size_t block_size = kv_location->buf.cache()->max_block_size();
    {
        blob_t blob(block_size, new_value->value_ref(), blob::btree_maxreflen);
        serialize_onto_blob(buf_parent_t(&kv_location->buf), &blob, data);
    }

    if (mod_info_out) {
        guarantee(mod_info_out->added.second.empty());
        mod_info_out->added.second.assign(new_value->value_ref(),
            new_value->value_ref() + new_value->inline_size(block_size));
    }

    if (kv_location->value.has()) {
        deletion_context->in_tree_deleter()->delete_value(
                buf_parent_t(&kv_location->buf), kv_location->value.get());
        if (mod_info_out != NULL) {
            guarantee(mod_info_out->deleted.second.empty());
            mod_info_out->deleted.second.assign(
                    kv_location->value_as<rdb_value_t>()->value_ref(),
                    kv_location->value_as<rdb_value_t>()->value_ref()
                    + kv_location->value_as<rdb_value_t>()->inline_size(block_size));
        }
    }

    // Actually update the leaf, if needed.
    kv_location->value = std::move(new_value);
    null_key_modification_callback_t null_cb;
    rdb_value_sizer_t sizer(block_size);
    apply_keyvalue_change(&sizer, kv_location, key.btree_key(),
                          timestamp,
                          deletion_context->balancing_detacher(), &null_cb);
}

void kv_location_set(keyvalue_location_t *kv_location,
                     const store_key_t &key,
                     const std::vector<char> &value_ref,
                     repli_timestamp_t timestamp,
                     const deletion_context_t *deletion_context) {
    // Detach/Delete the old value.
    if (kv_location->value.has()) {
        deletion_context->in_tree_deleter()->delete_value(
                buf_parent_t(&kv_location->buf), kv_location->value.get());
    }

    scoped_malloc_t<rdb_value_t> new_value(
            value_ref.data(), value_ref.data() + value_ref.size());

    // Update the leaf, if needed.
    kv_location->value = std::move(new_value);

    null_key_modification_callback_t null_cb;
    rdb_value_sizer_t sizer(kv_location->buf.cache()->max_block_size());
    apply_keyvalue_change(&sizer, kv_location, key.btree_key(), timestamp,
                          deletion_context->balancing_detacher(), &null_cb);
}

batched_replace_response_t rdb_replace_and_return_superblock(
    const btree_loc_info_t &info,
    const btree_point_replacer_t *replacer,
    const deletion_context_t *deletion_context,
    promise_t<superblock_t *> *superblock_promise,
    rdb_modification_info_t *mod_info_out,
    profile::trace_t *trace)
{
    bool return_vals = replacer->should_return_vals();
    const std::string &primary_key = *info.btree->primary_key;
    const store_key_t &key = *info.key;
    ql::datum_ptr_t resp(ql::datum_t::R_OBJECT);
    try {
        keyvalue_location_t kv_location;
        rdb_value_sizer_t sizer(info.superblock->cache()->max_block_size());
        find_keyvalue_location_for_write(&sizer, info.superblock,
                                         info.key->btree_key(),
                                         deletion_context->balancing_detacher(),
                                         &kv_location,
                                         &info.btree->slice->stats,
                                         trace,
                                         superblock_promise);

        bool started_empty, ended_empty;
        counted_t<const ql::datum_t> old_val;
        if (!kv_location.value.has()) {
            // If there's no entry with this key, pass NULL to the function.
            started_empty = true;
            old_val = make_counted<ql::datum_t>(ql::datum_t::R_NULL);
        } else {
            // Otherwise pass the entry with this key to the function.
            started_empty = false;
            old_val = get_data(kv_location.value_as<rdb_value_t>(),
                               buf_parent_t(&kv_location.buf));
            guarantee(old_val->get(primary_key, ql::NOTHROW).has());
        }
        guarantee(old_val.has());
        if (return_vals == RETURN_VALS) {
            bool conflict = resp.add("old_val", old_val)
                         || resp.add("new_val", old_val); // changed below
            guarantee(!conflict);
        }

        counted_t<const ql::datum_t> new_val = replacer->replace(old_val);
        if (return_vals == RETURN_VALS) {
            bool conflict = resp.add("new_val", new_val, ql::CLOBBER);
            guarantee(conflict); // We set it to `old_val` previously.
        }
        if (new_val->get_type() == ql::datum_t::R_NULL) {
            ended_empty = true;
        } else if (new_val->get_type() == ql::datum_t::R_OBJECT) {
            ended_empty = false;
            new_val->rcheck_valid_replace(
                old_val, counted_t<const ql::datum_t>(), primary_key);
            counted_t<const ql::datum_t> pk = new_val->get(primary_key, ql::NOTHROW);
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
                kv_location_set(&kv_location, *info.key, new_val, 
                                info.btree->timestamp, deletion_context,
                                mod_info_out);
                guarantee(mod_info_out->deleted.second.empty());
                guarantee(!mod_info_out->added.second.empty());
                mod_info_out->added.first = new_val;
            }
        } else {
            if (ended_empty) {
                conflict = resp.add("deleted", make_counted<ql::datum_t>(1.0));
                kv_location_delete(&kv_location, *info.key, info.btree->timestamp,
                                   deletion_context, mod_info_out);
                guarantee(!mod_info_out->deleted.second.empty());
                guarantee(mod_info_out->added.second.empty());
                mod_info_out->deleted.first = old_val;
            } else {
                r_sanity_check(
                    *old_val->get(primary_key) == *new_val->get(primary_key));
                if (*old_val == *new_val) {
                    conflict = resp.add("unchanged",
                                         make_counted<ql::datum_t>(1.0));
                } else {
                    conflict = resp.add("replaced", make_counted<ql::datum_t>(1.0));
                    r_sanity_check(new_val->get(primary_key, ql::NOTHROW).has());
                    kv_location_set(&kv_location, *info.key, new_val,
                                    info.btree->timestamp, deletion_context,
                                    mod_info_out);
                    guarantee(!mod_info_out->deleted.second.empty());
                    guarantee(!mod_info_out->added.second.empty());
                    mod_info_out->added.first = new_val;
                    mod_info_out->deleted.first = old_val;
                }
            }
        }
        guarantee(!conflict); // message never added twice
    } catch (const ql::base_exc_t &e) {
        resp.add_error(e.what());
    } catch (const interrupted_exc_t &e) {
        std::string msg = strprintf("interrupted (%s:%d)", __FILE__, __LINE__);
        resp.add_error(msg.c_str());
        // We don't rethrow because we're in a coroutine.  Theoretically the
        // above message should never make it back to a user because the calling
        // function will also be interrupted, but we document where it comes
        // from to aid in future debugging if that invariant becomes violated.
    }
    return resp.to_counted();
}


class one_replace_t : public btree_point_replacer_t {
public:
    one_replace_t(const btree_batched_replacer_t *_replacer, size_t _index)
        : replacer(_replacer), index(_index) { }

    counted_t<const ql::datum_t> replace(const counted_t<const ql::datum_t> &d) const {
        return replacer->replace(d, index);
    }
    bool should_return_vals() const { return replacer->should_return_vals(); }
private:
    const btree_batched_replacer_t *const replacer;
    const size_t index;
};

void do_a_replace_from_batched_replace(
    auto_drainer_t::lock_t,
    fifo_enforcer_sink_t *batched_replaces_fifo_sink,
    const fifo_enforcer_write_token_t &batched_replaces_fifo_token,
    const btree_loc_info_t &info,
    const one_replace_t one_replace,
    promise_t<superblock_t *> *superblock_promise,
    rdb_modification_report_cb_t *sindex_cb,
    batched_replace_response_t *stats_out,
    profile::trace_t *trace)
{
    fifo_enforcer_sink_t::exit_write_t exiter(
        batched_replaces_fifo_sink, batched_replaces_fifo_token);

    rdb_live_deletion_context_t deletion_context;
    rdb_modification_report_t mod_report(*info.key);
    counted_t<const ql::datum_t> res = rdb_replace_and_return_superblock(
        info, &one_replace, &deletion_context, superblock_promise, &mod_report.info,
        trace);
    *stats_out = (*stats_out)->merge(res, ql::stats_merge);

    // KSI: What is this for?  are we waiting to get in line to call on_mod_report?
    // I guess so.

    // JD: Looks like this is a do_a_replace_from_batched_replace specific thing.
    exiter.wait();
    sindex_cb->on_mod_report(mod_report);
}

batched_replace_response_t rdb_batched_replace(
    const btree_info_t &info,
    scoped_ptr_t<superblock_t> *superblock,
    const std::vector<store_key_t> &keys,
    const btree_batched_replacer_t *replacer,
    rdb_modification_report_cb_t *sindex_cb,
    profile::trace_t *trace) {

    fifo_enforcer_source_t batched_replaces_fifo_source;
    fifo_enforcer_sink_t batched_replaces_fifo_sink;

    counted_t<const ql::datum_t> stats(new ql::datum_t(ql::datum_t::R_OBJECT));

    // We have to drain write operations before destructing everything above us,
    // because the coroutines being drained use them.
    {
        auto_drainer_t drainer;
        // Note the destructor ordering: We release the superblock before draining
        // on all the write operations.
        scoped_ptr_t<superblock_t> current_superblock(superblock->release());
        for (size_t i = 0; i < keys.size(); ++i) {
            // Pass out the point_replace_response_t.
            promise_t<superblock_t *> superblock_promise;
            coro_t::spawn_sometime(
                std::bind(
                    &do_a_replace_from_batched_replace,
                    auto_drainer_t::lock_t(&drainer),
                    &batched_replaces_fifo_sink,
                    batched_replaces_fifo_source.enter_write(),

                    btree_loc_info_t(&info, current_superblock.release(), &keys[i]),
                    one_replace_t(replacer, i),

                    &superblock_promise,
                    sindex_cb,
                    &stats,
                    trace));

            current_superblock.init(superblock_promise.wait());
        }
    } // Make sure the drainer is destructed before the return statement.
    return stats;
}

void rdb_set(const store_key_t &key,
             counted_t<const ql::datum_t> data,
             bool overwrite,
             btree_slice_t *slice,
             repli_timestamp_t timestamp,
             superblock_t *superblock,
             const deletion_context_t *deletion_context,
             point_write_response_t *response_out,
             rdb_modification_info_t *mod_info,
             profile::trace_t *trace,
             promise_t<superblock_t *> *pass_back_superblock) {
    keyvalue_location_t kv_location;
    rdb_value_sizer_t sizer(superblock->cache()->max_block_size());
    find_keyvalue_location_for_write(&sizer, superblock, key.btree_key(),
                                     deletion_context->balancing_detacher(),
                                     &kv_location, &slice->stats, trace,
                                     pass_back_superblock);
    const bool had_value = kv_location.value.has();

    /* update the modification report */
    if (kv_location.value.has()) {
        mod_info->deleted.first = get_data(kv_location.value_as<rdb_value_t>(),
                                           buf_parent_t(&kv_location.buf));
    }

    mod_info->added.first = data;

    if (overwrite || !had_value) {
        kv_location_set(&kv_location, key, data, timestamp, deletion_context,
                        mod_info);
        guarantee(mod_info->deleted.second.empty() == !had_value &&
                  !mod_info->added.second.empty());
    }
    response_out->result =
        (had_value ? point_write_result_t::DUPLICATE : point_write_result_t::STORED);
}

class agnostic_rdb_backfill_callback_t : public agnostic_backfill_callback_t {
public:
    agnostic_rdb_backfill_callback_t(rdb_backfill_callback_t *cb,
                                     const key_range_t &kr,
                                     btree_slice_t *slice) :
        cb_(cb), kr_(kr), slice_(slice) { }

    void on_delete_range(const key_range_t &range, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        rassert(kr_.is_superset(range));
        cb_->on_delete_range(range, interruptor);
    }

    void on_deletion(const btree_key_t *key, repli_timestamp_t recency, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        rassert(kr_.contains_key(key->contents, key->size));
        cb_->on_deletion(key, recency, interruptor);
    }

    void on_pairs(buf_parent_t leaf_node, const std::vector<repli_timestamp_t> &recencies,
                  const std::vector<const btree_key_t *> &keys,
                  const std::vector<const void *> &vals,
                  signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {

        std::vector<backfill_atom_t> chunk_atoms;
        chunk_atoms.reserve(keys.size());
        size_t current_chunk_size = 0;

        for (size_t i = 0; i < keys.size(); ++i) {
            rassert(kr_.contains_key(keys[i]->contents, keys[i]->size));
            const rdb_value_t *value = static_cast<const rdb_value_t *>(vals[i]);

            backfill_atom_t atom;
            atom.key.assign(keys[i]->size, keys[i]->contents);
            atom.value = get_data(value, leaf_node);
            atom.recency = recencies[i];
            chunk_atoms.push_back(atom);
            current_chunk_size += static_cast<size_t>(atom.key.size())
                                  + serialized_size(atom.value);

            if (current_chunk_size >= BACKFILL_MAX_KVPAIRS_SIZE) {
                // To avoid flooding the receiving node with overly large chunks
                // (which could easily make it run out of memory in extreme
                // cases), pass on what we have got so far. Then continue
                // with the remaining values.
                slice_->stats.pm_keys_read.record(chunk_atoms.size());
                cb_->on_keyvalues(std::move(chunk_atoms), interruptor);
                chunk_atoms = std::vector<backfill_atom_t>();
                chunk_atoms.reserve(keys.size() - (i+1));
                current_chunk_size = 0;
            }
        }
        if (!chunk_atoms.empty()) {
            // Pass on the final chunk
            slice_->stats.pm_keys_read.record(chunk_atoms.size());
            cb_->on_keyvalues(std::move(chunk_atoms), interruptor);
        }
    }

    void on_sindexes(const std::map<std::string, secondary_index_t> &sindexes, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        cb_->on_sindexes(sindexes, interruptor);
    }

    rdb_backfill_callback_t *cb_;
    key_range_t kr_;
    btree_slice_t *slice_;
};

void rdb_backfill(btree_slice_t *slice, const key_range_t& key_range,
                  repli_timestamp_t since_when, rdb_backfill_callback_t *callback,
                  superblock_t *superblock,
                  buf_lock_t *sindex_block,
                  parallel_traversal_progress_t *p, signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) {
    agnostic_rdb_backfill_callback_t agnostic_cb(callback, key_range, slice);
    rdb_value_sizer_t sizer(superblock->cache()->max_block_size());
    do_agnostic_btree_backfill(&sizer, key_range, since_when, &agnostic_cb,
                               superblock, sindex_block, p, interruptor);
}

void rdb_delete(const store_key_t &key, btree_slice_t *slice,
                repli_timestamp_t timestamp,
                superblock_t *superblock,
                const deletion_context_t *deletion_context,
                point_delete_response_t *response,
                rdb_modification_info_t *mod_info,
                profile::trace_t *trace) {
    keyvalue_location_t kv_location;
    rdb_value_sizer_t sizer(superblock->cache()->max_block_size());
    find_keyvalue_location_for_write(&sizer, superblock, key.btree_key(),
            deletion_context->balancing_detacher(), &kv_location, &slice->stats, trace);
    bool exists = kv_location.value.has();

    /* Update the modification report. */
    if (exists) {
        mod_info->deleted.first = get_data(kv_location.value_as<rdb_value_t>(),
                                           buf_parent_t(&kv_location.buf));
        kv_location_delete(&kv_location, key, timestamp, deletion_context, mod_info);
    }
    guarantee(!mod_info->deleted.second.empty() && mod_info->added.second.empty());
    response->result = (exists ? point_delete_result_t::DELETED : point_delete_result_t::MISSING);
}

void rdb_value_deleter_t::delete_value(buf_parent_t parent, const void *value) const {
    // To not destroy constness, we operate on a copy of the value
    rdb_value_sizer_t sizer(parent.cache()->max_block_size());
    scoped_malloc_t<rdb_value_t> value_copy(sizer.max_possible_size());
    memcpy(value_copy.get(), value, sizer.size(value));
    actually_delete_rdb_value(parent, value_copy.get());
}

void rdb_value_detacher_t::delete_value(buf_parent_t parent, const void *value) const {
    detach_rdb_value(parent, value);
}

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

void sindex_erase_range(const key_range_t &key_range,
        superblock_t *superblock, auto_drainer_t::lock_t,
        signal_t *interruptor, bool release_superblock,
        const value_deleter_t *deleter) THROWS_NOTHING {

    rdb_value_sizer_t sizer(superblock->cache()->max_block_size());

    sindex_key_range_tester_t tester(key_range);

    try {
        btree_erase_range_generic(&sizer, &tester,
                                  deleter, NULL, NULL,
                                  superblock, interruptor,
                                  release_superblock);
    } catch (const interrupted_exc_t &) {
        // We were interrupted. That's fine nothing to be done about it.
    }
}

/* Spawns a coro to carry out the erase range for each sindex. */
void spawn_sindex_erase_ranges(
        const store_t::sindex_access_vector_t *sindex_access,
        const key_range_t &key_range,
        auto_drainer_t *drainer,
        auto_drainer_t::lock_t,
        bool release_superblock,
        signal_t *interruptor,
        const value_deleter_t *deleter) {
    for (auto it = sindex_access->begin(); it != sindex_access->end(); ++it) {
        coro_t::spawn_sometime(std::bind(
                    &sindex_erase_range, key_range, it->super_block.get(),
                    auto_drainer_t::lock_t(drainer), interruptor,
                    release_superblock, deleter));
    }
}

/* Helper function for rdb_erase_*_range() */
void rdb_erase_range_convert_keys(const key_range_t &key_range,
                                  bool *left_key_supplied_out,
                                  bool *right_key_supplied_out,
                                  store_key_t *left_key_exclusive_out,
                                  store_key_t *right_key_inclusive_out) {
    /* This is guaranteed because the way the keys are calculated below would
     * lead to a single key being deleted even if the range was empty. */
    guarantee(!key_range.is_empty());

    rassert(left_key_supplied_out != NULL);
    rassert(right_key_supplied_out != NULL);
    rassert(left_key_exclusive_out != NULL);
    rassert(right_key_inclusive_out != NULL);

    /* Twiddle some keys to get the in the form we want. Notice these are keys
     * which will be made  exclusive and inclusive as their names suggest
     * below. At the point of construction they aren't. */
    *left_key_exclusive_out = store_key_t(key_range.left);
    *right_key_inclusive_out = store_key_t(key_range.right.key);

    *left_key_supplied_out = left_key_exclusive_out->decrement();
    *right_key_supplied_out = !key_range.right.unbounded;
    if (*right_key_supplied_out) {
        right_key_inclusive_out->decrement();
    }

    /* Now left_key_exclusive and right_key_inclusive accurately reflect their
     * names. */
}

void rdb_erase_major_range(key_tester_t *tester,
                           const key_range_t &key_range,
                           buf_lock_t *sindex_block,
                           superblock_t *superblock,
                           store_t *store,
                           signal_t *interruptor) {

    /* Dispatch the erase range to the sindexes. */
    store_t::sindex_access_vector_t sindex_superblocks;
    {
        store->acquire_post_constructed_sindex_superblocks_for_write(
                sindex_block, &sindex_superblocks);

        scoped_ptr_t<new_mutex_in_line_t> acq =
            store->get_in_line_for_sindex_queue(sindex_block);
        sindex_block->reset_buf_lock();

        write_message_t wm;
        serialize(&wm, rdb_sindex_change_t(rdb_erase_major_range_report_t(key_range)));
        store->sindex_queue_push(wm, acq.get());
    }

    {
        rdb_value_detacher_t detacher;
        auto_drainer_t sindex_erase_drainer;
        spawn_sindex_erase_ranges(&sindex_superblocks, key_range,
                &sindex_erase_drainer, auto_drainer_t::lock_t(&sindex_erase_drainer),
                true /* release the superblock */, interruptor, &detacher);

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

    bool left_key_supplied, right_key_supplied;
    store_key_t left_key_exclusive, right_key_inclusive;
    rdb_erase_range_convert_keys(key_range, &left_key_supplied, &right_key_supplied,
            &left_key_exclusive, &right_key_inclusive);

    /* We need these structures to perform the erase range. */
    rdb_value_sizer_t sizer(superblock->cache()->max_block_size());

    /* Actually delete the values */
    rdb_value_deleter_t deleter;

    btree_erase_range_generic(&sizer, tester, &deleter,
        left_key_supplied ? left_key_exclusive.btree_key() : NULL,
        right_key_supplied ? right_key_inclusive.btree_key() : NULL,
        superblock, interruptor);
}

void rdb_erase_small_range(key_tester_t *tester,
                           const key_range_t &key_range,
                           superblock_t *superblock,
                           const deletion_context_t *deletion_context,
                           signal_t *interruptor,
                           std::vector<rdb_modification_report_t> *mod_reports_out) {
    rassert(mod_reports_out != NULL);
    mod_reports_out->clear();

    bool left_key_supplied, right_key_supplied;
    store_key_t left_key_exclusive, right_key_inclusive;
    rdb_erase_range_convert_keys(key_range, &left_key_supplied, &right_key_supplied,
             &left_key_exclusive, &right_key_inclusive);

    /* We need these structures to perform the erase range. */
    rdb_value_sizer_t sizer(superblock->cache()->max_block_size());

    struct on_erase_cb_t {
        static void on_erase(const store_key_t &key, const char *data,
                const buf_parent_t &parent,
                std::vector<rdb_modification_report_t> *_mod_reports_out) {
            const rdb_value_t *value = reinterpret_cast<const rdb_value_t *>(data);

            // The mod_report we generate is a simple delete. While there is generally
            // a difference between an erase and a delete (deletes get backfilled,
            // while an erase is as if the value had never existed), that
            // difference is irrelevant in the case of secondary indexes.
            rdb_modification_report_t mod_report;
            mod_report.primary_key = key;
            // Get the full data
            mod_report.info.deleted.first = get_data(value, parent);
            // Get the inline value
            max_block_size_t block_size = parent.cache()->max_block_size();
            mod_report.info.deleted.second.assign(value->value_ref(),
                value->value_ref() + value->inline_size(block_size));

            _mod_reports_out->push_back(mod_report);
        }
    };

    btree_erase_range_generic(&sizer, tester, deletion_context->in_tree_deleter(),
        left_key_supplied ? left_key_exclusive.btree_key() : NULL,
        right_key_supplied ? right_key_inclusive.btree_key() : NULL,
        superblock, interruptor, true /* release_superblock */,
        std::bind(&on_erase_cb_t::on_erase, ph::_1, ph::_2, ph::_3, mod_reports_out));
}

// This is actually a kind of misleading name. This function estimates the size
// of a datum, not a whole rget, though it is used for that purpose (by summing
// up these responses).
size_t estimate_rget_response_size(const counted_t<const ql::datum_t> &datum) {
    return serialized_size(datum);
}


typedef ql::transform_variant_t transform_variant_t;
typedef ql::terminal_variant_t terminal_variant_t;

class sindex_data_t {
public:
    sindex_data_t(const key_range_t &_pkey_range, const datum_range_t &_range,
                  ql::map_wire_func_t wire_func, sindex_multi_bool_t _multi)
        : pkey_range(_pkey_range), range(_range),
          func(wire_func.compile_wire_func()), multi(_multi) { }
private:
    friend class rget_cb_t;
    const key_range_t pkey_range;
    const datum_range_t range;
    const counted_t<ql::func_t> func;
    const sindex_multi_bool_t multi;
};

class job_data_t {
public:
    job_data_t(ql::env_t *_env, const ql::batchspec_t &batchspec,
               const std::vector<transform_variant_t> &_transforms,
               const boost::optional<terminal_variant_t> &_terminal,
               sorting_t _sorting)
        : env(_env),
          batcher(batchspec.to_batcher()),
          sorting(_sorting),
          accumulator(_terminal
                      ? ql::make_terminal(env, *_terminal)
                      : ql::make_append(sorting, &batcher)) {
        for (size_t i = 0; i < _transforms.size(); ++i) {
            transformers.emplace_back(ql::make_op(env, _transforms[i]));
        }
        guarantee(transformers.size() == _transforms.size());
    }
    job_data_t(job_data_t &&jd)
        : env(jd.env),
          batcher(std::move(jd.batcher)),
          transformers(std::move(jd.transformers)),
          sorting(jd.sorting),
          accumulator(jd.accumulator.release()) {
    }
private:
    friend class rget_cb_t;
    ql::env_t *const env;
    ql::batcher_t batcher;
    std::vector<scoped_ptr_t<ql::op_t> > transformers;
    sorting_t sorting;
    scoped_ptr_t<ql::accumulator_t> accumulator;
};

class io_data_t {
public:
    io_data_t(rget_read_response_t *_response, btree_slice_t *_slice)
        : response(_response), slice(_slice) { }
private:
    friend class rget_cb_t;
    rget_read_response_t *const response;
    btree_slice_t *const slice;
};

class rget_cb_t : public concurrent_traversal_callback_t {
public:
    rget_cb_t(io_data_t &&_io,
              job_data_t &&_job,
              boost::optional<sindex_data_t> &&_sindex,
              const key_range_t &range);

    virtual done_traversing_t handle_pair(scoped_key_value_t &&keyvalue,
                               concurrent_traversal_fifo_enforcer_signal_t waiter)
    THROWS_ONLY(interrupted_exc_t);
    void finish() THROWS_ONLY(interrupted_exc_t);
private:
    const io_data_t io; // How do get data in/out.
    job_data_t job; // What to do next (stateful).
    const boost::optional<sindex_data_t> sindex; // Optional sindex information.

    // State for internal bookkeeping.
    bool bad_init;
    scoped_ptr_t<profile::disabler_t> disabler;
    scoped_ptr_t<profile::sampler_t> sampler;
};

rget_cb_t::rget_cb_t(io_data_t &&_io,
                     job_data_t &&_job,
                     boost::optional<sindex_data_t> &&_sindex,
                     const key_range_t &range)
    : io(std::move(_io)),
      job(std::move(_job)),
      sindex(std::move(_sindex)),
      bad_init(false) {
    io.response->last_key = !reversed(job.sorting)
        ? range.left
        : (!range.right.unbounded ? range.right.key : store_key_t::max());
    disabler.init(new profile::disabler_t(job.env->trace));
    sampler.init(new profile::sampler_t("Range traversal doc evaluation.",
                                        job.env->trace));
}

void rget_cb_t::finish() THROWS_ONLY(interrupted_exc_t) {
    job.accumulator->finish(&io.response->result);
    if (job.accumulator->should_send_batch()) {
        io.response->truncated = true;
    }
}

// Handle a keyvalue pair.  Returns whether or not we're done early.
done_traversing_t rget_cb_t::handle_pair(scoped_key_value_t &&keyvalue,
                              concurrent_traversal_fifo_enforcer_signal_t waiter)
THROWS_ONLY(interrupted_exc_t) {
    sampler->new_sample();

    if (bad_init || boost::get<ql::exc_t>(&io.response->result) != NULL) {
        return done_traversing_t::YES;
    }

    // Load the key and value.
    store_key_t key(keyvalue.key());
    if (sindex && !sindex->pkey_range.contains_key(ql::datum_t::extract_primary(key))) {
        return done_traversing_t::NO;
    }

    lazy_json_t row(static_cast<const rdb_value_t *>(keyvalue.value()),
                    keyvalue.expose_buf());
    counted_t<const ql::datum_t> val;
    // We only load the value if we actually use it (`count` does not).
    if (job.accumulator->uses_val() || job.transformers.size() != 0 || sindex) {
        val = row.get();
        io.slice->stats.pm_keys_read.record();
    } else {
        row.reset();
    }
    guarantee(!row.references_parent());
    keyvalue.reset();
    waiter.wait_interruptible();

    try {
        // Update the last considered key.
        if ((io.response->last_key < key && !reversed(job.sorting)) ||
            (io.response->last_key > key && reversed(job.sorting))) {
            io.response->last_key = key;
        }

        // Check whether we're out of sindex range.
        counted_t<const ql::datum_t> sindex_val; // NULL if no sindex.
        if (sindex) {
            sindex_val = sindex->func->call(job.env, val)->as_datum();
            if (sindex->multi == sindex_multi_bool_t::MULTI
                && sindex_val->get_type() == ql::datum_t::R_ARRAY) {
                boost::optional<uint64_t> tag = *ql::datum_t::extract_tag(key);
                guarantee(tag);
                sindex_val = sindex_val->get(*tag, ql::NOTHROW);
                guarantee(sindex_val);
            }
            if (!sindex->range.contains(sindex_val)) {
                return done_traversing_t::NO;
            }
        }

        ql::groups_t data = {{counted_t<const ql::datum_t>(), ql::datums_t{val}}};

        for (auto it = job.transformers.begin(); it != job.transformers.end(); ++it) {
            (**it)(&data, sindex_val);
            //            ^^^^^^^^^^ NULL if no sindex
        }
        // We need lots of extra data for the accumulation because we might be
        // accumulating `rget_item_t`s for a batch.
        return (*job.accumulator)(&data, std::move(key), std::move(sindex_val));
        //                                       NULL if no sindex ^^^^^^^^^^
    } catch (const ql::exc_t &e) {
        io.response->result = e;
        return done_traversing_t::YES;
    } catch (const ql::datum_exc_t &e) {
#ifndef NDEBUG
        unreachable();
#else
        io.response->result = ql::exc_t(e, NULL);
        return done_traversing_t::YES;
#endif // NDEBUG
    }
}

// TODO: Having two functions which are 99% the same sucks.
void rdb_rget_slice(
    btree_slice_t *slice,
    const key_range_t &range,
    superblock_t *superblock,
    ql::env_t *ql_env,
    const ql::batchspec_t &batchspec,
    const std::vector<transform_variant_t> &transforms,
    const boost::optional<terminal_variant_t> &terminal,
    sorting_t sorting,
    rget_read_response_t *response) {
    r_sanity_check(boost::get<ql::exc_t>(&response->result) == NULL);
    profile::starter_t starter("Do range scan on primary index.", ql_env->trace);
    rget_cb_t callback(
        io_data_t(response, slice),
        job_data_t(ql_env, batchspec, transforms, terminal, sorting),
        boost::optional<sindex_data_t>(),
        range);
    btree_concurrent_traversal(superblock, range, &callback,
                               (!reversed(sorting) ? FORWARD : BACKWARD));
    callback.finish();
}

void rdb_rget_secondary_slice(
    btree_slice_t *slice,
    const datum_range_t &sindex_range,
    const region_t &sindex_region,
    superblock_t *superblock,
    ql::env_t *ql_env,
    const ql::batchspec_t &batchspec,
    const std::vector<transform_variant_t> &transforms,
    const boost::optional<terminal_variant_t> &terminal,
    const key_range_t &pk_range,
    sorting_t sorting,
    const ql::map_wire_func_t &sindex_func,
    sindex_multi_bool_t sindex_multi,
    rget_read_response_t *response) {
    r_sanity_check(boost::get<ql::exc_t>(&response->result) == NULL);
    profile::starter_t starter("Do range scan on secondary index.", ql_env->trace);
    rget_cb_t callback(
        io_data_t(response, slice),
        job_data_t(ql_env, batchspec, transforms, terminal, sorting),
        sindex_data_t(pk_range, sindex_range, sindex_func, sindex_multi),
        sindex_region.inner);
    btree_concurrent_traversal(
        superblock, sindex_region.inner, &callback,
        (!reversed(sorting) ? FORWARD : BACKWARD));
    callback.finish();
}

void rdb_distribution_get(int max_depth,
                          const store_key_t &left_key,
                          superblock_t *superblock,
                          distribution_read_response_t *response) {
    int64_t key_count_out;
    std::vector<store_key_t> key_splits;
    get_btree_key_distribution(superblock, max_depth,
                               &key_count_out, &key_splits);

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

void rdb_modification_info_t::rdb_serialize(write_message_t *wm) const {
    const uint64_t ser_version = 0;
    serialize_varint_uint64(wm, ser_version);

    if (!deleted.first.get()) {
        guarantee(deleted.second.empty());
        serialize(wm, HAS_NO_VALUE);
    } else {
        serialize(wm, HAS_VALUE);
        serialize(wm, deleted);
    }

    if (!added.first.get()) {
        guarantee(added.second.empty());
        serialize(wm, HAS_NO_VALUE);
    } else {
        serialize(wm, HAS_VALUE);
        serialize(wm, added);
    }
}

archive_result_t rdb_modification_info_t::rdb_deserialize(read_stream_t *s) {
    archive_result_t res;

    uint64_t ser_version;
    res = deserialize_varint_uint64(s, &ser_version);
    if (bad(res)) { return res; }
    if (ser_version != 0) { return archive_result_t::VERSION_ERROR; }

    int8_t has_value;
    res = deserialize(s, &has_value);
    if (bad(res)) { return res; }

    if (has_value == HAS_VALUE) {
        res = deserialize(s, &deleted);
        if (bad(res)) { return res; }
    }

    res = deserialize(s, &has_value);
    if (bad(res)) { return res; }

    if (has_value == HAS_VALUE) {
        res = deserialize(s, &added);
        if (bad(res)) { return res; }
    }

    return archive_result_t::SUCCESS;
}

RDB_IMPL_ME_SERIALIZABLE_2(rdb_modification_report_t, 0, primary_key, info);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_erase_major_range_report_t, 0, range_to_erase);

rdb_modification_report_cb_t::rdb_modification_report_cb_t(
        store_t *store,
        buf_lock_t *sindex_block,
        auto_drainer_t::lock_t lock)
    : lock_(lock), store_(store),
      sindex_block_(sindex_block) {
    store_->acquire_post_constructed_sindex_superblocks_for_write(
            sindex_block_, &sindexes_);
}

rdb_modification_report_cb_t::~rdb_modification_report_cb_t() { }

void rdb_modification_report_cb_t::on_mod_report(
        const rdb_modification_report_t &mod_report) {
    scoped_ptr_t<new_mutex_in_line_t> acq =
        store_->get_in_line_for_sindex_queue(sindex_block_);

    write_message_t wm;
    serialize(&wm, rdb_sindex_change_t(mod_report));
    store_->sindex_queue_push(wm, acq.get());

    rdb_live_deletion_context_t deletion_context;
    rdb_update_sindexes(sindexes_, &mod_report, sindex_block_->txn(),
                        &deletion_context);
}

void compute_keys(const store_key_t &primary_key, counted_t<const ql::datum_t> doc,
                  ql::map_wire_func_t *mapping, sindex_multi_bool_t multi, ql::env_t *env,
                  std::vector<store_key_t> *keys_out) {
    guarantee(keys_out->empty());
    counted_t<const ql::datum_t> index =
        mapping->compile_wire_func()->call(env, doc)->as_datum();

    if (multi == sindex_multi_bool_t::MULTI && index->get_type() == ql::datum_t::R_ARRAY) {
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
        const store_t::sindex_access_t *sindex,
        const deletion_context_t *deletion_context,
        const rdb_modification_report_t *modification,
        auto_drainer_t::lock_t) {
    // Note if you get this error it's likely that you've passed in a default
    // constructed mod_report. Don't do that.  Mod reports should always be passed
    // to a function as an output parameter before they're passed to this
    // function.
    guarantee(modification->primary_key.size() != 0);

    ql::map_wire_func_t mapping;
    sindex_multi_bool_t multi = sindex_multi_bool_t::MULTI;
    inplace_vector_read_stream_t read_stream(&sindex->sindex.opaque_definition);
    archive_result_t success = deserialize(&read_stream, &mapping);
    guarantee_deserialization(success, "sindex deserialize");
    success = deserialize(&read_stream, &multi);
    guarantee_deserialization(success, "sindex deserialize");

    // TODO we just use a NULL environment here. People should not be able
    // to do anything that requires an environment like gets from other
    // tables etc. but we don't have a nice way to disallow those things so
    // for now we pass null and it will segfault if an illegal sindex
    // mapping is passed.
    cond_t non_interruptor;
    ql::env_t env(NULL, &non_interruptor);

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
                    keyvalue_location_t kv_location;
                    rdb_value_sizer_t sizer(super_block->cache()->max_block_size());
                    find_keyvalue_location_for_write(&sizer,
                                                     super_block,
                                                     it->btree_key(),
                                                     deletion_context->balancing_detacher(),
                                                     &kv_location,
                                                     &sindex->btree->stats,
                                                     env.trace.get_or_null(),
                                                     &return_superblock_local);

                    if (kv_location.value.has()) {
                        kv_location_delete(&kv_location, *it,
                            repli_timestamp_t::distant_past, deletion_context, NULL);
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
                    keyvalue_location_t kv_location;

                    rdb_value_sizer_t sizer(super_block->cache()->max_block_size());
                    find_keyvalue_location_for_write(&sizer,
                                                     super_block,
                                                     it->btree_key(),
                                                     deletion_context->balancing_detacher(),
                                                     &kv_location,
                                                     &sindex->btree->stats,
                                                     env.trace.get_or_null(),
                                                     &return_superblock_local);

                    kv_location_set(&kv_location, *it,
                                    modification->info.added.second,
                                    repli_timestamp_t::distant_past,
                                    deletion_context);
                    // The keyvalue location gets destroyed here.
                }
                super_block = return_superblock_local.wait();
            }
        } catch (const ql::base_exc_t &) {
            // Do nothing (we just drop the row from the index).
        }
    }
}

void rdb_update_sindexes(const store_t::sindex_access_vector_t &sindexes,
                         const rdb_modification_report_t *modification,
                         txn_t *txn, const deletion_context_t *deletion_context) {
    {
        auto_drainer_t drainer;

        for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
            coro_t::spawn_sometime(std::bind(
                        &rdb_update_single_sindex, &*it, deletion_context,
                        modification, auto_drainer_t::lock_t(&drainer)));
        }
    }

    /* All of the sindex have been updated now it's time to actually clear the
     * deleted blob if it exists. */
    if (modification->info.deleted.first) {
        deletion_context->post_deleter()->delete_value(buf_parent_t(txn),
                modification->info.deleted.second.data());
    }
}

void rdb_erase_major_range_sindexes(const store_t::sindex_access_vector_t &sindexes,
                                    const rdb_erase_major_range_report_t *erase_range,
                                    signal_t *interruptor, const value_deleter_t *deleter) {
    auto_drainer_t drainer;

    spawn_sindex_erase_ranges(&sindexes, erase_range->range_to_erase,
                              &drainer, auto_drainer_t::lock_t(&drainer),
                              false /* don't release the superblock */, interruptor,
                              deleter);
}

class post_construct_traversal_helper_t : public btree_traversal_helper_t {
public:
    post_construct_traversal_helper_t(
            store_t *store,
            const std::set<uuid_u> &sindexes_to_post_construct,
            cond_t *interrupt_myself,
            signal_t *interruptor
            )
        : store_(store),
          sindexes_to_post_construct_(sindexes_to_post_construct),
          interrupt_myself_(interrupt_myself), interruptor_(interruptor)
    { }

    void process_a_leaf(buf_lock_t *leaf_node_buf,
                        const btree_key_t *, const btree_key_t *,
                        signal_t *, int *) THROWS_ONLY(interrupted_exc_t) {

        // KSI: FML
        scoped_ptr_t<txn_t> wtxn;
        store_t::sindex_access_vector_t sindexes;

        buf_read_t leaf_read(leaf_node_buf);
        const leaf_node_t *leaf_node
            = static_cast<const leaf_node_t *>(leaf_read.get_data_read());

        // Number of key/value pairs we process before yielding
        const int MAX_CHUNK_SIZE = 10;
        int current_chunk_size = 0;
        const rdb_post_construction_deletion_context_t deletion_context;
        for (auto it = leaf::begin(*leaf_node); it != leaf::end(*leaf_node); ++it) {
            if (current_chunk_size == 0) {
                // Start a write transaction and acquire the secondary index
                // at the beginning of each chunk. We reset the transaction
                // after each chunk because large write transactions can cause
                // the cache to go into throttling, and that would interfere
                // with other transactions on this table.
                try {
                    write_token_pair_t token_pair;
                    store_->new_write_token_pair(&token_pair);

                    scoped_ptr_t<real_superblock_t> superblock;

                    // We use HARD durability because we want post construction
                    // to be throttled if we insert data faster than it can
                    // be written to disk. Otherwise we might exhaust the cache's
                    // dirty page limit and bring down the whole table.
                    // Other than that, the hard durability guarantee is not actually
                    // needed here.
                    store_->acquire_superblock_for_write(
                            repli_timestamp_t::distant_past,
                            2 + MAX_CHUNK_SIZE,
                            write_durability_t::HARD,
                            &token_pair,
                            &wtxn,
                            &superblock,
                            interruptor_);

                    // Acquire the sindex block.
                    const block_id_t sindex_block_id = superblock->get_sindex_block_id();

                    buf_lock_t sindex_block
                        = store_->acquire_sindex_block_for_write(superblock->expose_buf(),
                                                                 sindex_block_id);

                    superblock.reset();

                    store_->acquire_sindex_superblocks_for_write(
                            sindexes_to_post_construct_,
                            &sindex_block,
                            &sindexes);

                    if (sindexes.empty()) {
                        interrupt_myself_->pulse_if_not_already_pulsed();
                        return;
                    }
                } catch (const interrupted_exc_t &e) {
                    return;
                }
            }

            store_->btree->stats.pm_keys_read.record();

            /* Grab relevant values from the leaf node. */
            const btree_key_t *key = (*it).first;
            const void *value = (*it).second;
            guarantee(key);

            const store_key_t pk(key);
            rdb_modification_report_t mod_report(pk);
            const rdb_value_t *rdb_value = static_cast<const rdb_value_t *>(value);
            const max_block_size_t block_size = leaf_node_buf->cache()->max_block_size();
            mod_report.info.added
                = std::make_pair(
                    get_data(rdb_value, buf_parent_t(leaf_node_buf)),
                    std::vector<char>(rdb_value->value_ref(),
                        rdb_value->value_ref() + rdb_value->inline_size(block_size)));

            rdb_update_sindexes(sindexes, &mod_report, wtxn.get(), &deletion_context);
            store_->btree->stats.pm_keys_set.record();

            ++current_chunk_size;
            if (current_chunk_size >= MAX_CHUNK_SIZE) {
                current_chunk_size = 0;
                // Release the write transaction and yield.
                // We continue later where we have left off.
                sindexes.clear();
                wtxn.reset();
                coro_t::yield();
            }
        }
    }

    void postprocess_internal_node(buf_lock_t *) { }

    void filter_interesting_children(buf_parent_t,
                                     ranged_block_ids_t *ids_source,
                                     interesting_children_callback_t *cb) {
        for (int i = 0, e = ids_source->num_block_ids(); i < e; ++i) {
            cb->receive_interesting_child(i);
        }
        cb->no_more_interesting_children();
    }

    access_t btree_superblock_mode() { return access_t::read; }
    access_t btree_node_mode() { return access_t::read; }

    store_t *store_;
    const std::set<uuid_u> &sindexes_to_post_construct_;
    cond_t *interrupt_myself_;
    signal_t *interruptor_;  
};

void post_construct_secondary_indexes(
        store_t *store,
        const std::set<uuid_u> &sindexes_to_post_construct,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) {
    cond_t local_interruptor;

    wait_any_t wait_any(&local_interruptor, interruptor);

    post_construct_traversal_helper_t helper(store,
            sindexes_to_post_construct, &local_interruptor, interruptor);
    /* Notice the ordering of progress_tracker and insertion_sentries matters.
     * insertion_sentries puts pointers in the progress tracker map. Once
     * insertion_sentries is destructed nothing has a reference to
     * progress_tracker so we know it's safe to destruct it. */
    parallel_traversal_progress_t progress_tracker;
    helper.progress = &progress_tracker;

    std::vector<map_insertion_sentry_t<uuid_u, const parallel_traversal_progress_t *> >
        insertion_sentries(sindexes_to_post_construct.size());
    auto sentry = insertion_sentries.begin();
    for (auto it = sindexes_to_post_construct.begin();
         it != sindexes_to_post_construct.end(); ++it) {
        store->add_progress_tracker(&*sentry, *it, &progress_tracker);
    }

    object_buffer_t<fifo_enforcer_sink_t::exit_read_t> read_token;
    store->new_read_token(&read_token);

    // Mind the destructor ordering.
    // The superblock must be released before txn (`btree_parallel_traversal`
    // usually already takes care of that).
    // The txn must be destructed before the cache_account.
    cache_account_t cache_account;
    scoped_ptr_t<txn_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;

    store->acquire_superblock_for_read(
        &read_token,
        &txn,
        &superblock,
        interruptor,
        true /* USE_SNAPSHOT */);

    cache_account
        = txn->cache()->create_cache_account(SINDEX_POST_CONSTRUCTION_CACHE_PRIORITY);
    txn->set_account(&cache_account);

    btree_parallel_traversal(superblock.get(), &helper, &wait_any);
}
