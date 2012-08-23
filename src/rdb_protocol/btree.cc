#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>

#include "btree/backfill.hpp"
#include "btree/depth_first_traversal.hpp"
#include "btree/erase_range.hpp"
#include "btree/operations.hpp"
#include "buffer_cache/blob.hpp"
#include "containers/archive/vector_stream.hpp"
#include "containers/scoped.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/environment.hpp"
#include "rdb_protocol/query_language.hpp"
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

boost::shared_ptr<scoped_cJSON_t> get_data(const rdb_value_t *value, transaction_t *txn) {
    blob_t blob(const_cast<rdb_value_t *>(value)->value_ref(), blob::btree_maxreflen);

    boost::shared_ptr<scoped_cJSON_t> data;

    /* Grab the data from the blob. */
    //TODO unnecessary copies, I hate them
    std::string serialized_data = blob.read_to_string(txn, 0, blob.valuesize());

    /* Deserialize the value and return it. */
    std::vector<char> data_vec(serialized_data.begin(), serialized_data.end());

    vector_read_stream_t read_stream(&data_vec);

    int res = deserialize(&read_stream, &data);
    guaranteef_err(res == 0, "corruption detected... this should probably be an exception\n");

    return data;
}

bool btree_value_fits(block_size_t bs, int data_length, const rdb_value_t *value) {
    return blob::ref_fits(bs, data_length, value->value_ref(), blob::btree_maxreflen);
}

point_read_response_t rdb_get(const store_key_t &store_key, btree_slice_t *slice, transaction_t *txn, superblock_t *superblock) {
    keyvalue_location_t<rdb_value_t> kv_location;
    find_keyvalue_location_for_read(txn, superblock, store_key.btree_key(), &kv_location, slice->root_eviction_priority, &slice->stats);

    if (!kv_location.value.has()) {
        return point_read_response_t(boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateNull())));
    }

    boost::shared_ptr<scoped_cJSON_t> data = get_data(kv_location.value.get(), txn);

    return point_read_response_t(data);
}

point_write_response_t rdb_set(const store_key_t &key, boost::shared_ptr<scoped_cJSON_t> data,
                       btree_slice_t *slice, repli_timestamp_t timestamp,
                       transaction_t *txn, superblock_t *superblock) {
    //block_size_t block_size = slice->cache()->get_block_size();

    keyvalue_location_t<rdb_value_t> kv_location;
    find_keyvalue_location_for_write(txn, superblock, key.btree_key(), &kv_location, &slice->root_eviction_priority, &slice->stats);
    bool already_existed = kv_location.value.has();

    scoped_malloc_t<rdb_value_t> new_value(MAX_RDB_VALUE_SIZE);
    bzero(new_value.get(), MAX_RDB_VALUE_SIZE);

    //TODO unnecessary copies they must go away.
    write_message_t wm;
    wm << data;
    vector_stream_t stream;
    int res = send_write_message(&stream, &wm);
    guaranteef_err(res == 0, "Serialization for json data failed... this shouldn't happen.\n");

    blob_t blob(new_value->value_ref(), blob::btree_maxreflen);

    //TODO more copies, good lord
    blob.append_region(txn, stream.vector().size());
    std::string sered_data(stream.vector().begin(), stream.vector().end());
    blob.write_from_string(sered_data, txn, 0);

    // Actually update the leaf, if needed.
    kv_location.value.reinterpret_swap(new_value);
    null_key_modification_callback_t<rdb_value_t> null_cb;
    apply_keyvalue_change(txn, &kv_location, key.btree_key(), timestamp, false, &null_cb, &slice->root_eviction_priority);
    //                                                                     ^-- That means the key isn't expired.

    return point_write_response_t(already_existed ? DUPLICATE : STORED);
}

class agnostic_rdb_backfill_callback_t : public agnostic_backfill_callback_t {
public:
    agnostic_rdb_backfill_callback_t(rdb_backfill_callback_t *cb, const key_range_t &kr) : cb_(cb), kr_(kr) { }

    void on_delete_range(const key_range_t &range) {
        rassert(kr_.is_superset(range));
        cb_->on_delete_range(range);
    }

    void on_deletion(const btree_key_t *key, repli_timestamp_t recency) {
        rassert(kr_.contains_key(key->contents, key->size));
        cb_->on_deletion(key, recency);
    }

