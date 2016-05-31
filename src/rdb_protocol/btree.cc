// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/btree.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <set>
#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "btree/concurrent_traversal.hpp"
#include "btree/get_distribution.hpp"
#include "btree/operations.hpp"
#include "btree/reql_specific.hpp"
#include "btree/superblock.hpp"
#include "buffer_cache/serialize_onto_blob.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/new_mutex.hpp"
#include "concurrency/queue/unlimited_fifo.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/buffer_group_stream.hpp"
#include "containers/archive/buffer_stream.hpp"
#include "containers/scoped.hpp"
#include "rdb_protocol/geo/exceptions.hpp"
#include "rdb_protocol/geo/indexing.hpp"
#include "rdb_protocol/blob_wrapper.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/geo_traversal.hpp"
#include "rdb_protocol/lazy_btree_val.hpp"
#include "rdb_protocol/pseudo_geometry.hpp"
#include "rdb_protocol/serialize_datum_onto_blob.hpp"
#include "rdb_protocol/shards.hpp"
#include "rdb_protocol/table_common.hpp"

#include "debug.hpp"

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
        response->data = ql::datum_t::null();
    } else {
        response->data = get_data(static_cast<rdb_value_t *>(kv_location.value.get()),
                                  buf_parent_t(&kv_location.buf));
    }
}

void kv_location_delete(keyvalue_location_t *kv_location,
                        const store_key_t &key,
                        repli_timestamp_t timestamp,
                        const deletion_context_t *deletion_context,
                        delete_mode_t delete_mode,
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
            deletion_context->balancing_detacher(), &null_cb, delete_mode);
}

MUST_USE ql::serialization_result_t
kv_location_set(keyvalue_location_t *kv_location,
                const store_key_t &key,
                ql::datum_t data,
                repli_timestamp_t timestamp,
                const deletion_context_t *deletion_context,
                rdb_modification_info_t *mod_info_out) THROWS_NOTHING {
    scoped_malloc_t<rdb_value_t> new_value(blob::btree_maxreflen);
    memset(new_value.get(), 0, blob::btree_maxreflen);

    const max_block_size_t block_size = kv_location->buf.cache()->max_block_size();
    {
        blob_t blob(block_size, new_value->value_ref(), blob::btree_maxreflen);
        ql::serialization_result_t res
            = datum_serialize_onto_blob(buf_parent_t(&kv_location->buf),
                                        &blob, data);
        if (bad(res)) return res;
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
                          deletion_context->balancing_detacher(), &null_cb,
                          delete_mode_t::REGULAR_QUERY);
    return ql::serialization_result_t::SUCCESS;
}

MUST_USE ql::serialization_result_t
kv_location_set(keyvalue_location_t *kv_location,
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
            value_ref.data(), value_ref.size());

    // Update the leaf, if needed.
    kv_location->value = std::move(new_value);

    null_key_modification_callback_t null_cb;
    rdb_value_sizer_t sizer(kv_location->buf.cache()->max_block_size());
    apply_keyvalue_change(&sizer, kv_location, key.btree_key(), timestamp,
                          deletion_context->balancing_detacher(), &null_cb,
                          delete_mode_t::REGULAR_QUERY);
    return ql::serialization_result_t::SUCCESS;
}

batched_replace_response_t rdb_replace_and_return_superblock(
    const btree_loc_info_t &info,
    const btree_point_replacer_t *replacer,
    const deletion_context_t *deletion_context,
    promise_t<superblock_t *> *superblock_promise,
    rdb_modification_info_t *mod_info_out,
    profile::trace_t *trace) {
    const return_changes_t return_changes = replacer->should_return_changes();
    const datum_string_t &primary_key = info.btree->primary_key;
    const store_key_t &key = *info.key;

    try {
        keyvalue_location_t kv_location;
        rdb_value_sizer_t sizer(info.superblock->cache()->max_block_size());
        find_keyvalue_location_for_write(&sizer, info.superblock,
                                         info.key->btree_key(),
                                         info.btree->timestamp,
                                         deletion_context->balancing_detacher(),
                                         &kv_location,
                                         trace,
                                         superblock_promise);
        info.btree->slice->stats.pm_keys_set.record();
        info.btree->slice->stats.pm_total_keys_set += 1;

        ql::datum_t old_val;
        if (!kv_location.value.has()) {
            // If there's no entry with this key, pass NULL to the function.
            old_val = ql::datum_t::null();
        } else {
            // Otherwise pass the entry with this key to the function.
            old_val = get_data(kv_location.value_as<rdb_value_t>(),
                               buf_parent_t(&kv_location.buf));
            guarantee(old_val.get_field(primary_key, ql::NOTHROW).has());
        }
        guarantee(old_val.has());

        ql::datum_t new_val;
        try {
            /* Compute the replacement value for the row */
            new_val = replacer->replace(old_val);

            /* Validate the replacement value and generate a stats object to return to
            the user, but don't return it yet if we need to make changes. The reason for
            this odd order is that we need to validate the change before we write the
            change. */
            rcheck_row_replacement(primary_key, key, old_val, new_val);
            bool was_changed;
            ql::datum_t resp = make_row_replacement_stats(
                primary_key, key, old_val, new_val, return_changes, &was_changed);
            if (!was_changed) {
                return resp;
            }

            /* Now that the change has passed validation, write it to disk */
            if (new_val.get_type() == ql::datum_t::R_NULL) {
                kv_location_delete(&kv_location, *info.key, info.btree->timestamp,
                                   deletion_context, delete_mode_t::REGULAR_QUERY,
                                   mod_info_out);
            } else {
                r_sanity_check(new_val.get_field(primary_key, ql::NOTHROW).has());
                ql::serialization_result_t res =
                    kv_location_set(&kv_location, *info.key, new_val,
                                    info.btree->timestamp, deletion_context,
                                    mod_info_out);
                if (res & ql::serialization_result_t::ARRAY_TOO_BIG) {
                    rfail_typed_target(&new_val, "Array too large for disk writes "
                                       "(limit 100,000 elements).");
                } else if (res & ql::serialization_result_t::EXTREMA_PRESENT) {
                    rfail_typed_target(&new_val, "`r.minval` and `r.maxval` cannot be "
                                       "written to disk.");
                }
                r_sanity_check(!ql::bad(res));
            }

            /* Report the changes for sindex and change-feed purposes */
            if (old_val.get_type() != ql::datum_t::R_NULL) {
                guarantee(!mod_info_out->deleted.second.empty());
                mod_info_out->deleted.first = old_val;
            } else {
                guarantee(mod_info_out->deleted.second.empty());
            }
            if (new_val.get_type() != ql::datum_t::R_NULL) {
                guarantee(!mod_info_out->added.second.empty());
                mod_info_out->added.first = new_val;
            } else {
                guarantee(mod_info_out->added.second.empty());
            }

            return resp;

        } catch (const ql::base_exc_t &e) {
            return make_row_replacement_error_stats(old_val,
                                                    new_val,
                                                    return_changes,
                                                    e.what());
        }
    } catch (const interrupted_exc_t &e) {
        ql::datum_object_builder_t object_builder;
        std::string msg = strprintf("interrupted (%s:%d)", __FILE__, __LINE__);
        object_builder.add_error(msg.c_str());
        // We don't rethrow because we're in a coroutine.  Theoretically the
        // above message should never make it back to a user because the calling
        // function will also be interrupted, but we document where it comes
        // from to aid in future debugging if that invariant becomes violated.
        return std::move(object_builder).to_datum();
    }
}


class one_replace_t : public btree_point_replacer_t {
public:
    one_replace_t(const btree_batched_replacer_t *_replacer, size_t _index)
        : replacer(_replacer), index(_index) { }

    ql::datum_t replace(const ql::datum_t &d) const {
        return replacer->replace(d, index);
    }
    return_changes_t should_return_changes() const { return replacer->should_return_changes(); }
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
    const ql::configured_limits_t &limits,
    promise_t<superblock_t *> *superblock_promise,
    rdb_modification_report_cb_t *mod_cb,
    bool update_pkey_cfeeds,
    batched_replace_response_t *stats_out,
    profile::trace_t *trace,
    std::set<std::string> *conditions) {

    fifo_enforcer_sink_t::exit_write_t exiter(
        batched_replaces_fifo_sink, batched_replaces_fifo_token);
    // We need to get in line for this while still holding the superblock so
    // that stamp read operations can't queue-skip.
    info.superblock->get()->write_acq_signal()->wait_lazily_unordered();
    rwlock_in_line_t stamp_spot = mod_cb->get_in_line_for_cfeed_stamp();

    rdb_live_deletion_context_t deletion_context;
    rdb_modification_report_t mod_report(*info.key);
    ql::datum_t res = rdb_replace_and_return_superblock(
        info, &one_replace, &deletion_context, superblock_promise, &mod_report.info,
        trace);
    *stats_out = (*stats_out).merge(res, ql::stats_merge, limits, conditions);

    // We wait to make sure we acquire `acq` in the same order we were
    // originally called.
    exiter.wait();
    new_mutex_in_line_t sindex_spot = mod_cb->get_in_line_for_sindex();

    mod_cb->on_mod_report(
        mod_report, update_pkey_cfeeds, &sindex_spot, &stamp_spot);
}

