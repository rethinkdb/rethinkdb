#include <errors.hpp>
#include <boost/variant.hpp>
#include <boost/bind.hpp>

#include "btree/operations.hpp"
#include "btree/slice.hpp"
#include "concurrency/access.hpp"
#include "concurrency/pmap.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/archive/vector_stream.hpp"
#include "containers/iterators.hpp"
#include "memcached/btree/append_prepend.hpp"
#include "memcached/btree/delete.hpp"
#include "memcached/btree/distribution.hpp"
#include "memcached/btree/erase_range.hpp"
#include "memcached/btree/get.hpp"
#include "memcached/btree/get_cas.hpp"
#include "memcached/btree/incr_decr.hpp"
#include "memcached/btree/rget.hpp"
#include "memcached/btree/set.hpp"
#include "memcached/protocol.hpp"
#include "memcached/queries.hpp"
#include "stl_utils.hpp"
#include "serializer/config.hpp"

#include "btree/keys.hpp"

write_message_t &operator<<(write_message_t &msg, const intrusive_ptr_t<data_buffer_t> &buf) {
    if (buf) {
        bool exists = true;
        msg << exists;
        int64_t size = buf->size();
        msg << size;
        msg.append(buf->buf(), buf->size());
    } else {
        bool exists = false;
        msg << exists;
    }
    return msg;
}

int deserialize(read_stream_t *s, intrusive_ptr_t<data_buffer_t> *buf) {
    bool exists;
    int res = deserialize(s, &exists);
    if (res) { return res; }
    if (exists) {
        int64_t size;
        int res = deserialize(s, &size);
        if (res) { return res; }
        if (size < 0) { return ARCHIVE_RANGE_ERROR; }
        *buf = data_buffer_t::create(size);
        int64_t num_read = force_read(s, (*buf)->buf(), size);

        if (num_read == -1) { return ARCHIVE_SOCK_ERROR; }
        if (num_read < size) { return ARCHIVE_SOCK_EOF; }
        rassert(num_read == size);
    }
    return ARCHIVE_SUCCESS;
}

template<typename T>
class vector_backed_one_way_iterator_t : public one_way_iterator_t<T> {
    typename std::vector<T> data;
    size_t pos;
public:
    vector_backed_one_way_iterator_t() : pos(0) { }
    void push_back(T &v) { data.push_back(v); }
    typename boost::optional<T> next() {
        if (pos < data.size()) {
            return boost::optional<T>(data[pos++]);
        } else {
            return boost::optional<T>();
        }
    }
    void prefetch() { }
};


write_message_t &operator<<(write_message_t &msg, const rget_result_t &iter) {
    while (boost::optional<key_with_data_buffer_t> pair = iter->next()) {
        const key_with_data_buffer_t &kv = pair.get();

        const std::string &key = kv.key;
        const intrusive_ptr_t<data_buffer_t> &data = kv.value_provider;
        bool next = true;
        msg << next;
        msg << key;
        msg << data;
    }
    bool next = false;
    msg << next;
    return msg;
}

int deserialize(read_stream_t *s, rget_result_t *iter) {
    *iter = rget_result_t(new vector_backed_one_way_iterator_t<key_with_data_buffer_t>());
    bool next;
    for (;;) {
        int res = deserialize(s, &next);
        if (res) { return res; }
        if (!next) {
            return ARCHIVE_SUCCESS;
        }

        // TODO: See the load function above.  I'm guessing this code is never used.
        std::string key;
        res = deserialize(s, &key);
        if (res) { return res; }
        intrusive_ptr_t<data_buffer_t> data;
        res = deserialize(s, &data);
        if (res) { return res; }

        // You'll note that we haven't put the values in the vector-backed iterator.  Neither does the load function above...
    }
}