    void on_pair(transaction_t *txn, repli_timestamp_t recency, const btree_key_t *key, const void *val) {
        rassert(kr_.contains_key(key->contents, key->size));
        const rdb_value_t *value = static_cast<const rdb_value_t *>(val);

        rdb_protocol_details::backfill_atom_t atom;
        atom.key.assign(key->size, key->contents);
        atom.value = get_data(value, txn);
        atom.recency = recency;
        cb_->on_keyvalue(atom);
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

point_delete_response_t rdb_delete(const store_key_t &key, btree_slice_t *slice, repli_timestamp_t timestamp, transaction_t *txn, superblock_t *superblock) {
    keyvalue_location_t<rdb_value_t> kv_location;
    find_keyvalue_location_for_write(txn, superblock, key.btree_key(), &kv_location, &slice->root_eviction_priority, &slice->stats);
    bool exists = kv_location.value.has();
    if(exists) {
        blob_t blob(kv_location.value->value_ref(), blob::btree_maxreflen);
        blob.clear(txn);
        kv_location.value.reset();
        null_key_modification_callback_t<rdb_value_t> null_cb;
        apply_keyvalue_change(txn, &kv_location, key.btree_key(), timestamp, false, &null_cb, &slice->root_eviction_priority);
    }

    return point_delete_response_t(exists ? DELETED : MISSING);
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

class rdb_rget_depth_first_traversal_callback_t : public depth_first_traversal_callback_t {
public:
    rdb_rget_depth_first_traversal_callback_t(transaction_t *txn, int max,
                                              query_language::runtime_environment_t *_env,
                                              const rdb_protocol_details::transform_t &_transform,
                                              boost::optional<rdb_protocol_details::terminal_t> _terminal,
                                              const key_range_t &range)
        : transaction(txn), maximum(max), cumulative_size(0),
          env(_env), transform(_transform), terminal(_terminal)
    {
        response.last_considered_key = range.left;

        if (terminal) {
            boost::apply_visitor(query_language::terminal_initializer_visitor_t(&response.result), *terminal);
        }
    }
    bool handle_pair(const btree_key_t* key, const void *value) {
        store_key_t store_key(key);
        if (response.last_considered_key < store_key) {
            response.last_considered_key = store_key;
        }

        const rdb_value_t *rdb_value = reinterpret_cast<const rdb_value_t *>(value);

        json_list_t data;
        data.push_back(get_data(rdb_value, transaction));

        //Apply transforms to the data
        typedef rdb_protocol_details::transform_t::iterator tit_t;
        for (tit_t it  = transform.begin();
                   it != transform.end();
                   ++it) {
            json_list_t tmp;

            for (json_list_t::iterator jt  = data.begin();
                                       jt != data.end();
                                       ++jt) {
                boost::apply_visitor(query_language::transform_visitor_t(*jt, &tmp, env), *it);
            }
            data.clear();
            data.splice(data.begin(), tmp);
        }

        if (!terminal) {
            typedef rget_read_response_t::stream_t stream_t;
            stream_t *stream = boost::get<stream_t>(&response.result);
            guarantee(stream);
            for (json_list_t::iterator it =  data.begin();
                                       it != data.end();
                                       ++it) {
                stream->push_back(std::make_pair(key, *it));
            }

            cumulative_size += estimate_rget_response_size(stream->back().second);
            // TODO: If we have to cast stream->size(), why is maximum an int?
            return static_cast<int>(stream->size()) < maximum && cumulative_size < rget_max_chunk_size;
        } else {
            for (json_list_t::iterator jt  = data.begin();
                                       jt != data.end();
                                       ++jt) {
                boost::apply_visitor(query_language::terminal_visitor_t(*jt, env, &response.result), *terminal);
            }
            return true;
        }
    }
    transaction_t *transaction;
    int maximum;
    rget_read_response_t response;
    size_t cumulative_size;
    query_language::runtime_environment_t *env;
    rdb_protocol_details::transform_t transform;
    boost::optional<rdb_protocol_details::terminal_t> terminal;
};

rget_read_response_t rdb_rget_slice(btree_slice_t *slice, const key_range_t &range,
                                    int maximum, transaction_t *txn, superblock_t *superblock,
                                    query_language::runtime_environment_t *env, const rdb_protocol_details::transform_t &transform,
                                    boost::optional<rdb_protocol_details::terminal_t> terminal) {
    rdb_rget_depth_first_traversal_callback_t callback(txn, maximum, env, transform, terminal, range);
    btree_depth_first_traversal(slice, txn, superblock, range, &callback);
    if (callback.cumulative_size >= rget_max_chunk_size) {
        callback.response.truncated = true;
    } else {
        callback.response.truncated = false;
    }
    return callback.response;
}