batched_replace_response_t rdb_batched_replace(
    const btree_info_t &info,
    scoped_ptr_t<real_superblock_t> *superblock,
    const std::vector<store_key_t> &keys,
    const btree_batched_replacer_t *replacer,
    rdb_modification_report_cb_t *sindex_cb,
    ql::configured_limits_t limits,
    profile::sampler_t *sampler,
    profile::trace_t *trace) {

    fifo_enforcer_source_t source;
    fifo_enforcer_sink_t sink;

    ql::datum_t stats = ql::datum_t::empty_object();

    std::set<std::string> conditions;

    // We have to drain write operations before destructing everything above us,
    // because the coroutines being drained use them.
    {
        // We must disable profiler events for subtasks, because multiple instances
        // of `handle_pair`are going to run in parallel which  would otherwise corrupt
        // the sequence of events in the profiler trace.
        // Instead we add a single event for the whole batched replace.
        sampler->new_sample();
        PROFILE_STARTER_IF_ENABLED(
            trace != nullptr,
            "Perform parallel replaces.",
            trace);
        profile::disabler_t trace_disabler(trace);
        unlimited_fifo_queue_t<std::function<void()> > coro_queue;
        struct callback_t : public coro_pool_callback_t<std::function<void()> > {
            virtual void coro_pool_callback(std::function<void()> f, signal_t *) {
                f();
            }
        } callback;
        const size_t MAX_CONCURRENT_REPLACES = 8;
        coro_pool_t<std::function<void()> > coro_pool(
            MAX_CONCURRENT_REPLACES, &coro_queue, &callback);
        // We release the superblock either before or after draining on all the
        // write operations depending on the presence of limit changefeeds.
        scoped_ptr_t<real_superblock_t> current_superblock(superblock->release());
        bool update_pkey_cfeeds = sindex_cb->has_pkey_cfeeds(keys);
        {
            auto_drainer_t drainer;
            for (size_t i = 0; i < keys.size(); ++i) {
                promise_t<superblock_t *> superblock_promise;
                coro_queue.push(
                    std::bind(
                        &do_a_replace_from_batched_replace,
                        auto_drainer_t::lock_t(&drainer),
                        &sink,
                        source.enter_write(),
                        btree_loc_info_t(&info, current_superblock.release(), &keys[i]),
                        one_replace_t(replacer, i),
                        limits,
                        &superblock_promise,
                        sindex_cb,
                        update_pkey_cfeeds,
                        &stats,
                        trace,
                        &conditions));
                current_superblock.init(
                    static_cast<real_superblock_t *>(superblock_promise.wait()));
            }
            if (!update_pkey_cfeeds) {
                current_superblock.reset(); // Release the superblock early if
                                            // we don't need to finish.
            }
        }
        // This needs to happen after draining.
        if (update_pkey_cfeeds) {
            guarantee(current_superblock.has());
            sindex_cb->finish(info.slice, current_superblock.get());
        }
    }

    ql::datum_object_builder_t out(stats);
    out.add_warnings(conditions, limits);
    return std::move(out).to_datum();
}

void rdb_set(const store_key_t &key,
             ql::datum_t data,
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
    find_keyvalue_location_for_write(&sizer, superblock, key.btree_key(), timestamp,
                                     deletion_context->balancing_detacher(),
                                     &kv_location, trace, pass_back_superblock);
    slice->stats.pm_keys_set.record();
    slice->stats.pm_total_keys_set += 1;
    const bool had_value = kv_location.value.has();

    /* update the modification report */
    if (kv_location.value.has()) {
        mod_info->deleted.first = get_data(kv_location.value_as<rdb_value_t>(),
                                           buf_parent_t(&kv_location.buf));
    }

    mod_info->added.first = data;

    if (overwrite || !had_value) {
        ql::serialization_result_t res =
            kv_location_set(&kv_location, key, data, timestamp, deletion_context,
                            mod_info);
        if (res & ql::serialization_result_t::ARRAY_TOO_BIG) {
            rfail_typed_target(&data, "Array too large for disk writes "
                               "(limit 100,000 elements).");
        } else if (res & ql::serialization_result_t::EXTREMA_PRESENT) {
            rfail_typed_target(&data, "`r.minval` and `r.maxval` cannot be "
                               "written to disk.");
        }
        r_sanity_check(!ql::bad(res));
        guarantee(mod_info->deleted.second.empty() == !had_value &&
                  !mod_info->added.second.empty());
    }
    response_out->result =
        (had_value ? point_write_result_t::DUPLICATE : point_write_result_t::STORED);
}

