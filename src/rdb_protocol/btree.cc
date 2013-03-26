// Copyright 2010-2012 RethinkDB, all rights reserved.
#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>

#include "btree/backfill.hpp"
#include "btree/depth_first_traversal.hpp"
#include "btree/erase_range.hpp"
#include "btree/get_distribution.hpp"
#include "btree/operations.hpp"
#include "buffer_cache/blob.hpp"
#include "containers/archive/buffer_group_stream.hpp"
#include "containers/archive/vector_stream.hpp"
#include "containers/scoped.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/transform_visitors.hpp"

typedef std::list<boost::shared_ptr<scoped_cJSON_t> > json_list_t;
typedef std::list<std::pair<store_key_t, boost::shared_ptr<scoped_cJSON_t> > > keyed_json_list_t;

#define MAX_RDB_VALUE_SIZE MAX_IN_NODE_VALUE_SIZE

struct rdb_value_t {
    char contents[];

public:
    int inline_size(block_size_t bs) const {
        return blob::ref_size(bs, contents, blob::btree_maxreflen);
    }

    int64_t value_size() const {
        return blob::value_size(contents, blob::btree_maxreflen);
    }

    const char *value_ref() const {
        return contents;
    }

    char *value_ref() {
        return contents;
    }
};

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

bool value_sizer_t<rdb_value_t>::deep_fsck(block_getter_t *getter, const void *value, int length_available, std::string *msg_out) const {
    if (!fits(value, length_available)) {
        *msg_out = "value does not fit in length_available";
        return false;
    }

    return blob::deep_fsck(getter, block_size_, as_rdb(value)->value_ref(), blob::btree_maxreflen, msg_out);
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

boost::shared_ptr<scoped_cJSON_t> get_data(const rdb_value_t *value,
                                           transaction_t *txn) {
    blob_t blob(const_cast<rdb_value_t *>(value)->value_ref(), blob::btree_maxreflen);

    boost::shared_ptr<scoped_cJSON_t> data;

    blob_acq_t acq_group;
    buffer_group_t buffer_group;
    blob.expose_all(txn, rwi_read, &buffer_group, &acq_group);
    buffer_group_read_stream_t read_stream(const_view(&buffer_group));
    int res = deserialize(&read_stream, &data);
    guarantee_err(res == 0, "corruption detected... this should probably be an exception\n");

    return data;
}

bool btree_value_fits(block_size_t bs, int data_length, const rdb_value_t *value) {
    return blob::ref_fits(bs, data_length, value->value_ref(), blob::btree_maxreflen);
}

void rdb_get(const store_key_t &store_key, btree_slice_t *slice, transaction_t *txn, superblock_t *superblock, point_read_response_t *response) {
    keyvalue_location_t<rdb_value_t> kv_location;
    find_keyvalue_location_for_read(txn, superblock, store_key.btree_key(), &kv_location, slice->root_eviction_priority, &slice->stats);

    if (!kv_location.value.has()) {
        response->data.reset(new scoped_cJSON_t(cJSON_CreateNull()));
    } else {
        response->data = get_data(kv_location.value.get(), txn);
    }
}

void kv_location_delete(keyvalue_location_t<rdb_value_t> *kv_location, const store_key_t &key,
                        btree_slice_t *slice, repli_timestamp_t timestamp, transaction_t *txn) {
    guarantee(kv_location->value.has());
    blob_t blob(kv_location->value->value_ref(), blob::btree_maxreflen);
    blob.clear(txn);
    kv_location->value.reset();
    null_key_modification_callback_t<rdb_value_t> null_cb;
    apply_keyvalue_change(txn, kv_location, key.btree_key(), timestamp, false, &null_cb, &slice->root_eviction_priority);
}

void kv_location_set(keyvalue_location_t<rdb_value_t> *kv_location, const store_key_t &key,
                     boost::shared_ptr<scoped_cJSON_t> data,
                     btree_slice_t *slice, repli_timestamp_t timestamp, transaction_t *txn) {

    scoped_malloc_t<rdb_value_t> new_value(MAX_RDB_VALUE_SIZE);
    bzero(new_value.get(), MAX_RDB_VALUE_SIZE);

    //TODO unnecessary copies they must go away.
    write_message_t wm;
    wm << data;
    vector_stream_t stream;
    int res = send_write_message(&stream, &wm);
    guarantee_err(res == 0, "Serialization for json data failed... this shouldn't happen.\n");

    blob_t blob(new_value->value_ref(), blob::btree_maxreflen);

    //TODO more copies, good lord
    blob.append_region(txn, stream.vector().size());
    std::string sered_data(stream.vector().begin(), stream.vector().end());
    blob.write_from_string(sered_data, txn, 0);

    // Actually update the leaf, if needed.
    kv_location->value.reinterpret_swap(new_value);
    null_key_modification_callback_t<rdb_value_t> null_cb;
    apply_keyvalue_change(txn, kv_location, key.btree_key(), timestamp, false, &null_cb, &slice->root_eviction_priority);
    //                                                                  ^^^^^ That means the key isn't expired.
}

// QL2 This implements UPDATE, REPLACE, and part of DELETE and INSERT (each is
// just a different function passed to this function).
void rdb_replace(btree_slice_t *slice,
                 repli_timestamp_t timestamp,
                 transaction_t *txn,
                 superblock_t *superblock,
                 const std::string &primary_key,
                 const store_key_t &key,
                 ql::map_wire_func_t *f,
                 ql::env_t *ql_env,
                 Datum *response_out) {
    const ql::datum_t *num_1 = ql_env->add_ptr(new ql::datum_t(1.0));
    ql::datum_t *resp = ql_env->add_ptr(new ql::datum_t(ql::datum_t::R_OBJECT));
    try {
        keyvalue_location_t<rdb_value_t> kv_location;
        find_keyvalue_location_for_write(
            txn, superblock, key.btree_key(), &kv_location,
            &slice->root_eviction_priority, &slice->stats);

        bool started_empty, ended_empty;
        const ql::datum_t *old_val;
        if (!kv_location.value.has()) {
            // If there's no entry with this key, pass NULL to the function.
            started_empty = true;
            old_val = ql_env->add_ptr(new ql::datum_t(ql::datum_t::R_NULL));
        } else {
            // Otherwise pass the entry with this key to the function.
            started_empty = false;
            boost::shared_ptr<scoped_cJSON_t> old_val_json =
                get_data(kv_location.value.get(), txn);
            guarantee(old_val_json->GetObjectItem(primary_key.c_str()));
            old_val = ql_env->add_ptr(new ql::datum_t(old_val_json, ql_env));
        }
        guarantee(old_val);

        const ql::datum_t *new_val = f->compile(ql_env)->call(old_val)->as_datum();
        if (new_val->get_type() == ql::datum_t::R_NULL) {
            ended_empty = true;
        } else if (new_val->get_type() == ql::datum_t::R_OBJECT) {
            ended_empty = false;
            rcheck_target(
                new_val, new_val->get(primary_key, ql::NOTHROW),
                strprintf("Inserted object must have primary key `%s`:\n%s",
                          primary_key.c_str(), new_val->print().c_str()));
        } else {
            rfail_target(new_val, "Inserted value must be an OBJECT (got %s):\n%s",
                         new_val->get_type_name(), new_val->print().c_str());
        }

        // We use `conflict` below to store whether or not there was a key
        // conflict when constructing the stats object.  It defaults to `true`
        // so that we fail an assertion if we never update the stats object.
        bool conflict = true;
        // Figure out what operation we're doing (based on started_empty,
        // ended_empty, and the result of the function call) and then do it.
        if (started_empty) {
            if (ended_empty) {
                conflict = resp->add("skipped", num_1);
            } else {
                conflict = resp->add("inserted", num_1);
                r_sanity_check(new_val->get(primary_key, ql::NOTHROW));
                kv_location_set(&kv_location, key, new_val->as_json(),
                                slice, timestamp, txn);
            }
        } else {
            if (ended_empty) {
                conflict = resp->add("deleted", num_1);
                kv_location_delete(&kv_location, key, slice, timestamp, txn);
            } else {
                if (*old_val->get(primary_key) == *new_val->get(primary_key)) {
                    if (*old_val == *new_val) {
                        conflict = resp->add("unchanged", num_1);
                    } else {
                        conflict = resp->add("replaced", num_1);
                        r_sanity_check(new_val->get(primary_key, ql::NOTHROW));
                        kv_location_set(&kv_location, key, new_val->as_json(),
                                        slice, timestamp, txn);
                    }
                } else {
                    rfail_target(
                        new_val,
                        "Primary key `%s` cannot be changed (%s -> %s)",
                        primary_key.c_str(),
                        old_val->print().c_str(), new_val->print().c_str());
                }
            }
        }
        guarantee(!conflict); // message never added twice
    } catch (const ql::base_exc_t &e) {
        std::string msg = e.what();
        bool b = resp->add("errors", num_1)
              || resp->add("first_error", ql_env->add_ptr(new ql::datum_t(msg)));
        guarantee(!b);
    }
    resp->write_to_protobuf(response_out);
}

void rdb_set(const store_key_t &key, boost::shared_ptr<scoped_cJSON_t> data, bool overwrite,
             btree_slice_t *slice, repli_timestamp_t timestamp,
             transaction_t *txn, superblock_t *superblock, point_write_response_t *response) {
    //block_size_t block_size = slice->cache()->get_block_size();
    keyvalue_location_t<rdb_value_t> kv_location;
    find_keyvalue_location_for_write(txn, superblock, key.btree_key(), &kv_location, &slice->root_eviction_priority, &slice->stats);
    bool had_value = kv_location.value.has();
    if (overwrite || !had_value) {
        kv_location_set(&kv_location, key, data, slice, timestamp, txn);
    }
    response->result = (had_value ? DUPLICATE : STORED);
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

    rdb_backfill_callback_t *cb_;
    key_range_t kr_;
};

void rdb_backfill(btree_slice_t *slice, const key_range_t& key_range,
        repli_timestamp_t since_when, rdb_backfill_callback_t *callback,
        transaction_t *txn, superblock_t *superblock,
        parallel_traversal_progress_t *p, signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    agnostic_rdb_backfill_callback_t agnostic_cb(callback, key_range);
    value_sizer_t<rdb_value_t> sizer(slice->cache()->get_block_size());
    do_agnostic_btree_backfill(&sizer, slice, key_range, since_when, &agnostic_cb, txn, superblock, p, interruptor);
}

void rdb_delete(const store_key_t &key, btree_slice_t *slice,
                repli_timestamp_t timestamp, transaction_t *txn,
                superblock_t *superblock, point_delete_response_t *response) {
    keyvalue_location_t<rdb_value_t> kv_location;
    find_keyvalue_location_for_write(txn, superblock, key.btree_key(), &kv_location, &slice->root_eviction_priority, &slice->stats);
    bool exists = kv_location.value.has();
    if (exists) kv_location_delete(&kv_location, key, slice, timestamp, txn);
    response->result = (exists ? DELETED : MISSING);
}

void rdb_erase_range(btree_slice_t *slice, key_tester_t *tester,
                     bool left_key_supplied, const store_key_t& left_key_exclusive,
                     bool right_key_supplied, const store_key_t& right_key_inclusive,
                     transaction_t *txn, superblock_t *superblock) {

    value_sizer_t<rdb_value_t> rdb_sizer(slice->cache()->get_block_size());
    value_sizer_t<void> *sizer = &rdb_sizer;

    struct : public value_deleter_t {
        void delete_value(transaction_t *_txn, void *_value) {
            blob_t blob(static_cast<rdb_value_t *>(_value)->value_ref(), blob::btree_maxreflen);
            blob.clear(_txn);
        }
    } deleter;

    btree_erase_range_generic(sizer, slice, tester, &deleter,
        left_key_supplied ? left_key_exclusive.btree_key() : NULL,
        right_key_supplied ? right_key_inclusive.btree_key() : NULL,
        txn, superblock);
}

void rdb_erase_range(btree_slice_t *slice, key_tester_t *tester,
                       const key_range_t &keys,
                       transaction_t *txn, superblock_t *superblock) {
    store_key_t left_exclusive(keys.left);
    store_key_t right_inclusive(keys.right.key);

    bool left_key_supplied = left_exclusive.decrement();
    bool right_key_supplied = !keys.right.unbounded;
    if (right_key_supplied) {
        right_inclusive.decrement();
    }
    rdb_erase_range(slice, tester, left_key_supplied, left_exclusive, right_key_supplied, right_inclusive, txn, superblock);
}

size_t estimate_rget_response_size(const boost::shared_ptr<scoped_cJSON_t> &/*json*/) {
    // TODO: don't be stupid, be a smarty, come and join the nazy
    // party (json size estimation will be much easier once we switch
    // to bson -- fuck it for now).
    return 250;
}


class result_gc_visitor_t : public boost::static_visitor<void> {
public:
    result_gc_visitor_t(ql::env_t *e, ql::env_gc_checkpoint_t *egct) :
        env(e), env_gc_checkpoint(egct), total_rounds(0) { }

    void operator()(const rget_read_response_t::stream_t &) const { }
    void operator()(const rget_read_response_t::groups_t &) const { }
    void operator()(const rget_read_response_t::atom_t &) const { }
    void operator()(const rget_read_response_t::length_t &) const { }
    void operator()(const rget_read_response_t::inserted_t &) const { }
    void operator()(const query_language::runtime_exc_t &) const { }
    void operator()(const ql::exc_t &) const { }
    void operator()(const std::vector<ql::wire_datum_t> &) const { }
    void operator()(const std::vector<ql::wire_datum_map_t> &) const { }
    void operator()(const rget_read_response_t::empty_t &) const { }
    void operator()(const rget_read_response_t::vec_t &) const { }

    void operator()(ql::wire_datum_t &d) {
        env_gc_checkpoint->maybe_gc(d.get());
    }
    void operator()(ql::wire_datum_map_t &dm) {
        total_rounds += 1;
        if (total_rounds % ql::WIRE_DATUM_MAP_GC_ROUNDS == 0) {
            env_gc_checkpoint->maybe_gc(dm.to_arr(env));
        }
    }

private:
    ql::env_t *env;
    ql::env_gc_checkpoint_t *env_gc_checkpoint;
    int64_t total_rounds;
};

class rdb_rget_depth_first_traversal_callback_t : public depth_first_traversal_callback_t {
public:
    rdb_rget_depth_first_traversal_callback_t(
        transaction_t *txn,
        ql::env_t *_ql_env,
        const rdb_protocol_details::transform_t &_transform,
        boost::optional<rdb_protocol_details::terminal_t> _terminal,
        const key_range_t &range,
        rget_read_response_t *_response)
        : bad_init(false), transaction(txn), response(_response), cumulative_size(0),
          ql_env(_ql_env), transform(_transform), terminal(_terminal)
    {
        try {
            response->last_considered_key = range.left;

            if (terminal) {
                boost::apply_visitor(query_language::terminal_initializer_visitor_t(
                                         &response->result, ql_env,
                                         terminal->scopes, terminal->backtrace),
                                     terminal->variant);
            }
        } catch (const query_language::runtime_exc_t &e) {
            /* Evaluation threw so we're not going to be accepting any more requests. */
            response->result = e;
            bad_init = true;
        } catch (const ql::exc_t &e2) {
            /* Evaluation threw so we're not going to be accepting any more requests. */
            response->result = e2;
            bad_init = true;
        }
    }

    bool handle_pair(const btree_key_t* key, const void *value) {
        if (bad_init) return false;
        try {
            store_key_t store_key(key);
            if (response->last_considered_key < store_key) {
                response->last_considered_key = store_key;
            }

            const rdb_value_t *rdb_value = reinterpret_cast<const rdb_value_t *>(value);

            json_list_t data;
            data.push_back(get_data(rdb_value, transaction));

            //Apply transforms to the data
            {
                rdb_protocol_details::transform_t::iterator it;
                for (it = transform.begin(); it != transform.end(); ++it) {
                    json_list_t tmp;

                    for (json_list_t::iterator jt  = data.begin();
                         jt != data.end();
                         ++jt) {
                        boost::apply_visitor(query_language::transform_visitor_t(
                                                 *jt, &tmp, ql_env, it->scopes,
                                                 it->backtrace), it->variant);
                    }
                    data.clear();
                    data.splice(data.begin(), tmp);
                }
            }

            if (!terminal) {
                typedef rget_read_response_t::stream_t stream_t;
                stream_t *stream = boost::get<stream_t>(&response->result);
                guarantee(stream);
                for (json_list_t::iterator it =  data.begin();
                                           it != data.end();
                                           ++it) {
                    stream->push_back(std::make_pair(store_key_t(key), *it));
                    cumulative_size += estimate_rget_response_size(*it);
                }

                return cumulative_size < rget_max_chunk_size;
            } else {
                // We use garbage collection during the reduction step, since
                // most reductions throw away most of the allocate data.
                ql::env_gc_checkpoint_t gc_checkpoint(ql_env);
                result_gc_visitor_t result_gc_visitor(ql_env, &gc_checkpoint);
                json_list_t::iterator jt;
                for (jt = data.begin(); jt != data.end(); ++jt) {
                    boost::apply_visitor(query_language::terminal_visitor_t(
                                             *jt, ql_env, terminal->scopes,
                                             terminal->backtrace, &response->result),
                                         terminal->variant);
                    boost::apply_visitor(result_gc_visitor, response->result);
                }
                return true;
            }
        } catch (const query_language::runtime_exc_t &e) {
            /* Evaluation threw so we're not going to be accepting any more requests. */
            response->result = e;
            return false;
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
};

class result_finalizer_visitor_t : public boost::static_visitor<void> {
public:
    void operator()(const rget_read_response_t::stream_t &) const { }
    void operator()(const rget_read_response_t::groups_t &) const { }
    void operator()(const rget_read_response_t::atom_t &) const { }
    void operator()(const rget_read_response_t::length_t &) const { }
    void operator()(const rget_read_response_t::inserted_t &) const { }
    void operator()(const query_language::runtime_exc_t &) const { }
    void operator()(const ql::exc_t &) const { }
    void operator()(const std::vector<ql::wire_datum_t> &) const { }
    void operator()(const std::vector<ql::wire_datum_map_t> &) const { }
    void operator()(const rget_read_response_t::empty_t &) const { }
    void operator()(const rget_read_response_t::vec_t &) const { }

    void operator()(ql::wire_datum_t &d) const {
        d.finalize();
    }
    void operator()(ql::wire_datum_map_t &dm) const {
        dm.finalize();
    }
};

void rdb_rget_slice(btree_slice_t *slice, const key_range_t &range,
                    transaction_t *txn, superblock_t *superblock,
                    ql::env_t *ql_env,
                    const rdb_protocol_details::transform_t &transform,
                    const boost::optional<rdb_protocol_details::terminal_t> &terminal,
                    rget_read_response_t *response) {
    rdb_rget_depth_first_traversal_callback_t callback(txn, ql_env, transform, terminal, range, response);
    btree_depth_first_traversal(slice, txn, superblock, range, &callback);

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