RDB_IMPL_SERIALIZABLE_1(get_query_t, key);
RDB_IMPL_SERIALIZABLE_4(rget_query_t, left_mode, left_key, right_mode, right_key);
RDB_IMPL_SERIALIZABLE_2(distribution_get_query_t, max_depth, range);
RDB_IMPL_SERIALIZABLE_3(get_result_t, value, flags, cas);
RDB_IMPL_SERIALIZABLE_3(key_with_data_buffer_t, key, mcflags, value_provider);
RDB_IMPL_SERIALIZABLE_1(distribution_result_t, key_counts);
RDB_IMPL_SERIALIZABLE_1(get_cas_mutation_t, key);
RDB_IMPL_SERIALIZABLE_7(sarc_mutation_t, key, data, flags, exptime, add_policy, replace_policy, old_cas);
RDB_IMPL_SERIALIZABLE_2(delete_mutation_t, key, dont_put_in_delete_queue);
RDB_IMPL_SERIALIZABLE_3(incr_decr_mutation_t, kind, key, amount);
RDB_IMPL_SERIALIZABLE_2(incr_decr_result_t, res, new_value);
RDB_IMPL_SERIALIZABLE_3(append_prepend_mutation_t, kind, key, data);
RDB_IMPL_SERIALIZABLE_6(backfill_atom_t, key, value, flags, exptime, recency, cas_or_zero);


/* `memcached_protocol_t::read_t::get_region()` */

static key_range_t::bound_t convert_bound_mode(rget_bound_mode_t rbm) {
    switch (rbm) {
        case rget_bound_open: return key_range_t::open;
        case rget_bound_closed: return key_range_t::closed;
        case rget_bound_none: return key_range_t::none;
        default: unreachable();
    }
}

struct read_get_region_visitor_t : public boost::static_visitor<key_range_t> {
    key_range_t operator()(get_query_t get) {
        return key_range_t(key_range_t::closed, get.key, key_range_t::closed, get.key);
    }
    key_range_t operator()(rget_query_t rget) {
        return key_range_t(
            convert_bound_mode(rget.left_mode),
            rget.left_key,
            convert_bound_mode(rget.right_mode),
            rget.right_key
            );
    }
    key_range_t operator()(distribution_get_query_t dst_get) {
        return dst_get.range;
    }
};

key_range_t memcached_protocol_t::read_t::get_region() const THROWS_NOTHING {
    read_get_region_visitor_t v;
    return boost::apply_visitor(v, query);
}

/* `memcached_protocol_t::read_t::shard()` */

struct read_shard_visitor_t : public boost::static_visitor<memcached_protocol_t::read_t> {
    read_shard_visitor_t(const key_range_t &r, exptime_t et) :
        region(r), effective_time(et) { }
    const key_range_t &region;
    exptime_t effective_time;
    memcached_protocol_t::read_t operator()(get_query_t get) {
        rassert(region == key_range_t(key_range_t::closed, get.key, key_range_t::closed, get.key));
        return memcached_protocol_t::read_t(get, effective_time);
    }
    memcached_protocol_t::read_t operator()(UNUSED rget_query_t original_rget) {
        rassert(region_is_superset(
            key_range_t(
                convert_bound_mode(original_rget.left_mode),
                original_rget.left_key,
                convert_bound_mode(original_rget.right_mode),
                original_rget.right_key
                ),
            region
            ));
        rget_query_t sub_rget;
        sub_rget.left_mode = rget_bound_closed;
        sub_rget.left_key = region.left;
        if (region.right.unbounded) {
            sub_rget.right_mode = rget_bound_none;
        } else {
            sub_rget.right_mode = rget_bound_open;
            sub_rget.right_key = region.right.key;
        }
        return memcached_protocol_t::read_t(sub_rget, effective_time);
    }
    memcached_protocol_t::read_t operator()(distribution_get_query_t distribution_get) {
        distribution_get.range = region;
        return memcached_protocol_t::read_t(distribution_get, effective_time);
    }
};

memcached_protocol_t::read_t memcached_protocol_t::read_t::shard(const key_range_t &r) const THROWS_NOTHING {
    read_shard_visitor_t v(r, effective_time);
    return boost::apply_visitor(v, query);
}

/* `memcached_protocol_t::read_t::unshard()` */

typedef merge_ordered_data_iterator_t<key_with_data_buffer_t, key_with_data_buffer_t::less> merged_results_iterator_t;

struct read_unshard_visitor_t : public boost::static_visitor<memcached_protocol_t::read_response_t> {
    std::vector<memcached_protocol_t::read_response_t> &bits;