void rdb_delete(const store_key_t &key, btree_slice_t *slice,
                repli_timestamp_t timestamp,
                real_superblock_t *superblock,
                const deletion_context_t *deletion_context,
                delete_mode_t delete_mode,
                point_delete_response_t *response,
                rdb_modification_info_t *mod_info,
                profile::trace_t *trace,
                promise_t<superblock_t *> *pass_back_superblock) {
    keyvalue_location_t kv_location;
    rdb_value_sizer_t sizer(superblock->cache()->max_block_size());
    find_keyvalue_location_for_write(&sizer, superblock, key.btree_key(), timestamp,
            deletion_context->balancing_detacher(), &kv_location, trace,
            pass_back_superblock);
    slice->stats.pm_keys_set.record();
    slice->stats.pm_total_keys_set += 1;
    bool exists = kv_location.value.has();

    /* Update the modification report. */
    if (exists) {
        mod_info->deleted.first = get_data(kv_location.value_as<rdb_value_t>(),
                                           buf_parent_t(&kv_location.buf));
        kv_location_delete(&kv_location, key, timestamp, deletion_context,
            delete_mode, mod_info);
        guarantee(!mod_info->deleted.second.empty() && mod_info->added.second.empty());
    }
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

typedef ql::transform_variant_t transform_variant_t;
typedef ql::terminal_variant_t terminal_variant_t;

class rget_sindex_data_t {
public:
    rget_sindex_data_t(key_range_t _pkey_range,
                       ql::datumspec_t _datumspec,
                       key_range_t *_active_region_range_inout,
                       reql_version_t wire_func_reql_version,
                       ql::map_wire_func_t wire_func,
                       sindex_multi_bool_t _multi)
        : pkey_range(std::move(_pkey_range)),
          datumspec(std::move(_datumspec)),
          active_region_range_inout(_active_region_range_inout),
          func_reql_version(wire_func_reql_version),
          func(wire_func.compile_wire_func()),
          multi(_multi) {
        datumspec.visit<void>(
            [&](const ql::datum_range_t &r) {
                lbound_trunc_key = r.get_left_bound_trunc_key(func_reql_version);
                rbound_trunc_key = r.get_right_bound_trunc_key(func_reql_version);
            },
            [](const std::map<ql::datum_t, uint64_t> &) { });
    }
private:
    friend class rget_cb_t;
    const key_range_t pkey_range;
    const ql::datumspec_t datumspec;
    key_range_t *active_region_range_inout;
    const reql_version_t func_reql_version;
    const counted_t<const ql::func_t> func;
    const sindex_multi_bool_t multi;
    // The (truncated) boundary keys for the datum range stored in `datumspec`.
    std::string lbound_trunc_key;
    std::string rbound_trunc_key;
};

class job_data_t {
public:
    job_data_t(ql::env_t *_env,
               const ql::batchspec_t &batchspec,
               const std::vector<transform_variant_t> &_transforms,
               const boost::optional<terminal_variant_t> &_terminal,
               region_t region,
               store_key_t last_key,
               sorting_t _sorting,
               require_sindexes_t require_sindex_val)
        : env(_env),
          batcher(make_scoped<ql::batcher_t>(batchspec.to_batcher())),
          sorting(_sorting),
          accumulator(_terminal
                      ? ql::make_terminal(*_terminal)
                      : ql::make_append(std::move(region),
                                        std::move(last_key),
                                        sorting,
                                        batcher.get(),
                                        require_sindex_val)) {
        for (size_t i = 0; i < _transforms.size(); ++i) {
            transformers.push_back(ql::make_op(_transforms[i]));
        }
        guarantee(transformers.size() == _transforms.size());
    }
    job_data_t(job_data_t &&) = default;

    bool should_send_batch() const {
        return accumulator->should_send_batch();
    }
private:
    friend class rget_cb_t;
    ql::env_t *const env;
    scoped_ptr_t<ql::batcher_t> batcher;
    std::vector<scoped_ptr_t<ql::op_t> > transformers;
    sorting_t sorting;
    scoped_ptr_t<ql::accumulator_t> accumulator;
};

class rget_io_data_t {
public:
    rget_io_data_t(rget_read_response_t *_response, btree_slice_t *_slice)
        : response(_response), slice(_slice) { }
private:
    friend class rget_cb_t;
    rget_read_response_t *const response;
    btree_slice_t *const slice;
};


class rget_cb_t {
public:
    rget_cb_t(rget_io_data_t &&_io,
              job_data_t &&_job,
              boost::optional<rget_sindex_data_t> &&_sindex);

    continue_bool_t handle_pair(
        scoped_key_value_t &&keyvalue,
        size_t default_copies,
        const boost::optional<std::string> &skey_left,
        concurrent_traversal_fifo_enforcer_signal_t waiter)
        THROWS_ONLY(interrupted_exc_t);
    void finish(continue_bool_t last_cb) THROWS_ONLY(interrupted_exc_t);
private:
    const rget_io_data_t io; // How do get data in/out.
    job_data_t job; // What to do next (stateful).
    const boost::optional<rget_sindex_data_t> sindex; // Optional sindex information.

    scoped_ptr_t<ql::env_t> sindex_env;

    // State for internal bookkeeping.
    bool bad_init;
    boost::optional<std::string> last_truncated_secondary_for_abort;
    scoped_ptr_t<profile::disabler_t> disabler;
    scoped_ptr_t<profile::sampler_t> sampler;
};

// This is the interface the btree code expects, but our actual callback needs a
// little bit more so we use this wrapper to hold the extra information.
class rget_cb_wrapper_t : public concurrent_traversal_callback_t {
public:
    rget_cb_wrapper_t(
            rget_cb_t *_cb,
            size_t _copies,
            boost::optional<std::string> _skey_left)
        : cb(_cb), copies(_copies), skey_left(std::move(_skey_left)) { }
    virtual continue_bool_t handle_pair(
        scoped_key_value_t &&keyvalue,
        concurrent_traversal_fifo_enforcer_signal_t waiter)
        THROWS_ONLY(interrupted_exc_t) {
        return cb->handle_pair(
            std::move(keyvalue),
            copies,
            skey_left,
            std::move(waiter));
    }
private:
    rget_cb_t *cb;
    size_t copies;
    boost::optional<std::string> skey_left;
};

rget_cb_t::rget_cb_t(rget_io_data_t &&_io,
                     job_data_t &&_job,
                     boost::optional<rget_sindex_data_t> &&_sindex)
    : io(std::move(_io)),
      job(std::move(_job)),
      sindex(std::move(_sindex)),
      bad_init(false) {

    if (sindex) {
        // Secondary index functions are deterministic (so no need for an
        // rdb_context_t) and evaluated in a pristine environment (without global
        // optargs).
        sindex_env.init(new ql::env_t(job.env->interruptor,
                                      ql::return_empty_normal_batches_t::NO,
                                      sindex->func_reql_version));
    }

    // We must disable profiler events for subtasks, because multiple instances
    // of `handle_pair`are going to run in parallel which  would otherwise corrupt
    // the sequence of events in the profiler trace.
    disabler.init(new profile::disabler_t(job.env->trace));
    sampler.init(new profile::sampler_t("Range traversal doc evaluation.",
                                        job.env->trace));
}

void rget_cb_t::finish(continue_bool_t last_cb) THROWS_ONLY(interrupted_exc_t) {
    job.accumulator->finish(last_cb, &io.response->result);
}

// Handle a keyvalue pair.  Returns whether or not we're done early.
continue_bool_t rget_cb_t::handle_pair(
    scoped_key_value_t &&keyvalue,
    size_t default_copies,
    const boost::optional<std::string> &skey_left,
    concurrent_traversal_fifo_enforcer_signal_t waiter)
    THROWS_ONLY(interrupted_exc_t) {

    //////////////////////////////////////////////////
    // STUFF THAT CAN HAPPEN OUT OF ORDER GOES HERE //
    //////////////////////////////////////////////////
    sampler->new_sample();
    if (bad_init || boost::get<ql::exc_t>(&io.response->result) != NULL) {
        return continue_bool_t::ABORT;
    }
    // Load the key and value.
    store_key_t key(keyvalue.key());
    if (sindex && !sindex->pkey_range.contains_key(ql::datum_t::extract_primary(key))) {
        return continue_bool_t::CONTINUE;
    }
    lazy_btree_val_t row(static_cast<const rdb_value_t *>(keyvalue.value()),
                         keyvalue.expose_buf());
    ql::datum_t val;
    // Count stats whether or not we deserialize the value
    io.slice->stats.pm_keys_read.record();
    io.slice->stats.pm_total_keys_read += 1;
    // We only load the value if we actually use it (`count` does not).
    if (job.accumulator->uses_val() || job.transformers.size() != 0 || sindex) {
        val = row.get();
    } else {
        row.reset();
    }
    guarantee(!row.references_parent());
    keyvalue.reset();
    waiter.wait_interruptible(); // This enforces ordering.

    ///////////////////////////////////////////////////////
    // STUFF THAT HAS TO HAPPEN IN ORDER GOES BELOW HERE //
    ///////////////////////////////////////////////////////

    // If the sindex portion of the key is long enough that it might be >= the
    // length of a truncated sindex, we need to rember the key so we can make
    // sure not to stop in the middle of a sindex range where some of the values
    // are out of order because of truncation.
    bool remember_key_for_sindex_batching = sindex
        ? (ql::datum_t::extract_secondary(key_to_unescaped_str(key)).size()
           >= ql::datum_t::max_trunc_size())
        : false;
    if (last_truncated_secondary_for_abort) {
        std::string cur_truncated_secondary =
            ql::datum_t::extract_truncated_secondary(key_to_unescaped_str(key));
        if (cur_truncated_secondary != *last_truncated_secondary_for_abort) {
            // The semantics here are that we're returning the "last considered
            // key", which we set specially here to preserve the invariant that
            // unsharding either consumes all rows with a particular truncated
            // sindex value or none of them.
            store_key_t stop_key;
            if (!reversed(job.sorting)) {
                stop_key = store_key_t(cur_truncated_secondary);
            } else {
                stop_key = store_key_t(*last_truncated_secondary_for_abort);
            }
            stop_key.decrement();
            job.accumulator->stop_at_boundary(std::move(stop_key));
            return continue_bool_t::ABORT;
        }
    }

    try {
        // Update the active region range.
        if (sindex) {
            if (!reversed(job.sorting)) {
                if (sindex->active_region_range_inout->left < key) {
                    sindex->active_region_range_inout->left = key;
                    sindex->active_region_range_inout->left.increment();
                }
            } else {
                if (key < sindex->active_region_range_inout->right.key_or_max()) {
                    sindex->active_region_range_inout->right =
                        key_range_t::right_bound_t(key);
                }
            }
        }

        // There are certain transformations and accumulators that need the
        // secondary index value, though many don't. We don't want to compute
        // it if we don't end up needing it, because that would be expensive.
        // So we provide a function that computes the secondary index value
        // lazily the first time it's called.
        ql::datum_t sindex_val_cache; // an empty `datum_t` until initialized
        auto lazy_sindex_val = [&]() -> ql::datum_t {
            if (sindex && !sindex_val_cache.has()) {
                sindex_val_cache =
                    sindex->func->call(sindex_env.get(), val)->as_datum();
                if (sindex->multi == sindex_multi_bool_t::MULTI
                    && sindex_val_cache.get_type() == ql::datum_t::R_ARRAY) {
                    boost::optional<uint64_t> tag = *ql::datum_t::extract_tag(key);
                    guarantee(tag);
                    sindex_val_cache = sindex_val_cache.get(*tag, ql::NOTHROW);
                    guarantee(sindex_val_cache.has());
                }
            }
            return sindex_val_cache;
        };

        // Check whether we're outside the sindex range.
        // We only need to check this if we are on the boundary of the sindex range, and
        // the involved keys are truncated.
        size_t copies = default_copies;
        if (sindex) {
            /* Here's an attempt at explaining the different case distinctions handled in
               this check (for the left bound; the right bound check is similar):
               The case distinctions are as follows:
               1. left_bound_is_truncated
                If the left bound key had to be truncated, we first compare the prefix of
                the current secondary key (skey_current), and the left bound key.
                The comparison cannot be -1, because that would mean that we computed the
                traversal key range incorrectly in the first place (there's no need to
                consider keys that are *smaller* than the left bound).
                If the comparison is 1, the current key's secondary part is larger than
                the left bound, and we know that the corresponding datum_t value must
                also be larger than the datum_t corresponding to the left bound.
                Finally, since the left bound is truncated, the comparison can determine
                that the prefix is equal for values in the btree with corresponding index
                values that are either left of the bound (but match in the truncated
                prefix), at the bound (which we want to include only if the left bound is
                closed), or right of the bound (which we always want to include, as far
                as the left bound id concerned). We can't determine which case we have,
                by looking only at the keys. Hence we must check the number of copies for
                `cmp == 0`. The only exception is if the current key was actually not
                truncated, in which case we know that it will actually be smaller than
                the left bound (that's encoded in line 825).
               2. !left_bound_is_truncated && left_bound is closed
                If the bound wasn't truncated, we know that the traversal range will not
                include any values which are smaller than the left bound. Hence we can
                skip the check for whether the sindex value is actually in the datum
                range.
               3. !left_bound_is_truncated && left_bound is open
                In contrast, if the left bound is open, we compare the left bound and
                current key. If they have the same size and their contents compare equal,
                we actually know that they are outside the range and could set the number
                of copies to 0. We do the slightly less optimal but simpler thing and
                just check the number of copies in this case, so that we can share the
                code path with case 1. */
            const size_t max_trunc_size = ql::datum_t::max_trunc_size();
            sindex->datumspec.visit<void>(
            [&](const ql::datum_range_t &r) {
                bool must_check_copies = false;
                std::string skey_current =
                    ql::datum_t::extract_truncated_secondary(key_to_unescaped_str(key));
                const bool left_bound_is_truncated =
                    sindex->lbound_trunc_key.size() == max_trunc_size;
                if (left_bound_is_truncated
                    || r.left_bound_type == key_range_t::bound_t::open) {
                    int cmp = memcmp(
                        skey_current.data(),
                        sindex->lbound_trunc_key.data(),
                        std::min<size_t>(skey_current.size(),
                                         sindex->lbound_trunc_key.size()));
                    if (skey_current.size() < sindex->lbound_trunc_key.size()) {
                        guarantee(cmp != 0);
                    }
                    guarantee(cmp >= 0);
                    if (cmp == 0
                        && skey_current.size() == sindex->lbound_trunc_key.size()) {
                        must_check_copies = true;
                    }
                }
                if (!must_check_copies) {
                    const bool right_bound_is_truncated =
                        sindex->rbound_trunc_key.size() == max_trunc_size;
                    if (right_bound_is_truncated
                        || r.right_bound_type == key_range_t::bound_t::open) {
                        int cmp = memcmp(
                            skey_current.data(),
                            sindex->rbound_trunc_key.data(),
                            std::min<size_t>(skey_current.size(),
                                             sindex->rbound_trunc_key.size()));
                        if (skey_current.size() > sindex->rbound_trunc_key.size()) {
                            guarantee(cmp != 0);
                        }
                        guarantee(cmp <= 0);
                        if (cmp == 0
                            && skey_current.size() == sindex->rbound_trunc_key.size()) {
                            must_check_copies = true;
                        }
                    }
                }
                if (must_check_copies) {
                    copies = sindex->datumspec.copies(lazy_sindex_val());
                } else {
                    copies = 1;
                }
            },
            [&](const std::map<ql::datum_t, uint64_t> &) {
                guarantee(skey_left);
                std::string skey_current =
                    ql::datum_t::extract_secondary(key_to_unescaped_str(key));
                const bool skey_current_is_truncated =
                    skey_current.size() >= max_trunc_size;
                const bool skey_left_is_truncated = skey_left->size() >= max_trunc_size;

                if (skey_current_is_truncated || skey_left_is_truncated) {
                    copies = sindex->datumspec.copies(lazy_sindex_val());
                } else if (*skey_left != skey_current) {
                    copies = 0;
                }
            });
            if (copies == 0) {
                return continue_bool_t::CONTINUE;
            }
        }

        ql::groups_t data = {{ql::datum_t(), ql::datums_t(copies, val)}};

        for (auto it = job.transformers.begin(); it != job.transformers.end(); ++it) {
            (**it)(job.env, &data, lazy_sindex_val);
        }
        // We need lots of extra data for the accumulation because we might be
        // accumulating `rget_item_t`s for a batch.
        continue_bool_t cont = (*job.accumulator)(job.env, &data, key, lazy_sindex_val);
        if (remember_key_for_sindex_batching) {
            if (cont == continue_bool_t::ABORT) {
                last_truncated_secondary_for_abort =
                    ql::datum_t::extract_truncated_secondary(key_to_unescaped_str(key));
            }
            return continue_bool_t::CONTINUE;
        } else {
            return cont;
        }
    } catch (const ql::exc_t &e) {
        io.response->result = e;
        return continue_bool_t::ABORT;
    } catch (const ql::datum_exc_t &e) {
#ifndef NDEBUG
        unreachable();
#else
        io.response->result = ql::exc_t(e, ql::backtrace_id_t::empty());
        return continue_bool_t::ABORT;
#endif // NDEBUG
    }
}

// TODO: Having two functions which are 99% the same sucks.
void rdb_rget_slice(
        btree_slice_t *slice,
        const region_t &shard,
        const key_range_t &range,
        const boost::optional<std::map<store_key_t, uint64_t> > &primary_keys,
        superblock_t *superblock,
        ql::env_t *ql_env,
        const ql::batchspec_t &batchspec,
        const std::vector<transform_variant_t> &transforms,
        const boost::optional<terminal_variant_t> &terminal,
        sorting_t sorting,
        rget_read_response_t *response,
        release_superblock_t release_superblock) {
    r_sanity_check(boost::get<ql::exc_t>(&response->result) == NULL);
    PROFILE_STARTER_IF_ENABLED(
        ql_env->profile() == profile_bool_t::PROFILE,
        "Do range scan on primary index.",
        ql_env->trace);

    rget_cb_t callback(
        rget_io_data_t(response, slice),
        job_data_t(ql_env,
                   batchspec,
                   transforms,
                   terminal,
                   shard,
                   !reversed(sorting)
                       ? range.left
                       : range.right.key_or_max(),
                   sorting,
                   require_sindexes_t::NO),
        boost::none);

    direction_t direction = reversed(sorting) ? BACKWARD : FORWARD;
    continue_bool_t cont = continue_bool_t::CONTINUE;
    if (primary_keys) {
        auto cb = [&](const std::pair<store_key_t, uint64_t> &pair, bool is_last) {
            rget_cb_wrapper_t wrapper(&callback, pair.second, boost::none);
            return btree_concurrent_traversal(
                superblock,
                key_range_t::one_key(pair.first),
                &wrapper,
                direction,
                is_last ? release_superblock : release_superblock_t::KEEP);
        };
        if (!reversed(sorting)) {
            for (auto it = primary_keys->begin(); it != primary_keys->end();) {
                auto this_it = it++;
                if (cb(*this_it, it == primary_keys->end()) == continue_bool_t::ABORT) {
                    // If required the superblock will get released further up the stack.
                    cont = continue_bool_t::ABORT;
                    break;
                }
            }
        } else {
            for (auto it = primary_keys->rbegin(); it != primary_keys->rend();) {
                auto this_it = it++;
                if (cb(*this_it, it == primary_keys->rend()) == continue_bool_t::ABORT) {
                    // If required the superblock will get released further up the stack.
                    cont = continue_bool_t::ABORT;
                    break;
                }
            }
        }
    } else {
        rget_cb_wrapper_t wrapper(&callback, 1, boost::none);
        cont = btree_concurrent_traversal(
            superblock, range, &wrapper, direction, release_superblock);
    }
    callback.finish(cont);
}

void rdb_rget_secondary_slice(
        btree_slice_t *slice,
        const region_t &shard,
        const ql::datumspec_t &datumspec,
        const key_range_t &sindex_region_range,
        sindex_superblock_t *superblock,
        ql::env_t *ql_env,
        const ql::batchspec_t &batchspec,
        const std::vector<transform_variant_t> &transforms,
        const boost::optional<terminal_variant_t> &terminal,
        const key_range_t &pk_range,
        sorting_t sorting,
        require_sindexes_t require_sindex_val,
        const sindex_disk_info_t &sindex_info,
        rget_read_response_t *response,
        release_superblock_t release_superblock) {
    r_sanity_check(boost::get<ql::exc_t>(&response->result) == NULL);
    guarantee(sindex_info.geo == sindex_geo_bool_t::REGULAR);
    PROFILE_STARTER_IF_ENABLED(
        ql_env->profile() == profile_bool_t::PROFILE,
        "Do range scan on secondary index.",
        ql_env->trace);

    const reql_version_t sindex_func_reql_version =
        sindex_info.mapping_version_info.latest_compatible_reql_version;

    key_range_t active_region_range = sindex_region_range;
    rget_cb_t callback(
        rget_io_data_t(response, slice),
        job_data_t(ql_env,
                   batchspec,
                   transforms,
                   terminal,
                   shard,
                   !reversed(sorting)
                       ? sindex_region_range.left
                       : sindex_region_range.right.key_or_max(),
                   sorting,
                   require_sindex_val),
        rget_sindex_data_t(
            pk_range,
            datumspec,
            &active_region_range,
            sindex_func_reql_version,
            sindex_info.mapping,
            sindex_info.multi));

    direction_t direction = reversed(sorting) ? BACKWARD : FORWARD;
    auto cb = [&](const std::pair<ql::datum_range_t, uint64_t> &pair, bool is_last) {
        key_range_t sindex_keyrange =
            pair.first.to_sindex_keyrange(sindex_func_reql_version);
        rget_cb_wrapper_t wrapper(
            &callback,
            pair.second,
            key_to_unescaped_str(sindex_keyrange.left));
        key_range_t active_range = active_region_range.intersection(sindex_keyrange);
        // This can happen sometimes with truncated keys.
        if (active_range.is_empty()) return continue_bool_t::CONTINUE;
        return btree_concurrent_traversal(
            superblock,
            active_range,
            &wrapper,
            direction,
            is_last ? release_superblock : release_superblock_t::KEEP);
    };
    continue_bool_t cont = datumspec.iter(sorting, cb);
    callback.finish(cont);
}

void rdb_get_intersecting_slice(
        btree_slice_t *slice,
        const region_t &shard,
        const ql::datum_t &query_geometry,
        const key_range_t &sindex_range,
        sindex_superblock_t *superblock,
        ql::env_t *ql_env,
        const ql::batchspec_t &batchspec,
        const std::vector<ql::transform_variant_t> &transforms,
        const boost::optional<ql::terminal_variant_t> &terminal,
        const key_range_t &pk_range,
        const sindex_disk_info_t &sindex_info,
        is_stamp_read_t is_stamp_read,
        rget_read_response_t *response) {
    guarantee(query_geometry.has());

    guarantee(sindex_info.geo == sindex_geo_bool_t::GEO);
    PROFILE_STARTER_IF_ENABLED(
        ql_env->profile() == profile_bool_t::PROFILE,
        "Do intersection scan on geospatial index.",
        ql_env->trace);

    const reql_version_t sindex_func_reql_version =
        sindex_info.mapping_version_info.latest_compatible_reql_version;
    collect_all_geo_intersecting_cb_t callback(
        slice,
        geo_job_data_t(ql_env,
                       shard,
                       // The sorting is never `DESCENDING`, so this is always right.
                       sindex_range.left,
                       batchspec,
                       transforms,
                       terminal,
                       is_stamp_read),
        geo_sindex_data_t(pk_range, sindex_info.mapping,
                          sindex_func_reql_version, sindex_info.multi),
        query_geometry,
        response);
    continue_bool_t cont = btree_concurrent_traversal(
        superblock, sindex_range, &callback,
        direction_t::FORWARD,
        release_superblock_t::RELEASE);
    callback.finish(cont);
}

void rdb_get_nearest_slice(
    btree_slice_t *slice,
    const lon_lat_point_t &center,
    double max_dist,
    uint64_t max_results,
    const ellipsoid_spec_t &geo_system,
    sindex_superblock_t *superblock,
    ql::env_t *ql_env,
    const key_range_t &pk_range,
    const sindex_disk_info_t &sindex_info,
    nearest_geo_read_response_t *response) {

    guarantee(sindex_info.geo == sindex_geo_bool_t::GEO);
    PROFILE_STARTER_IF_ENABLED(
        ql_env->profile() == profile_bool_t::PROFILE,
        "Do nearest traversal on geospatial index.",
        ql_env->trace);

    const reql_version_t sindex_func_reql_version =
        sindex_info.mapping_version_info.latest_compatible_reql_version;

    // TODO (daniel): Instead of calling this multiple times until we are done,
    //   results should be streamed lazily. Also, even if we don't do that,
    //   the copying of the result we do here is bad.
    nearest_traversal_state_t state(center, max_results, max_dist, geo_system);
    response->results_or_error = nearest_geo_read_response_t::result_t();
    do {
        nearest_geo_read_response_t partial_response;
        try {
            nearest_traversal_cb_t callback(
                slice,
                geo_sindex_data_t(pk_range, sindex_info.mapping,
                                  sindex_func_reql_version, sindex_info.multi),
                ql_env,
                &state);
            btree_concurrent_traversal(
                superblock, key_range_t::universe(), &callback,
                direction_t::FORWARD,
                release_superblock_t::KEEP);
            callback.finish(&partial_response);
        } catch (const geo_exception_t &e) {
            partial_response.results_or_error =
                ql::exc_t(ql::base_exc_t::LOGIC, e.what(),
                          ql::backtrace_id_t::empty());
        }
        if (boost::get<ql::exc_t>(&partial_response.results_or_error)) {
            response->results_or_error = partial_response.results_or_error;
            return;
        } else {
            auto partial_res = boost::get<nearest_geo_read_response_t::result_t>(
                &partial_response.results_or_error);
            guarantee(partial_res != NULL);
            auto full_res = boost::get<nearest_geo_read_response_t::result_t>(
                &response->results_or_error);
            std::move(partial_res->begin(), partial_res->end(),
                      std::back_inserter(*full_res));
        }
    } while (state.proceed_to_next_batch() == continue_bool_t::CONTINUE);
}

void rdb_distribution_get(int max_depth,
                          const store_key_t &left_key,
                          real_superblock_t *superblock,
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

template <cluster_version_t W>
void serialize(write_message_t *wm, const rdb_modification_info_t &info) {
    if (!info.deleted.first.has()) {
        guarantee(info.deleted.second.empty());
        serialize<W>(wm, HAS_NO_VALUE);
    } else {
        serialize<W>(wm, HAS_VALUE);
        serialize<W>(wm, info.deleted);
    }

    if (!info.added.first.has()) {
        guarantee(info.added.second.empty());
        serialize<W>(wm, HAS_NO_VALUE);
    } else {
        serialize<W>(wm, HAS_VALUE);
        serialize<W>(wm, info.added);
    }
}

template <cluster_version_t W>
archive_result_t deserialize(read_stream_t *s, rdb_modification_info_t *info) {
    int8_t has_value;
    archive_result_t res = deserialize<W>(s, &has_value);
    if (bad(res)) { return res; }

    if (has_value == HAS_VALUE) {
        res = deserialize<W>(s, &info->deleted);
        if (bad(res)) { return res; }
    }

    res = deserialize<W>(s, &has_value);
    if (bad(res)) { return res; }

    if (has_value == HAS_VALUE) {
        res = deserialize<W>(s, &info->added);
        if (bad(res)) { return res; }
    }

    return archive_result_t::SUCCESS;
}

INSTANTIATE_SERIALIZABLE_SINCE_v1_13(rdb_modification_info_t);

RDB_IMPL_SERIALIZABLE_2_SINCE_v1_13(rdb_modification_report_t, primary_key, info);

rdb_modification_report_cb_t::rdb_modification_report_cb_t(
        store_t *store,
        buf_lock_t *sindex_block,
        auto_drainer_t::lock_t lock)
    : lock_(lock), store_(store),
      sindex_block_(sindex_block) {
    store_->acquire_all_sindex_superblocks_for_write(sindex_block_, &sindexes_);
}

rdb_modification_report_cb_t::~rdb_modification_report_cb_t() { }

bool rdb_modification_report_cb_t::has_pkey_cfeeds(
    const std::vector<store_key_t> &keys) {
    const store_key_t *min = nullptr, *max = nullptr;
    for (const auto &key : keys) {
        if (min == nullptr || key < *min) min = &key;
        if (max == nullptr || key > *max) max = &key;
    }
    if (min != nullptr && max != nullptr) {
        key_range_t range(key_range_t::closed, *min,
                          key_range_t::closed, *max);
        auto cservers = store_->access_changefeed_servers();
        for (auto &&pair : *cservers.first) {
            if (pair.first.inner.overlaps(range)
                && pair.second->has_limit(boost::optional<std::string>(),
                                          pair.second->get_keepalive())) {
                return true;
            }
        }
    }
    return false;
}

void rdb_modification_report_cb_t::finish(
    btree_slice_t *btree, real_superblock_t *superblock) {
    auto cservers = store_->access_changefeed_servers();
    for (auto &&pair : *cservers.first) {
        pair.second->foreach_limit(
            boost::optional<std::string>(),
            nullptr,
            [&](rwlock_in_line_t *clients_spot,
                rwlock_in_line_t *limit_clients_spot,
                rwlock_in_line_t *lm_spot,
                ql::changefeed::limit_manager_t *lm) {
                if (!lm->drainer.is_draining()) {
                    auto lock = lm->drainer.lock();
                    guarantee(clients_spot->read_signal()->is_pulsed());
                    guarantee(limit_clients_spot->read_signal()->is_pulsed());
                    lm->commit(lm_spot,
                               ql::changefeed::primary_ref_t{btree, superblock});
                }
            }, pair.second->get_keepalive());
    }
}

new_mutex_in_line_t rdb_modification_report_cb_t::get_in_line_for_sindex() {
    return store_->get_in_line_for_sindex_queue(sindex_block_);
}
rwlock_in_line_t rdb_modification_report_cb_t::get_in_line_for_cfeed_stamp() {
    return store_->get_in_line_for_cfeed_stamp(access_t::write);
}

void rdb_modification_report_cb_t::on_mod_report(
    const rdb_modification_report_t &report,
    bool update_pkey_cfeeds,
    new_mutex_in_line_t *sindex_spot,
    rwlock_in_line_t *cfeed_stamp_spot) {
    if (report.info.deleted.first.has() || report.info.added.first.has()) {
        // We spawn the sindex update in its own coroutine because we don't want to
        // hold the sindex update for the changefeed update or vice-versa.
        cond_t sindexes_updated_cond, keys_available_cond;
        index_vals_t old_cfeed_keys, new_cfeed_keys;
        sindex_spot->acq_signal()->wait_lazily_unordered();
        coro_t::spawn_now_dangerously(
            std::bind(&rdb_modification_report_cb_t::on_mod_report_sub,
                      this,
                      report,
                      sindex_spot,
                      &keys_available_cond,
                      &sindexes_updated_cond,
                      &old_cfeed_keys,
                      &new_cfeed_keys));
        auto cserver = store_->changefeed_server(report.primary_key);
        if (update_pkey_cfeeds && cserver.first != nullptr) {
            cserver.first->foreach_limit(
                boost::optional<std::string>(),
                &report.primary_key,
                [&](rwlock_in_line_t *clients_spot,
                    rwlock_in_line_t *limit_clients_spot,
                    rwlock_in_line_t *lm_spot,
                    ql::changefeed::limit_manager_t *lm) {
                    if (!lm->drainer.is_draining()) {
                        auto lock = lm->drainer.lock();
                        guarantee(clients_spot->read_signal()->is_pulsed());
                        guarantee(limit_clients_spot->read_signal()->is_pulsed());
                        if (report.info.deleted.first.has()) {
                            lm->del(lm_spot, report.primary_key, is_primary_t::YES);
                        }
                        if (report.info.added.first.has()) {
                            // The conflicting null values are resolved by
                            // the primary key.
                            lm->add(lm_spot, report.primary_key, is_primary_t::YES,
                                    ql::datum_t::null(), report.info.added.first);
                        }
                    }
                }, cserver.second);
        }
        keys_available_cond.wait_lazily_unordered();
        if (cserver.first != nullptr) {
            cserver.first->send_all(
                ql::changefeed::msg_t(
                    ql::changefeed::msg_t::change_t{
                        old_cfeed_keys,
                        new_cfeed_keys,
                        report.primary_key,
                        report.info.deleted.first,
                        report.info.added.first}),
                report.primary_key,
                cfeed_stamp_spot,
                cserver.second);
        }
        sindexes_updated_cond.wait_lazily_unordered();
    }
}

void rdb_modification_report_cb_t::on_mod_report_sub(
    const rdb_modification_report_t &mod_report,
    new_mutex_in_line_t *spot,
    cond_t *keys_available_cond,
    cond_t *done_cond,
    index_vals_t *cfeed_old_keys_out,
    index_vals_t *cfeed_new_keys_out) {
    store_->sindex_queue_push(mod_report, spot);
    rdb_live_deletion_context_t deletion_context;
    rdb_update_sindexes(store_,
                        sindexes_,
                        &mod_report,
                        sindex_block_->txn(),
                        &deletion_context,
                        keys_available_cond,
                        cfeed_old_keys_out,
                        cfeed_new_keys_out);
    guarantee(keys_available_cond->is_pulsed());
    done_cond->pulse();
}

std::vector<std::string> expand_geo_key(
        reql_version_t reql_version,
        const ql::datum_t &key,
        const store_key_t &primary_key,
        boost::optional<uint64_t> tag_num) {
    // Ignore non-geometry objects in geo indexes.
    // TODO (daniel): This needs to be changed once compound geo index
    // support gets added.
    if (!key.is_ptype(ql::pseudo::geometry_string)) {
        return std::vector<std::string>();
    }

    try {
        std::vector<std::string> grid_keys =
            compute_index_grid_keys(key, GEO_INDEX_GOAL_GRID_CELLS);

        std::vector<std::string> result;
        result.reserve(grid_keys.size());
        for (size_t i = 0; i < grid_keys.size(); ++i) {
            // TODO (daniel): Something else that needs change for compound index
            //   support: We must be able to truncate geo keys and handle such
            //   truncated keys.
            rassert(grid_keys[i].length() <= ql::datum_t::trunc_size(
                        key_to_unescaped_str(primary_key).length()));

            result.push_back(
                ql::datum_t::compose_secondary(
                    ql::skey_version_from_reql_version(reql_version),
                    grid_keys[i], primary_key, tag_num));
        }

        return result;
    } catch (const geo_exception_t &e) {
        // As things are now, this exception is actually ignored in
        // `compute_keys()`. That's ok, though it would be nice if we could
        // pass on some kind of warning to the user.
        logWRN("Failed to compute grid keys for an index: %s", e.what());
        rfail_target(&key, ql::base_exc_t::LOGIC,
                "Failed to compute grid keys: %s", e.what());
    }
}

void compute_keys(const store_key_t &primary_key,
                  ql::datum_t doc,
                  const sindex_disk_info_t &index_info,
                  std::vector<std::pair<store_key_t, ql::datum_t> > *keys_out,
                  std::vector<index_pair_t> *cfeed_keys_out) {

    guarantee(keys_out->empty());

    const reql_version_t reql_version =
        index_info.mapping_version_info.latest_compatible_reql_version;

    // Secondary index functions are deterministic (so no need for an rdb_context_t)
    // and evaluated in a pristine environment (without global optargs).
    cond_t non_interruptor;
    ql::env_t sindex_env(&non_interruptor,
                         ql::return_empty_normal_batches_t::NO,
                         reql_version);

    ql::datum_t index =
        index_info.mapping.compile_wire_func()->call(&sindex_env, doc)->as_datum();

    if (index_info.multi == sindex_multi_bool_t::MULTI
        && index.get_type() == ql::datum_t::R_ARRAY) {
        for (uint64_t i = 0; i < index.arr_size(); ++i) {
            const ql::datum_t &skey = index.get(i, ql::THROW);
            if (index_info.geo == sindex_geo_bool_t::GEO) {
                std::vector<std::string> geo_keys = expand_geo_key(reql_version,
                                                                   skey,
                                                                   primary_key,
                                                                   i);
                for (auto it = geo_keys.begin(); it != geo_keys.end(); ++it) {
                    keys_out->push_back(std::make_pair(store_key_t(*it), skey));
                }
                if (cfeed_keys_out != nullptr) {
                    // For geospatial indexes, we generate multiple keys for the same
                    // index entry. We only pass the smallest one on in order to not get
                    // redundant results on the changefeed.
                    auto min_it = std::min_element(geo_keys.begin(), geo_keys.end());
                    if (min_it != geo_keys.end()) {
                        cfeed_keys_out->push_back(
                            std::make_pair(skey, std::move(*min_it)));
                    }
                }
            } else {
                try {
                    std::string store_key =
                        skey.print_secondary(reql_version, primary_key, i);
                    keys_out->push_back(
                        std::make_pair(store_key_t(store_key), skey));
                    if (cfeed_keys_out != nullptr) {
                        cfeed_keys_out->push_back(
                            std::make_pair(skey, std::move(store_key)));
                    }
                } catch (const ql::base_exc_t &e) {
                    if (reql_version < reql_version_t::v2_1) {
                        throw;
                    }
                    // One of the values couldn't be converted to an index key.
                    // Ignore it and move on to the next one.
                }
            }
        }
    } else {
        if (index_info.geo == sindex_geo_bool_t::GEO) {
            std::vector<std::string> geo_keys = expand_geo_key(reql_version,
                                                               index,
                                                               primary_key,
                                                               boost::none);
            for (auto it = geo_keys.begin(); it != geo_keys.end(); ++it) {
                keys_out->push_back(std::make_pair(store_key_t(*it), index));
            }
            if (cfeed_keys_out != nullptr) {
                // For geospatial indexes, we generate multiple keys for the same
                // index entry. We only pass the smallest one on in order to not get
                // redundant results on the changefeed.
                auto min_it = std::min_element(geo_keys.begin(), geo_keys.end());
                if (min_it != geo_keys.end()) {
                    cfeed_keys_out->push_back(
                        std::make_pair(index, std::move(*min_it)));
                }
            }
        } else {
            std::string store_key =
                index.print_secondary(reql_version, primary_key, boost::none);
            keys_out->push_back(
                std::make_pair(store_key_t(store_key), index));
            if (cfeed_keys_out != nullptr) {
                cfeed_keys_out->push_back(
                    std::make_pair(index, std::move(store_key)));
            }
        }
    }
}

void serialize_sindex_info(write_message_t *wm,
                           const sindex_disk_info_t &info) {
    serialize_cluster_version(wm, cluster_version_t::LATEST_DISK);
    serialize<cluster_version_t::LATEST_DISK>(
        wm, info.mapping_version_info.original_reql_version);
    serialize<cluster_version_t::LATEST_DISK>(
        wm, info.mapping_version_info.latest_compatible_reql_version);
    serialize<cluster_version_t::LATEST_DISK>(
        wm, info.mapping_version_info.latest_checked_reql_version);

    serialize<cluster_version_t::LATEST_DISK>(wm, info.mapping);
    serialize<cluster_version_t::LATEST_DISK>(wm, info.multi);
    serialize<cluster_version_t::LATEST_DISK>(wm, info.geo);
}

void deserialize_sindex_info(
        const std::vector<char> &data,
        sindex_disk_info_t *info_out,
        const std::function<void(obsolete_reql_version_t)> &obsolete_cb) {
    buffer_read_stream_t read_stream(data.data(), data.size());
    // This cluster version field is _not_ a ReQL evaluation version field, which is
    // in secondary_index_t -- it only says how the value was serialized.
    cluster_version_t cluster_version;
    static_assert(obsolete_cluster_version_t::v1_13_2_is_latest
                  == obsolete_cluster_version_t::v1_13_2,
                  "1.13 is no longer the only obsolete cluster version.  "
                  "Instead of passing a constant obsolete_reql_version_t::v1_13 into "
                  "`obsolete_cb` below, there should be a separate `obsolete_cb` to "
                  "handle the different obsolete cluster versions.");
    archive_result_t success;

    try {
        success = deserialize_cluster_version(
            &read_stream,
            &cluster_version,
            std::bind(obsolete_cb, obsolete_reql_version_t::v1_13));

    } catch (const archive_exc_t &e) {
        rfail_toplevel(ql::base_exc_t::INTERNAL,
                       "Unrecognized secondary index version,"
                       " secondary index not created.");
    }
    throw_if_bad_deserialization(success, "sindex description");

    switch (cluster_version) {
    case cluster_version_t::v1_14:
    case cluster_version_t::v1_15:
    case cluster_version_t::v1_16:
    case cluster_version_t::v2_0:
    case cluster_version_t::v2_1:
    case cluster_version_t::v2_2:
    case cluster_version_t::v2_3_is_latest:
        success = deserialize_reql_version(
                &read_stream,
                &info_out->mapping_version_info.original_reql_version,
                obsolete_cb);
        throw_if_bad_deserialization(success, "original_reql_version");
        success = deserialize_for_version(
                cluster_version,
                &read_stream,
                &info_out->mapping_version_info.latest_compatible_reql_version);
        throw_if_bad_deserialization(success, "latest_compatible_reql_version");
        success = deserialize_for_version(
                cluster_version,
                &read_stream,
                &info_out->mapping_version_info.latest_checked_reql_version);
        throw_if_bad_deserialization(success, "latest_checked_reql_version");
        break;
    default:
        unreachable();
    }

    success = deserialize_for_version(cluster_version,
        &read_stream, &info_out->mapping);
    throw_if_bad_deserialization(success, "sindex description");

    success = deserialize_for_version(cluster_version, &read_stream, &info_out->multi);
    throw_if_bad_deserialization(success, "sindex description");
    switch (cluster_version) {
    case cluster_version_t::v1_14:
        info_out->geo = sindex_geo_bool_t::REGULAR;
        break;
    case cluster_version_t::v1_15: // fallthru
    case cluster_version_t::v1_16: // fallthru
    case cluster_version_t::v2_0: // fallthru
    case cluster_version_t::v2_1: // fallthru
    case cluster_version_t::v2_2: // fallthru
    case cluster_version_t::v2_3_is_latest:
        success = deserialize_for_version(cluster_version, &read_stream, &info_out->geo);
        throw_if_bad_deserialization(success, "sindex description");
        break;
    default: unreachable();
    }
    guarantee(static_cast<size_t>(read_stream.tell()) == data.size(),
              "An sindex description was incompletely deserialized.");
}

void deserialize_sindex_info_or_crash(
        const std::vector<char> &data,
        sindex_disk_info_t *info_out)
            THROWS_ONLY(archive_exc_t) {
    deserialize_sindex_info(data, info_out,
        [](obsolete_reql_version_t ver) -> void {
            switch (ver) {
            case obsolete_reql_version_t::v1_13:
                fail_due_to_user_error("Encountered a RethinkDB 1.13 secondary index, "
                                       "which is no longer supported.  You can use "
                                       "RethinkDB 2.0.5 to update your secondary index.");
                break;
            // v1_15 is equal to v1_14
            case obsolete_reql_version_t::v1_14:
                fail_due_to_user_error("Encountered an index from before RethinkDB "
                                       "1.16, which is no longer supported.  You can "
                                       "use RethinkDB 2.1 to update your secondary "
                                       "index.");
                break;
            default: unreachable();
            }
        });
}

/* Used below by rdb_update_sindexes. */
void rdb_update_single_sindex(
        store_t *store,
        const store_t::sindex_access_t *sindex,
        const deletion_context_t *deletion_context,
        const rdb_modification_report_t *modification,
        size_t *updates_left,
        auto_drainer_t::lock_t,
        cond_t *keys_available_cond,
        std::vector<index_pair_t> *cfeed_old_keys_out,
        std::vector<index_pair_t> *cfeed_new_keys_out)
    THROWS_NOTHING {
    // Note if you get this error it's likely that you've passed in a default
    // constructed mod_report. Don't do that.  Mod reports should always be passed
    // to a function as an output parameter before they're passed to this
    // function.
    guarantee(modification->primary_key.size() != 0);

    guarantee(cfeed_old_keys_out == nullptr || cfeed_old_keys_out->size() == 0);
    guarantee(cfeed_new_keys_out == nullptr || cfeed_new_keys_out->size() == 0);

    sindex_disk_info_t sindex_info;
    try {
        deserialize_sindex_info_or_crash(sindex->sindex.opaque_definition, &sindex_info);
    } catch (const archive_exc_t &e) {
        crash("%s", e.what());
    }
    // TODO(2015-01): Actually get real profiling information for
    // secondary index updates.
    profile::trace_t *const trace = nullptr;

    sindex_superblock_t *superblock = sindex->superblock.get();

    auto cserver = store->changefeed_server(modification->primary_key);

    if (modification->info.deleted.first.has()) {
        guarantee(!modification->info.deleted.second.empty());
        try {
            ql::datum_t deleted = modification->info.deleted.first;

            std::vector<std::pair<store_key_t, ql::datum_t> > keys;
            compute_keys(
                modification->primary_key, deleted, sindex_info,
                &keys, cfeed_old_keys_out);
            if (cserver.first != nullptr) {
                cserver.first->foreach_limit(
                    sindex->name.name,
                    &modification->primary_key,
                    [&](rwlock_in_line_t *clients_spot,
                        rwlock_in_line_t *limit_clients_spot,
                        rwlock_in_line_t *lm_spot,
                        ql::changefeed::limit_manager_t *lm) {
                        guarantee(clients_spot->read_signal()->is_pulsed());
                        guarantee(limit_clients_spot->read_signal()->is_pulsed());
                        for (const auto &pair : keys) {
                            lm->del(lm_spot, pair.first, is_primary_t::NO);
                        }
                    }, cserver.second);
            }
            for (auto it = keys.begin(); it != keys.end(); ++it) {
                promise_t<superblock_t *> return_superblock_local;
                {
                    keyvalue_location_t kv_location;
                    rdb_value_sizer_t sizer(superblock->cache()->max_block_size());

                    find_keyvalue_location_for_write(
                        &sizer,
                        superblock,
                        it->first.btree_key(),
                        repli_timestamp_t::distant_past,
                        deletion_context->balancing_detacher(),
                        &kv_location,
                        trace,
                        &return_superblock_local);

                    if (kv_location.value.has()) {
                        kv_location_delete(
                            &kv_location,
                            it->first,
                            repli_timestamp_t::distant_past,
                            deletion_context,
                            delete_mode_t::REGULAR_QUERY,
                            nullptr);
                    }
                    // The keyvalue location gets destroyed here.
                }
                superblock =
                    static_cast<sindex_superblock_t *>(return_superblock_local.wait());
            }
        } catch (const ql::base_exc_t &) {
            // Do nothing (it wasn't actually in the index).

            // See comment in `catch` below.
            guarantee(cfeed_old_keys_out == nullptr || cfeed_old_keys_out->size() == 0);
        }
    }

    // If the secondary index is being deleted, we don't add any new values to
    // the sindex tree.
    // This is so we don't race against any sindex erase about who is faster
    // (we with inserting new entries, or the erase with removing them).
    const bool sindex_is_being_deleted = sindex->sindex.being_deleted;
    if (!sindex_is_being_deleted && modification->info.added.first.has()) {
        bool decremented_updates_left = false;
        try {
            ql::datum_t added = modification->info.added.first;

            std::vector<std::pair<store_key_t, ql::datum_t> > keys;

            compute_keys(
                modification->primary_key, added, sindex_info,
                &keys, cfeed_new_keys_out);
            if (keys_available_cond != nullptr) {
                guarantee(*updates_left > 0);
                decremented_updates_left = true;
                if (--*updates_left == 0) {
                    keys_available_cond->pulse();
                }
            }
            if (cserver.first != nullptr) {
                cserver.first->foreach_limit(
                    sindex->name.name,
                    &modification->primary_key,
                    [&](rwlock_in_line_t *clients_spot,
                        rwlock_in_line_t *limit_clients_spot,
                        rwlock_in_line_t *lm_spot,
                        ql::changefeed::limit_manager_t *lm) {
                        guarantee(clients_spot->read_signal()->is_pulsed());
                        guarantee(limit_clients_spot->read_signal()->is_pulsed());
                        for (const auto &pair :keys) {
                            lm->add(lm_spot, pair.first, is_primary_t::NO,
                                    pair.second, added);
                        }
                    }, cserver.second);
            }
            for (auto it = keys.begin(); it != keys.end(); ++it) {
                promise_t<superblock_t *> return_superblock_local;
                {
                    keyvalue_location_t kv_location;

                    rdb_value_sizer_t sizer(superblock->cache()->max_block_size());
                    find_keyvalue_location_for_write(
                        &sizer,
                        superblock,
                        it->first.btree_key(),
                        repli_timestamp_t::distant_past,
                        deletion_context->balancing_detacher(),
                        &kv_location,
                        trace,
                        &return_superblock_local);

                    ql::serialization_result_t res =
                        kv_location_set(&kv_location, it->first,
                                        modification->info.added.second,
                                        repli_timestamp_t::distant_past,
                                        deletion_context);
                    // this particular context cannot fail AT THE MOMENT.
                    guarantee(!bad(res));
                    // The keyvalue location gets destroyed here.
                }
                superblock = static_cast<sindex_superblock_t *>(
                    return_superblock_local.wait());
            }
        } catch (const ql::base_exc_t &) {
            // Do nothing (we just drop the row from the index).

            // If we've decremented `updates_left` already, that means we might
            // have sent a change with new values for this key even though we're
            // actually dropping the row.  I *believe* that this catch statement
            // can only be triggered by an exception thrown from `compute_keys`
            // (which begs the question of why so many other statements are
            // inside of it), so this guarantee should never trip.
            if (keys_available_cond != nullptr) {
                guarantee(!decremented_updates_left);
                guarantee(cfeed_new_keys_out->size() == 0);
                guarantee(*updates_left > 0);
                if (--*updates_left == 0) {
                    keys_available_cond->pulse();
                }
            }
        }
    } else {
        if (keys_available_cond != nullptr) {
            guarantee(*updates_left > 0);
            if (--*updates_left == 0) {
                keys_available_cond->pulse();
            }
        }
    }

    if (cserver.first != nullptr) {
        cserver.first->foreach_limit(
            sindex->name.name,
            &modification->primary_key,
            [&](rwlock_in_line_t *clients_spot,
                rwlock_in_line_t *limit_clients_spot,
                rwlock_in_line_t *lm_spot,
                ql::changefeed::limit_manager_t *lm) {
                guarantee(clients_spot->read_signal()->is_pulsed());
                guarantee(limit_clients_spot->read_signal()->is_pulsed());
                lm->commit(lm_spot, ql::changefeed::sindex_ref_t{
                        sindex->btree, superblock, &sindex_info});
            }, cserver.second);
    }
}

void rdb_update_sindexes(
    store_t *store,
    const store_t::sindex_access_vector_t &sindexes,
    const rdb_modification_report_t *modification,
    txn_t *txn,
    const deletion_context_t *deletion_context,
    cond_t *keys_available_cond,
    index_vals_t *cfeed_old_keys_out,
    index_vals_t *cfeed_new_keys_out) {

    rdb_noop_deletion_context_t noop_deletion_context;
    {
        auto_drainer_t drainer;

        // This assert is here to make sure that we pulse keys_available_cond even if
        // some indexes aren't done constructing yet.
        ASSERT_NO_CORO_WAITING;

        size_t counter = 0;
        for (const auto &sindex : sindexes) {
            // Update only indexes that have been post-constructed for the relevant
            // range.
            if (!sindex->sindex.needs_post_construction_range.contains_key(
                    modification->primary_key)) {
                ++counter;
                // If the index isn't done constructing yet, we must use a noop deletion
                // context. The reason is that such an index might have pointers to
                // blocks that have already been deleted, and trying to detach them
                // through a regular deleter would be illegal.
                // Also (luckily) we don't need to detach the values, because there's
                // no way that any snapshotted read transaction can be active in a
                // non-post-constructed index.
                const deletion_context_t *actual_deletion_context =
                    sindex->sindex.post_construction_complete()
                    ? deletion_context
                    : &noop_deletion_context;
                coro_t::spawn_sometime(
                    std::bind(
                        &rdb_update_single_sindex,
                        store,
                        sindex.get(),
                        actual_deletion_context,
                        modification,
                        &counter,
                        auto_drainer_t::lock_t(&drainer),
                        keys_available_cond,
                        cfeed_old_keys_out == NULL
                            ? NULL
                            : &(*cfeed_old_keys_out)[sindex->name.name],
                        cfeed_new_keys_out == NULL
                            ? NULL
                            : &(*cfeed_new_keys_out)[sindex->name.name]));
            }
        }
        if (counter == 0 && keys_available_cond != NULL) {
            keys_available_cond->pulse();
        }
    }

    /* All of the sindex have been updated now it's time to actually clear the
     * deleted blob if it exists. */
    if (modification->info.deleted.first.has()) {
        deletion_context->post_deleter()->delete_value(buf_parent_t(txn),
                modification->info.deleted.second.data());
    }
}

class post_construct_traversal_helper_t : public concurrent_traversal_callback_t {
public:
    post_construct_traversal_helper_t(
            store_t *store,
            const std::set<uuid_u> &sindexes_to_post_construct,
            cond_t *on_indexes_deleted,
            const std::function<bool(int64_t)> &check_should_abort,
            signal_t *interruptor)
        : store_(store),
          sindexes_to_post_construct_(sindexes_to_post_construct),
          on_indexes_deleted_(on_indexes_deleted),
          interruptor_(interruptor),
          check_should_abort_(check_should_abort),
          pairs_constructed_(0),
          stopped_before_completion_(false),
          current_chunk_size_(0) {
        // Start an initial write transaction for the first chunk.
        // (this acquisition should never block)
        new_mutex_acq_t wtxn_acq(&wtxn_lock_);
        start_write_transaction(&wtxn_acq);
    }

    continue_bool_t handle_pair(
            scoped_key_value_t &&keyvalue,
            concurrent_traversal_fifo_enforcer_signal_t waiter)
            THROWS_ONLY(interrupted_exc_t) {

        if (interruptor_->is_pulsed() || on_indexes_deleted_->is_pulsed()) {
            throw interrupted_exc_t();
        }

        // Account for the read value in the stats
        store_->btree->stats.pm_keys_read.record();
        store_->btree->stats.pm_total_keys_read += 1;

        // Grab the key and value and construct a modification report for the key/value
        // pair.
        const store_key_t primary_key(keyvalue.key());
        const rdb_value_t *rdb_value =
            static_cast<const rdb_value_t *>(keyvalue.value());
        rdb_modification_report_t mod_report(primary_key);
        const max_block_size_t block_size =
            keyvalue.expose_buf().cache()->max_block_size();
        mod_report.info.added
            = std::make_pair(
                get_data(rdb_value, buf_parent_t(keyvalue.expose_buf())),
                std::vector<char>(rdb_value->value_ref(),
                    rdb_value->value_ref() + rdb_value->inline_size(block_size)));

        // Store the value into the secondary indexes
        {
            // We need this mutex because we don't want `wtxn` to be destructed,
            // but also because only one coroutine can be traversing the indexes
            // through `rdb_update_sindexes` at a time (or else the btree will get
            // corrupted!).
            new_mutex_acq_t wtxn_acq(&wtxn_lock_, interruptor_);
            guarantee(wtxn_.has());
            const rdb_post_construction_deletion_context_t deletion_context;
            rdb_update_sindexes(store_,
                                sindexes_,
                                &mod_report,
                                wtxn_.get(),
                                &deletion_context,
                                NULL,
                                NULL,
                                NULL);
        }

        // Account for the sindex writes in the stats
        store_->btree->stats.pm_keys_set.record(sindexes_.size());
        store_->btree->stats.pm_total_keys_set += sindexes_.size();

        // Update the traversed range boundary (everything below here will happen in
        // key order).
        // This can't be interrupted, because we have already called rdb_update_sindexes,
        // so now we /must/ update traversed_right_bound.
        waiter.wait();
        traversed_right_bound_ = primary_key;

        // Release the write transaction and secondary index locks once we've reached the
        // designated chunk size. Then acquire a new transaction once the previous one
        // has been flushed.
        {
            new_mutex_acq_t wtxn_acq(&wtxn_lock_, interruptor_);
            ++current_chunk_size_;
            if (current_chunk_size_ >= MAX_CHUNK_SIZE) {
                current_chunk_size_ = 0;
                sindexes_.clear();
                wtxn_.reset();
                start_write_transaction(&wtxn_acq);
            }
        }

        ++pairs_constructed_;
        if (check_should_abort_(pairs_constructed_)) {
            stopped_before_completion_ = true;
            return continue_bool_t::ABORT;
        } else {
            return continue_bool_t::CONTINUE;
        }
    }

    store_key_t get_traversed_right_bound() const {
        return traversed_right_bound_;
    }

    bool stopped_before_completion() const {
        return stopped_before_completion_;
    }

private:
    // Number of key/value pairs we process before releasing the write transaction
    // and waiting for the secondary index data to be flushed to disk.
    // Also see the comment above `scoped_ptr_t<txn_t> wtxn;` below.
    static const int MAX_CHUNK_SIZE = 32;

    void start_write_transaction(new_mutex_acq_t *wtxn_acq) {
        wtxn_acq->guarantee_is_holding(&wtxn_lock_);
        guarantee(!wtxn_.has());

        // Start a write transaction and acquire the secondary indexes
        write_token_t token;
        store_->new_write_token(&token);

        // We use HARD durability because we want post construction
        // to be throttled if we insert data faster than it can
        // be written to disk. Otherwise we might exhaust the cache's
        // dirty page limit and bring down the whole table.
        // Other than that, the hard durability guarantee is not actually
        // needed here.
        scoped_ptr_t<real_superblock_t> superblock;
        store_->acquire_superblock_for_write(
                2 + MAX_CHUNK_SIZE,
                write_durability_t::HARD,
                &token,
                &wtxn_,
                &superblock,
                interruptor_);

        // Acquire the sindex block and release the superblock.
        const block_id_t sindex_block_id = superblock->get_sindex_block_id();
        buf_lock_t sindex_block(superblock->expose_buf(), sindex_block_id,
                                access_t::write);
        superblock.reset();
        store_t::sindex_access_vector_t all_sindexes;
        store_->acquire_sindex_superblocks_for_write(
            sindexes_to_post_construct_,
            &sindex_block,
            &all_sindexes);

        // Filter out indexes that are being deleted. No need to keep post-constructing
        // those.
        guarantee(sindexes_.empty());
        for (auto &&access : all_sindexes) {
            if (!access->sindex.being_deleted) {
                sindexes_.emplace_back(std::move(access));
            }
        }
        if (sindexes_.empty()) {
            // All indexes have been deleted. Interrupt the traversal.
            on_indexes_deleted_->pulse_if_not_already_pulsed();
        }

        // We pretend that the indexes have been fully constructed, so that when we call
        // `rdb_update_sindexes` above, it actually updates the range we're currently
        // constructing. This is a bit hacky, but works.
        for (auto &&access : sindexes_) {
            access->sindex.needs_post_construction_range = key_range_t::empty();
        }
    }

    store_t *store_;
    const std::set<uuid_u> sindexes_to_post_construct_;
    cond_t *on_indexes_deleted_;
    signal_t *interruptor_;

    std::function<bool(int64_t)> check_should_abort_;

    // How far we've come in the traversal
    int64_t pairs_constructed_;
    store_key_t traversed_right_bound_;
    bool stopped_before_completion_;

    // We re-use a single write transaction and secondary index acquisition for a chunk
    // of writes to get better efficiency when flushing the index writes to disk.
    // We reset the transaction  after each chunk because large write transactions can
    // cause the cache to go into throttling, and that would interfere with other
    // transactions on this table.
    // Another aspect to keep in mind is that if we hold the write lock on the sindexes
    // for too long, other concurrent writes to parts of the secondary index that
    // are already live will also be delayed.
    scoped_ptr_t<txn_t> wtxn_;
    store_t::sindex_access_vector_t sindexes_;
    int current_chunk_size_;
    // Controls access to `sindexes_` and `wtxn_`.
    new_mutex_t wtxn_lock_;
};

void post_construct_secondary_index_range(
        store_t *store,
        const std::set<uuid_u> &sindex_ids_to_post_construct,
        key_range_t *construction_range_inout,
        const std::function<bool(int64_t)> &check_should_abort,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) {

    // In case the index gets deleted in the middle of the construction, this gets
    // triggered.
    cond_t on_index_deleted_interruptor;

    // Mind the destructor ordering.
    // The superblock must be released before txn (`btree_concurrent_traversal`
    // usually already takes care of that).
    // The txn must be destructed before the cache_account.
    cache_account_t cache_account;
    scoped_ptr_t<txn_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;

    // Start a snapshotted read transaction to traverse the primary btree
    read_token_t read_token;
    store->new_read_token(&read_token);
    store->acquire_superblock_for_read(
        &read_token,
        &txn,
        &superblock,
        interruptor,
        true /* USE_SNAPSHOT */);

    // Note: This starts a write transaction, which might get throttled.
    // It is important that we construct the `traversal_cb` *after* we've started
    // the snapshotted read transaction, or otherwise we might deadlock in the presence
    // of additional (unrelated) write transactions..
    post_construct_traversal_helper_t traversal_cb(
        store,
        sindex_ids_to_post_construct,
        &on_index_deleted_interruptor,
        check_should_abort,
        interruptor);

    cache_account
        = txn->cache()->create_cache_account(SINDEX_POST_CONSTRUCTION_CACHE_PRIORITY);
    txn->set_account(&cache_account);

    continue_bool_t cont = btree_concurrent_traversal(
        superblock.get(),
        *construction_range_inout,
        &traversal_cb,
        direction_t::FORWARD,
        release_superblock_t::RELEASE);
    if (cont == continue_bool_t::ABORT
        && (interruptor->is_pulsed() || on_index_deleted_interruptor.is_pulsed())) {
        throw interrupted_exc_t();
    }

    // Update the left bound of the construction range
    if (!traversal_cb.stopped_before_completion()) {
        // The construction is done. Set the remaining range to empty.
        *construction_range_inout = key_range_t::empty();
    } else {
        // (the key_range_t construction below needs to be updated if the right bound can
        //  be bounded. For now we assume it's unbounded.)
        guarantee(construction_range_inout->right.unbounded);
        *construction_range_inout = key_range_t(
            key_range_t::bound_t::open, traversal_cb.get_traversed_right_bound(),
            key_range_t::bound_t::none, store_key_t());
    }
}

void noop_value_deleter_t::delete_value(buf_parent_t, const void *) const { }