    explicit read_unshard_visitor_t(std::vector<memcached_protocol_t::read_response_t> &b) : bits(b) { }
    memcached_protocol_t::read_response_t operator()(UNUSED get_query_t get) {
        rassert(bits.size() == 1);
        return memcached_protocol_t::read_response_t(boost::get<get_result_t>(bits[0].result));
    }
    memcached_protocol_t::read_response_t operator()(UNUSED rget_query_t rget) {
        boost::shared_ptr<merged_results_iterator_t> merge_iterator(new merged_results_iterator_t());
        for (int i = 0; i < (int)bits.size(); i++) {
            merge_iterator->add_mergee(boost::get<rget_result_t>(bits[i].result));
        }
        return memcached_protocol_t::read_response_t(rget_result_t(merge_iterator));
    }
    memcached_protocol_t::read_response_t operator()(distribution_get_query_t) {
        distribution_result_t res;
        for (int i = 0; i < (int)bits.size(); i++) {
            distribution_result_t *result = boost::get<distribution_result_t>(&bits[i].result);
            rassert(result, "Bad boost::get\n");
#ifndef NDEBUG
            for (std::map<std::string, int>::iterator it  = result->key_counts.begin();
                                                      it != result->key_counts.end();
                                                      ++it) {
                rassert(!std_contains(res.key_counts, it->first));
            }
#endif
            res.key_counts.insert(result->key_counts.begin(), result->key_counts.end());

        }
        return memcached_protocol_t::read_response_t(res);
    }
};

memcached_protocol_t::read_response_t memcached_protocol_t::read_t::unshard(std::vector<read_response_t> responses, UNUSED temporary_cache_t *cache) const THROWS_NOTHING {
    read_unshard_visitor_t v(responses);
    return boost::apply_visitor(v, query);
}

/* `memcached_protocol_t::write_t::get_region()` */

struct write_get_region_visitor_t : public boost::static_visitor<key_range_t> {
    /* All the types of mutation have a member called `key` */
    template<class mutation_t>
    key_range_t operator()(mutation_t mut) {
        return key_range_t(key_range_t::closed, mut.key, key_range_t::closed, mut.key);
    }
};

key_range_t memcached_protocol_t::write_t::get_region() const THROWS_NOTHING {
    write_get_region_visitor_t v;
    return apply_visitor(v, mutation);
}

/* `memcached_protocol_t::write_t::shard()` */

memcached_protocol_t::write_t memcached_protocol_t::write_t::shard(UNUSED key_range_t region) const THROWS_NOTHING {
    rassert(region == get_region());
    return *this;
}

/* `memcached_protocol_t::write_response_t::unshard()` */

memcached_protocol_t::write_response_t memcached_protocol_t::write_t::unshard(std::vector<memcached_protocol_t::write_response_t> responses, UNUSED temporary_cache_t *cache) const THROWS_NOTHING {
    /* TODO: Make sure the request type matches the response type */
    rassert(responses.size() == 1);
    return responses[0];
}

memcached_protocol_t::store_t::store_t(const std::string& filename, bool create, perfmon_collection_t *_perfmon_collection) 
    : store_view_t<memcached_protocol_t>(key_range_t::universe()),
      perfmon_collection(_perfmon_collection)
{
    if (create) {
        standard_serializer_t::create(
            standard_serializer_t::dynamic_config_t(),
            standard_serializer_t::private_dynamic_config_t(filename),
            standard_serializer_t::static_config_t()
            );
    }

    serializer.reset(new standard_serializer_t(
        standard_serializer_t::dynamic_config_t(),
        standard_serializer_t::private_dynamic_config_t(filename),
        perfmon_collection
        ));

    if (create) {
        mirrored_cache_static_config_t cache_static_config;
        cache_t::create(serializer.get(), &cache_static_config);
    }

    cache_dynamic_config.max_size = GIGABYTE;
    cache_dynamic_config.max_dirty_size = GIGABYTE / 2;
    cache.reset(new cache_t(serializer.get(), &cache_dynamic_config, perfmon_collection));

    if (create) {
        btree_slice_t::create(cache.get());
    }

    btree.reset(new btree_slice_t(cache.get(), perfmon_collection));

    if (create) {
        // Initialize metainfo to an empty `binary_blob_t` because its domain is
        // required to be `key_range_t::universe()` at all times
        /* Wow, this is a lot of lines of code for a simple concept. Can we do better? */
        boost::scoped_ptr<real_superblock_t> superblock;
        boost::scoped_ptr<transaction_t> txn;
        order_token_t order_token = order_source.check_in("memcached_protocol_t::store_t::store_t");
        order_token = btree->order_checkpoint_.check_through(order_token);
        get_btree_superblock(btree.get(), rwi_write, 1, repli_timestamp_t::invalid, order_token, &superblock, txn);
        buf_lock_t* sb_buf = superblock->get();
        clear_superblock_metainfo(txn.get(), sb_buf);

        vector_stream_t key;
        write_message_t msg;
        key_range_t kr = key_range_t::universe();   // `operator<<` needs a non-const reference
        msg << kr;
        DEBUG_ONLY_VAR int res = send_write_message(&key, &msg);
        rassert(!res);
        set_superblock_metainfo(txn.get(), sb_buf, key.vector(), std::vector<char>());
    }
}

memcached_protocol_t::store_t::~store_t() { }

void memcached_protocol_t::store_t::new_read_token(boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> &token_out) {
    fifo_enforcer_read_token_t token = token_source.enter_read();
    token_out.reset(new fifo_enforcer_sink_t::exit_read_t(&token_sink, token));
}

void memcached_protocol_t::store_t::new_write_token(boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> &token_out) {
    fifo_enforcer_write_token_t token = token_source.enter_write();
    token_out.reset(new fifo_enforcer_sink_t::exit_write_t(&token_sink, token));
}

void memcached_protocol_t::store_t::acquire_superblock_for_read(
        access_t access,
        bool snapshot,
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> &token,
        boost::scoped_ptr<transaction_t> &txn_out,
        boost::scoped_ptr<real_superblock_t> &sb_out,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    btree->assert_thread();

    boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> local_token;
    local_token.swap(token);
    wait_interruptible(local_token.get(), interruptor);

    order_token_t order_token = order_source.check_in("memcached_protocol_t::store_t::acquire_superblock_for_read");
    order_token = btree->order_checkpoint_.check_through(order_token);

    get_btree_superblock_for_reading(btree.get(), access, order_token, snapshot, &sb_out, txn_out);
}

void memcached_protocol_t::store_t::acquire_superblock_for_backfill(
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> &token,
        boost::scoped_ptr<transaction_t> &txn_out,
        boost::scoped_ptr<real_superblock_t> &sb_out,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    btree->assert_thread();

    boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> local_token;
    local_token.swap(token);
    wait_interruptible(local_token.get(), interruptor);

    order_token_t order_token = order_source.check_in("memcached_protocol_t::store_t::acquire_superblock_for_backfill");
    order_token = btree->order_checkpoint_.check_through(order_token);

    get_btree_superblock_for_backfilling(btree.get(), order_token, &sb_out, txn_out);
}

void memcached_protocol_t::store_t::acquire_superblock_for_write(
        access_t access,
        int expected_change_count,
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> &token,
        boost::scoped_ptr<transaction_t> &txn_out,
        boost::scoped_ptr<real_superblock_t> &sb_out,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    btree->assert_thread();

    boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> local_token;
    local_token.swap(token);
    wait_interruptible(local_token.get(), interruptor);

    order_token_t order_token = order_source.check_in("memcached_protocol_t::store_t::acquire_superblock_for_write");
    order_token = btree->order_checkpoint_.check_through(order_token);

    get_btree_superblock(btree.get(), access, expected_change_count, repli_timestamp_t::invalid, order_token, &sb_out, txn_out);
}

memcached_protocol_t::store_t::metainfo_t memcached_protocol_t::store_t::get_metainfo(
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> &token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    boost::scoped_ptr<real_superblock_t> superblock;
    boost::scoped_ptr<transaction_t> txn;
    acquire_superblock_for_read(rwi_read, false, token, txn, superblock, interruptor);

    return get_metainfo_internal(txn.get(), superblock->get());
}

region_map_t<memcached_protocol_t, binary_blob_t> memcached_protocol_t::store_t::get_metainfo_internal(transaction_t *txn, buf_lock_t *sb_buf) const THROWS_NOTHING {
    std::vector<std::pair<std::vector<char>, std::vector<char> > > kv_pairs;
    get_superblock_metainfo(txn, sb_buf, kv_pairs);   // FIXME: this is inefficient, cut out the middleman (vector)

    std::vector<std::pair<memcached_protocol_t::region_t, binary_blob_t> > result;
    for (std::vector<std::pair<std::vector<char>, std::vector<char> > >::iterator i = kv_pairs.begin(); i != kv_pairs.end(); ++i) {
        const std::vector<char> &value = i->second;

        memcached_protocol_t::region_t region;
        {
            vector_read_stream_t key(&i->first);
            DEBUG_ONLY_VAR int res = deserialize(&key, &region);
            rassert(!res);
        }

        result.push_back(std::make_pair(
            region,
            binary_blob_t(value.begin(), value.end())
        ));
    }
    region_map_t<memcached_protocol_t, binary_blob_t> res(result.begin(), result.end());
    rassert(res.get_domain() == key_range_t::universe());
    return res;
}

void memcached_protocol_t::store_t::set_metainfo(
        const metainfo_t &new_metainfo,
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> &token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    boost::scoped_ptr<real_superblock_t> superblock;
    boost::scoped_ptr<transaction_t> txn;
    acquire_superblock_for_write(rwi_write, 1, token, txn, superblock, interruptor);

    region_map_t<memcached_protocol_t, binary_blob_t> old_metainfo = get_metainfo_internal(txn.get(), superblock->get());
    update_metainfo(old_metainfo, new_metainfo, txn.get(), superblock.get());
}

struct read_visitor_t : public boost::static_visitor<memcached_protocol_t::read_response_t> {

    memcached_protocol_t::read_response_t operator()(const get_query_t& get) {
        return memcached_protocol_t::read_response_t(
            memcached_get(get.key, btree, effective_time, txn.get(), superblock.get()));
    }
    memcached_protocol_t::read_response_t operator()(const rget_query_t& rget) {
        return memcached_protocol_t::read_response_t(
            memcached_rget_slice(btree, rget.left_mode, rget.left_key, rget.right_mode, rget.right_key, effective_time, txn, superblock));
    }
    memcached_protocol_t::read_response_t operator()(const distribution_get_query_t& dget) {
        distribution_result_t dstr = memcached_distribution_get(btree, dget.max_depth, dget.range.left, effective_time, txn, superblock.get());

        for (std::map<std::string, int>::iterator it  = dstr.key_counts.begin();
                                                  it != dstr.key_counts.end();
                                                  /* increments done in loop */) {
            if (!dget.range.contains_key(store_key_t(it->first))) {
                dstr.key_counts.erase(it++);
            } else {
                ++it;
            }
        }

        return memcached_protocol_t::read_response_t(dstr);
    }


    read_visitor_t(btree_slice_t *btree_, boost::scoped_ptr<transaction_t>& txn_, boost::scoped_ptr<superblock_t> &superblock_, exptime_t effective_time_) :
        btree(btree_), txn(txn_), superblock(superblock_), effective_time(effective_time_) { }

private:
    btree_slice_t *btree;
    boost::scoped_ptr<transaction_t>& txn;
    boost::scoped_ptr<superblock_t> &superblock;
    exptime_t effective_time;
};

memcached_protocol_t::read_response_t memcached_protocol_t::store_t::read(
        DEBUG_ONLY(const metainfo_t& expected_metainfo, )
        const memcached_protocol_t::read_t &read,
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> &token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    boost::scoped_ptr<real_superblock_t> superblock;
    boost::scoped_ptr<transaction_t> txn;
    acquire_superblock_for_read(rwi_read, false, token, txn, superblock, interruptor);

    check_metainfo(DEBUG_ONLY(expected_metainfo, ) txn.get(), superblock.get());

    /* Ugly hack */
    boost::scoped_ptr<superblock_t> superblock2;
    superblock.swap(*reinterpret_cast<boost::scoped_ptr<real_superblock_t> *>(&superblock2));

    read_visitor_t v(btree.get(), txn, superblock2, read.effective_time);
    return boost::apply_visitor(v, read.query);
}

struct write_visitor_t : public boost::static_visitor<memcached_protocol_t::write_response_t> {
    memcached_protocol_t::write_response_t operator()(const get_cas_mutation_t &m) {
        return memcached_protocol_t::write_response_t(
            memcached_get_cas(m.key, btree, proposed_cas, effective_time, timestamp, txn.get(), superblock));
    }
    memcached_protocol_t::write_response_t operator()(const sarc_mutation_t &m) {
        return memcached_protocol_t::write_response_t(
            memcached_set(m.key, btree, m.data, m.flags, m.exptime, m.add_policy, m.replace_policy, m.old_cas, proposed_cas, effective_time, timestamp, txn.get(), superblock));
    }
    memcached_protocol_t::write_response_t operator()(const incr_decr_mutation_t &m) {
        return memcached_protocol_t::write_response_t(
            memcached_incr_decr(m.key, btree, (m.kind == incr_decr_INCR), m.amount, proposed_cas, effective_time, timestamp, txn.get(), superblock));
    }
    memcached_protocol_t::write_response_t operator()(const append_prepend_mutation_t &m) {
        return memcached_protocol_t::write_response_t(
            memcached_append_prepend(m.key, btree, m.data, (m.kind == append_prepend_APPEND), proposed_cas, effective_time, timestamp, txn.get(), superblock));
    }
    memcached_protocol_t::write_response_t operator()(const delete_mutation_t &m) {
        rassert(proposed_cas == INVALID_CAS);
        return memcached_protocol_t::write_response_t(
            memcached_delete(m.key, m.dont_put_in_delete_queue, btree, effective_time, timestamp, txn.get(), superblock));
    }

    write_visitor_t(btree_slice_t *btree_, boost::scoped_ptr<transaction_t>& txn_, superblock_t *superblock_, cas_t proposed_cas_, exptime_t effective_time_, repli_timestamp_t timestamp_) : btree(btree_), txn(txn_), superblock(superblock_), proposed_cas(proposed_cas_), effective_time(effective_time_), timestamp(timestamp_) { }

private:
    btree_slice_t *btree;
    boost::scoped_ptr<transaction_t>& txn;
    superblock_t *superblock;
    cas_t proposed_cas;
    exptime_t effective_time;
    repli_timestamp_t timestamp;
};

memcached_protocol_t::write_response_t memcached_protocol_t::store_t::write(
        DEBUG_ONLY(const metainfo_t& expected_metainfo, )
        const metainfo_t& new_metainfo,
        const memcached_protocol_t::write_t &write,
        transition_timestamp_t timestamp,
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> &token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    boost::scoped_ptr<real_superblock_t> superblock;
    boost::scoped_ptr<transaction_t> txn;
    const int expected_change_count = 2; // FIXME: this is incorrect, but will do for now
    acquire_superblock_for_write(rwi_write, expected_change_count, token, txn, superblock, interruptor);

    check_and_update_metainfo(DEBUG_ONLY(expected_metainfo, ) new_metainfo, txn.get(), superblock.get());

    write_visitor_t v(btree.get(), txn, superblock.get(), write.proposed_cas, write.effective_time, timestamp.to_repli_timestamp());
    return boost::apply_visitor(v, write.mutation);
}

struct memcached_backfill_callback_t : public backfill_callback_t {
    typedef memcached_protocol_t::backfill_chunk_t chunk_t;
    const boost::function<void(chunk_t)> &chunk_fun;

    explicit memcached_backfill_callback_t(const boost::function<void(chunk_t)> &chunk_fun_) : chunk_fun(chunk_fun_) { }

    void on_delete_range(const btree_key_t *left_exclusive, const btree_key_t *right_inclusive) {
        chunk_fun(chunk_t::delete_range(
            key_range_t(
                left_exclusive ? key_range_t::open : key_range_t::none, left_exclusive ? store_key_t(left_exclusive->size, left_exclusive->contents) : store_key_t(),
                right_inclusive ? key_range_t::closed : key_range_t::none, right_inclusive ? store_key_t(right_inclusive->size, right_inclusive->contents) : store_key_t()
            )
        ));
    }

    void on_deletion(const btree_key_t *key, UNUSED repli_timestamp_t recency) {
        chunk_fun(chunk_t::delete_key(to_store_key(key), recency));
    }

    void on_keyvalue(const backfill_atom_t& atom) {
        chunk_fun(chunk_t::set_key(atom));
    }
    ~memcached_backfill_callback_t() { }
protected:
    store_key_t to_store_key(const btree_key_t *key) {
        return store_key_t(key->size, key->contents);
    }
};

class refcount_superblock_t : public superblock_t {
public:
    refcount_superblock_t(superblock_t *sb, int rc) :
        sub_superblock(sb), refcount(rc) { }

    void release() {
        refcount--;
        rassert(refcount >= 0);
        if (refcount == 0) {
            sub_superblock->release();
            sub_superblock = NULL;
        }
    }

    block_id_t get_root_block_id() const {
        return sub_superblock->get_root_block_id();
    }

    void set_root_block_id(const block_id_t new_root_block) {
        sub_superblock->set_root_block_id(new_root_block);
    }

    block_id_t get_stat_block_id() const {
        return sub_superblock->get_stat_block_id();
    }

    void set_stat_block_id(block_id_t new_stat_block) {
        sub_superblock->set_stat_block_id(new_stat_block);
    }

    void set_eviction_priority(eviction_priority_t eviction_priority) {
        sub_superblock->set_eviction_priority(eviction_priority);
    }

    eviction_priority_t get_eviction_priority() {
        return sub_superblock->get_eviction_priority();
    }

private:
    superblock_t *sub_superblock;
    int refcount;
};

static void call_memcached_backfill(int i, btree_slice_t *btree, const std::vector<std::pair<memcached_protocol_t::region_t, state_timestamp_t> > &regions,
        backfill_callback_t *callback, transaction_t *txn, superblock_t *superblock, memcached_protocol_t::backfill_progress_t *progress) {
    traversal_progress_t *p = new traversal_progress_t;
    progress->add_constituent(p);
    repli_timestamp_t timestamp = regions[i].second.to_repli_timestamp();
    memcached_backfill(btree, regions[i].first, timestamp, callback, txn, superblock, p);
}

bool memcached_protocol_t::store_t::send_backfill(
        const region_map_t<memcached_protocol_t, state_timestamp_t> &start_point,
        const boost::function<bool(const metainfo_t&)> &should_backfill,
        const boost::function<void(memcached_protocol_t::backfill_chunk_t)> &chunk_fun,
        backfill_progress_t *progress,
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> &token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    boost::scoped_ptr<real_superblock_t> superblock;
    boost::scoped_ptr<transaction_t> txn;
    acquire_superblock_for_backfill(token, txn, superblock, interruptor);

    metainfo_t metainfo = get_metainfo_internal(txn.get(), superblock->get()).mask(start_point.get_domain());
    if (should_backfill(metainfo)) {
        memcached_backfill_callback_t callback(chunk_fun);

        std::vector<std::pair<memcached_protocol_t::region_t, state_timestamp_t> > regions(start_point.begin(), start_point.end());
        refcount_superblock_t refcount_wrapper(superblock.get(), regions.size());
        pmap(regions.size(), boost::bind(&call_memcached_backfill, _1,
            btree.get(), regions, &callback, txn.get(), &refcount_wrapper, progress));

        return true;
    } else {
        return false;
    }
}

struct receive_backfill_visitor_t : public boost::static_visitor<> {
    receive_backfill_visitor_t(btree_slice_t *btree_, transaction_t *txn_, superblock_t *superblock_, signal_t *interruptor_) : btree(btree_), txn(txn_), superblock(superblock_), interruptor(interruptor_) { }
    void operator()(const memcached_protocol_t::backfill_chunk_t::delete_key_t& delete_key) const {
        // FIXME: we ignored delete_key.recency here. Should we use it in place of repli_timestamp_t::invalid?
        memcached_delete(delete_key.key, true, btree, 0, repli_timestamp_t::invalid, txn, superblock);
    }
    void operator()(const memcached_protocol_t::backfill_chunk_t::delete_range_t& delete_range) const {
        const key_range_t& range = delete_range.range;
        range_key_tester_t tester(range);
        bool left_supplied = range.left.size > 0;
        bool right_supplied = !range.right.unbounded;
        memcached_erase_range(btree, &tester,
              left_supplied, range.left,
              right_supplied, range.right.key,
              txn, superblock);
    }
    void operator()(const memcached_protocol_t::backfill_chunk_t::key_value_pair_t& kv) const {
        const backfill_atom_t& bf_atom = kv.backfill_atom;
        memcached_set(bf_atom.key, btree,
            bf_atom.value, bf_atom.flags, bf_atom.exptime,
            add_policy_yes, replace_policy_yes, INVALID_CAS,
            // TODO: Should we pass bf_atom.recency in place of repli_timestamp_t::invalid here? Ask Sam.
            bf_atom.cas_or_zero, 0, repli_timestamp_t::invalid,
            txn, superblock);
    }
private:
    /* TODO: This might be redundant. I thought that `key_tester_t` was only
    originally necessary because in v1.1.x the hashing scheme might be different
    between the source and destination machines. */
    struct range_key_tester_t : public key_tester_t {
        explicit range_key_tester_t(const key_range_t& delete_range_) : delete_range(delete_range_) { }
        bool key_should_be_erased(const btree_key_t *key) {
            return delete_range.contains_key(key->contents, key->size);
        }

        const key_range_t& delete_range;
    };
    btree_slice_t *btree;
    transaction_t *txn;
    superblock_t *superblock;
    signal_t *interruptor;  // FIXME: interruptors are not used in btree code, so this one ignored.
};

void memcached_protocol_t::store_t::receive_backfill(
        const memcached_protocol_t::backfill_chunk_t &chunk,
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> &token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    boost::scoped_ptr<real_superblock_t> superblock;
    boost::scoped_ptr<transaction_t> txn;
    const int expected_change_count = 1; // FIXME: this is probably not correct

    acquire_superblock_for_write(rwi_write, expected_change_count, token, txn, superblock, interruptor);

    boost::apply_visitor(receive_backfill_visitor_t(btree.get(), txn.get(), superblock.get(), interruptor), chunk.val);
}

void memcached_protocol_t::store_t::reset_data(
        memcached_protocol_t::region_t subregion,
        const metainfo_t &new_metainfo,
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> &token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    boost::scoped_ptr<real_superblock_t> superblock;
    boost::scoped_ptr<transaction_t> txn;

    // We're passing 2 for the expected_change_count based on the
    // reasoning that we're probably going to touch a leaf-node-sized
    // range of keys and that it won't be aligned right on a leaf node
    // boundary.
    // TODO that's not reasonable; reset_data() is sometimes used to wipe out
    // entire databases.
    const int expected_change_count = 2;
    acquire_superblock_for_write(rwi_write, expected_change_count, token, txn, superblock, interruptor);

    region_map_t<memcached_protocol_t, binary_blob_t> old_metainfo = get_metainfo_internal(txn.get(), superblock->get());
    update_metainfo(old_metainfo, new_metainfo, txn.get(), superblock.get());

    always_true_key_tester_t key_tester;
    memcached_erase_range(btree.get(), &key_tester, subregion, txn.get(), superblock.get());
}

void memcached_protocol_t::store_t::check_and_update_metainfo(
        DEBUG_ONLY(const metainfo_t& expected_metainfo, )
        const metainfo_t &new_metainfo,
        transaction_t *txn,
        real_superblock_t *superblock) const
        THROWS_NOTHING {

    metainfo_t old_metainfo = check_metainfo(DEBUG_ONLY(expected_metainfo, ) txn, superblock);
    update_metainfo(old_metainfo, new_metainfo, txn, superblock);
}

memcached_protocol_t::store_t::metainfo_t memcached_protocol_t::store_t::check_metainfo(
        DEBUG_ONLY(const metainfo_t& expected_metainfo, )
        transaction_t *txn,
        real_superblock_t *superblock) const
        THROWS_NOTHING {

    region_map_t<memcached_protocol_t, binary_blob_t> old_metainfo = get_metainfo_internal(txn, superblock->get());
    rassert(old_metainfo.mask(expected_metainfo.get_domain()) == expected_metainfo);
    return old_metainfo;
}

void memcached_protocol_t::store_t::update_metainfo(const metainfo_t &old_metainfo, const metainfo_t &new_metainfo, transaction_t *txn, real_superblock_t *superblock) const THROWS_NOTHING {
    region_map_t<memcached_protocol_t, binary_blob_t> updated_metadata = old_metainfo;
    updated_metadata.update(new_metainfo);

    rassert(updated_metadata.get_domain() == key_range_t::universe());

    buf_lock_t* sb_buf = superblock->get();
    clear_superblock_metainfo(txn, sb_buf);

    for (region_map_t<memcached_protocol_t, binary_blob_t>::const_iterator i = updated_metadata.begin(); i != updated_metadata.end(); ++i) {

        vector_stream_t key;
        write_message_t msg;
        msg << i->first;
        DEBUG_ONLY_VAR int res = send_write_message(&key, &msg);
        rassert(!res);

        std::vector<char> value(static_cast<const char*>((*i).second.data()),
                                static_cast<const char*>((*i).second.data()) + (*i).second.size());
        set_superblock_metainfo(txn, sb_buf, key.vector(), value); // FIXME: this is not efficient either, see how value is created
    }
}

