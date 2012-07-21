#include "memcached/protocol.hpp"

#include "errors.hpp"
#include <boost/variant.hpp>
#include <boost/bind.hpp>

#include "btree/operations.hpp"
#include "btree/parallel_traversal.hpp"
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

archive_result_t deserialize(read_stream_t *s, intrusive_ptr_t<data_buffer_t> *buf) {
    bool exists;
    archive_result_t res = deserialize(s, &exists);
    if (res) { return res; }
    if (exists) {
        int64_t size;
        res = deserialize(s, &size);
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

RDB_IMPL_SERIALIZABLE_1(get_query_t, key);
RDB_IMPL_SERIALIZABLE_2(rget_query_t, range, maximum);
RDB_IMPL_SERIALIZABLE_2(distribution_get_query_t, max_depth, range);
RDB_IMPL_SERIALIZABLE_3(get_result_t, value, flags, cas);
RDB_IMPL_SERIALIZABLE_3(key_with_data_buffer_t, key, mcflags, value_provider);
RDB_IMPL_SERIALIZABLE_2(rget_result_t, pairs, truncated);
RDB_IMPL_SERIALIZABLE_1(distribution_result_t, key_counts);
RDB_IMPL_SERIALIZABLE_1(get_cas_mutation_t, key);
RDB_IMPL_SERIALIZABLE_7(sarc_mutation_t, key, data, flags, exptime, add_policy, replace_policy, old_cas);
RDB_IMPL_SERIALIZABLE_2(delete_mutation_t, key, dont_put_in_delete_queue);
RDB_IMPL_SERIALIZABLE_3(incr_decr_mutation_t, kind, key, amount);
RDB_IMPL_SERIALIZABLE_2(incr_decr_result_t, res, new_value);
RDB_IMPL_SERIALIZABLE_3(append_prepend_mutation_t, kind, key, data);
RDB_IMPL_SERIALIZABLE_6(backfill_atom_t, key, value, flags, exptime, recency, cas_or_zero);


hash_region_t<key_range_t> monokey_region(const store_key_t &k) {
    uint64_t h = hash_region_hasher(k.contents(), k.size());
    return hash_region_t<key_range_t>(h, h + 1, key_range_t(key_range_t::closed, k, key_range_t::closed, k));
}

/* `memcached_protocol_t::read_t::get_region()` */

struct read_get_region_visitor_t : public boost::static_visitor<hash_region_t<key_range_t> > {
    hash_region_t<key_range_t> operator()(get_query_t get) {
        return monokey_region(get.key);
    }
    hash_region_t<key_range_t> operator()(rget_query_t rget) {
        // TODO: I bet this causes problems.
        return hash_region_t<key_range_t>(rget.range);
    }
    hash_region_t<key_range_t> operator()(distribution_get_query_t dst_get) {
        // TODO: Similarly, I bet this causes problems.
        return hash_region_t<key_range_t>(dst_get.range);
    }
};

hash_region_t<key_range_t> memcached_protocol_t::read_t::get_region() const THROWS_NOTHING {
    read_get_region_visitor_t v;
    return boost::apply_visitor(v, query);
}

/* `memcached_protocol_t::read_t::shard()` */

struct read_shard_visitor_t : public boost::static_visitor<memcached_protocol_t::read_t> {
    read_shard_visitor_t(const hash_region_t<key_range_t> &r, exptime_t et) :
        region(r), effective_time(et) { }

    const hash_region_t<key_range_t> &region;
    exptime_t effective_time;

    memcached_protocol_t::read_t operator()(get_query_t get) {
        rassert(region == monokey_region(get.key));
        return memcached_protocol_t::read_t(get, effective_time);
    }
    memcached_protocol_t::read_t operator()(rget_query_t rget) {
        rassert(region_is_superset(hash_region_t<key_range_t>(rget.range), region));
        // TODO: Reevaluate this code.  Should rget_query_t really have a key_range_t range?
        rget.range = region.inner;
        return memcached_protocol_t::read_t(rget, effective_time);
    }
    memcached_protocol_t::read_t operator()(distribution_get_query_t distribution_get) {
        rassert(region_is_superset(hash_region_t<key_range_t>(distribution_get.range), region));

        // TODO: Reevaluate this code.  Should distribution_get_query_t really have a key_range_t range?
        distribution_get.range = region.inner;
        return memcached_protocol_t::read_t(distribution_get, effective_time);
    }
};

memcached_protocol_t::read_t memcached_protocol_t::read_t::shard(const hash_region_t<key_range_t> &r) const THROWS_NOTHING {
    read_shard_visitor_t v(r, effective_time);
    return boost::apply_visitor(v, query);
}

/* `memcached_protocol_t::read_t::unshard()` */

class key_with_data_buffer_less_t {
public:
    bool operator()(const key_with_data_buffer_t& x, const key_with_data_buffer_t& y) {
        int cmp = x.key.compare(y.key);

        // We should never have equal keys.
        rassert(cmp != 0);
        return cmp < 0;
    }
};

struct read_unshard_visitor_t : public boost::static_visitor<memcached_protocol_t::read_response_t> {
    const std::vector<memcached_protocol_t::read_response_t> &bits;

    explicit read_unshard_visitor_t(const std::vector<memcached_protocol_t::read_response_t> &b) : bits(b) { }
    memcached_protocol_t::read_response_t operator()(UNUSED get_query_t get) {
        rassert(bits.size() == 1);
        return memcached_protocol_t::read_response_t(boost::get<get_result_t>(bits[0].result));
    }
    memcached_protocol_t::read_response_t operator()(rget_query_t rget) {
        std::map<store_key_t, const rget_result_t *> sorted_bits;
        for (int i = 0; i < int(bits.size()); i++) {
            const rget_result_t *bit = boost::get<rget_result_t>(&bits[i].result);
            if (!bit->pairs.empty()) {
                const store_key_t &key = bit->pairs.front().key;
                rassert(sorted_bits.count(key) == 0);
                sorted_bits.insert(std::make_pair(key, bit));
            }
        }
#ifndef NDEBUG
        store_key_t last;
#endif
        rget_result_t result;
        size_t cumulative_size = 0;
        for (std::map<store_key_t, const rget_result_t *>::iterator it = sorted_bits.begin(); it != sorted_bits.end(); it++) {
            if (cumulative_size >= rget_max_chunk_size || int(result.pairs.size()) > rget.maximum) {
                break;
            }
            for (std::vector<key_with_data_buffer_t>::const_iterator jt = it->second->pairs.begin(); jt != it->second->pairs.end(); jt++) {
                if (cumulative_size >= rget_max_chunk_size || int(result.pairs.size()) > rget.maximum) {
                    break;
                }
                result.pairs.push_back(*jt);
                cumulative_size += estimate_rget_result_pair_size(*jt);
#ifndef NDEBUG
                rassert(result.pairs.size() == 0 || jt->key > last);
                last = jt->key;
#endif
            }
        }
        if (cumulative_size >= rget_max_chunk_size) {
            result.truncated = true;
        } else {
            result.truncated = false;
        }
        return memcached_protocol_t::read_response_t(result);
    }
    memcached_protocol_t::read_response_t operator()(UNUSED distribution_get_query_t dget) {
        rassert(bits.size() > 0);
        rassert(boost::get<distribution_result_t>(&bits[0].result));
        rassert(bits.size() == 1 || boost::get<distribution_result_t>(&bits[1].result));

        // Asserts that we don't look like a hash-sharded thing.
        rassert(!(bits.size() > 1
                  && boost::get<distribution_result_t>(&bits[0].result)->key_counts.begin()->first == boost::get<distribution_result_t>(&bits[1].result)->key_counts.begin()->first));

        distribution_result_t res;

        for (int i = 0, e = bits.size(); i < e; i++) {
            const distribution_result_t *result = boost::get<distribution_result_t>(&bits[i].result);
            rassert(result, "Bad boost::get\n");

#ifndef NDEBUG
            for (std::map<store_key_t, int>::const_iterator it = result->key_counts.begin();
                 it != result->key_counts.end();
                 ++it) {
                rassert(!std_contains(res.key_counts, it->first), "repeated key '%*.*s'", int(it->first.size()), int(it->first.size()), it->first.contents());
            }
#endif
            res.key_counts.insert(result->key_counts.begin(), result->key_counts.end());

        }

        return memcached_protocol_t::read_response_t(res);
    }
};

memcached_protocol_t::read_response_t memcached_protocol_t::read_t::unshard(const std::vector<read_response_t>& responses, UNUSED temporary_cache_t *cache) const THROWS_NOTHING {
    read_unshard_visitor_t v(responses);
    return boost::apply_visitor(v, query);
}

struct read_multistore_unshard_visitor_t : public boost::static_visitor<memcached_protocol_t::read_response_t> {
    const std::vector<memcached_protocol_t::read_response_t> &bits;

    explicit read_multistore_unshard_visitor_t(const std::vector<memcached_protocol_t::read_response_t> &b) : bits(b) { }
    memcached_protocol_t::read_response_t operator()(UNUSED get_query_t get) {
        rassert(bits.size() == 1);
        return memcached_protocol_t::read_response_t(boost::get<get_result_t>(bits[0].result));
    }
    memcached_protocol_t::read_response_t operator()(rget_query_t rget) {
        std::vector<key_with_data_buffer_t> pairs;
        for (int i = 0; i < int(bits.size()); ++i) {
            const rget_result_t *bit = boost::get<rget_result_t>(&bits[i].result);
            pairs.insert(pairs.end(), bit->pairs.begin(), bit->pairs.end());
        }

        std::sort(pairs.begin(), pairs.end(), key_with_data_buffer_less_t());

        size_t cumulative_size = 0;
        int ix = 0;
        for (int e = std::min<int>(rget.maximum, pairs.size());
             ix < e && cumulative_size < rget_max_chunk_size;
             ++ix) {
            cumulative_size += estimate_rget_result_pair_size(pairs[ix]);
        }

        rget_result_t result;

        // TODO: This truncated value is based on the code below, and,
        // what if we also reached rget.maximum?  What if we reached
        // the end of the pairs?  We shouldn't be truncated then, no?
        result.truncated = (cumulative_size >= rget_max_chunk_size);

        pairs.resize(ix);
        result.pairs.swap(pairs);

        return memcached_protocol_t::read_response_t(result);
    }
    memcached_protocol_t::read_response_t operator()(UNUSED distribution_get_query_t dget) {
        rassert(bits.size() > 0);
        rassert(boost::get<distribution_result_t>(&bits[0].result));
        rassert(bits.size() == 1 || boost::get<distribution_result_t>(&bits[1].result));

        // These test properties of distribution queries sharded by hash rather than key.
        rassert(bits.size() > 1);
        rassert(boost::get<distribution_result_t>(&bits[0].result)->key_counts.begin()->first == boost::get<distribution_result_t>(&bits[1].result)->key_counts.begin()->first);

        distribution_result_t res;
        int64_t total_num_keys = 0;
        rassert(bits.size() > 0);

        int64_t total_keys_in_res = 0;
        for (int i = 0, e = bits.size(); i < e; ++i) {
            const distribution_result_t *result = boost::get<distribution_result_t>(&bits[i].result);
            rassert(result, "Bad boost::get\n");

            int64_t tmp_total_keys = 0;
            for (std::map<store_key_t, int>::const_iterator it = result->key_counts.begin();
                 it != result->key_counts.end();
                 ++it) {
                tmp_total_keys += it->second;
            }

            total_num_keys += tmp_total_keys;

            if (res.key_counts.size() < result->key_counts.size()) {
                res = *result;
                total_keys_in_res = tmp_total_keys;
            }
        }

        if (total_keys_in_res == 0) {
            return memcached_protocol_t::read_response_t(res);
        }

        double scale_factor = double(total_num_keys) / double(total_keys_in_res);

        rassert(scale_factor >= 1.0);  // Directly provable from the code above.

        for (std::map<store_key_t, int>::iterator it = res.key_counts.begin();
             it != res.key_counts.end();
             ++it) {
            it->second = static_cast<int>(it->second * scale_factor);
        }

        return memcached_protocol_t::read_response_t(res);
    }
};


memcached_protocol_t::read_response_t memcached_protocol_t::read_t::multistore_unshard(const std::vector<read_response_t>& responses, UNUSED temporary_cache_t *cache) const THROWS_NOTHING {
    read_multistore_unshard_visitor_t v(responses);
    return boost::apply_visitor(v, query);
}

/* `memcached_protocol_t::write_t::get_region()` */

struct write_get_region_visitor_t : public boost::static_visitor< hash_region_t<key_range_t> > {
    /* All the types of mutation have a member called `key` */
    template<class mutation_t>
    hash_region_t<key_range_t> operator()(mutation_t mut) {
        return monokey_region(mut.key);
    }
};

hash_region_t<key_range_t> memcached_protocol_t::write_t::get_region() const THROWS_NOTHING {
    write_get_region_visitor_t v;
    return boost::apply_visitor(v, mutation);
}

/* `memcached_protocol_t::write_t::shard()` */

memcached_protocol_t::write_t memcached_protocol_t::write_t::shard(DEBUG_ONLY_VAR const hash_region_t<key_range_t> &region) const THROWS_NOTHING {
    rassert(region == get_region());
    return *this;
}

/* `memcached_protocol_t::write_response_t::unshard()` */

memcached_protocol_t::write_response_t memcached_protocol_t::write_t::unshard(const std::vector<memcached_protocol_t::write_response_t>& responses, UNUSED temporary_cache_t *cache) const THROWS_NOTHING {
    /* TODO: Make sure the request type matches the response type */
    rassert(responses.size() == 1);
    return responses[0];
}

memcached_protocol_t::write_response_t memcached_protocol_t::write_t::multistore_unshard(const std::vector<memcached_protocol_t::write_response_t>& responses, temporary_cache_t *cache) const THROWS_NOTHING {
    return unshard(responses, cache);
}


struct backfill_chunk_get_region_visitor_t : public boost::static_visitor< hash_region_t<key_range_t> > {
    hash_region_t<key_range_t> operator()(const memcached_protocol_t::backfill_chunk_t::delete_key_t &del) {
        return monokey_region(del.key);
    }

    hash_region_t<key_range_t> operator()(const memcached_protocol_t::backfill_chunk_t::delete_range_t &del) {
        return del.range;
    }

    hash_region_t<key_range_t> operator()(const memcached_protocol_t::backfill_chunk_t::key_value_pair_t &kv) {
        return monokey_region(kv.backfill_atom.key);
    }
};


memcached_protocol_t::region_t memcached_protocol_t::backfill_chunk_t::get_region() const THROWS_NOTHING {
    backfill_chunk_get_region_visitor_t v;
    return boost::apply_visitor(v, val);
}

struct backfill_chunk_shard_visitor_t : public boost::static_visitor<memcached_protocol_t::backfill_chunk_t> {
public:
    explicit backfill_chunk_shard_visitor_t(const memcached_protocol_t::region_t &_region) : region(_region) { }
    memcached_protocol_t::backfill_chunk_t operator()(const memcached_protocol_t::backfill_chunk_t::delete_key_t &del) {
        memcached_protocol_t::backfill_chunk_t ret(del);
        rassert(region_is_superset(region, ret.get_region()));
        return ret;
    }
    memcached_protocol_t::backfill_chunk_t operator()(const memcached_protocol_t::backfill_chunk_t::delete_range_t &del) {
        memcached_protocol_t::region_t r = region_intersection(del.range, region);
        rassert(!region_is_empty(r));
        return memcached_protocol_t::backfill_chunk_t(memcached_protocol_t::backfill_chunk_t::delete_range_t(r));
    }
    memcached_protocol_t::backfill_chunk_t operator()(const memcached_protocol_t::backfill_chunk_t::key_value_pair_t &kv) {
        memcached_protocol_t::backfill_chunk_t ret(kv);
        rassert(region_is_superset(region, ret.get_region()));
        return ret;
    }
private:
    const memcached_protocol_t::region_t &region;
};

memcached_protocol_t::backfill_chunk_t memcached_protocol_t::backfill_chunk_t::shard(const memcached_protocol_t::region_t &region) const THROWS_NOTHING {
    backfill_chunk_shard_visitor_t v(region);
    return boost::apply_visitor(v, val);
}

hash_region_t<key_range_t> memcached_protocol_t::cpu_sharding_subspace(int subregion_number, int num_cpu_shards) {
    rassert(subregion_number >= 0);
    rassert(subregion_number < num_cpu_shards);

    // We have to be careful with the math here, to avoid overflow.
    uint64_t width = HASH_REGION_HASH_SIZE / num_cpu_shards;

    uint64_t beg = width * subregion_number;
    uint64_t end = subregion_number + 1 == num_cpu_shards ? HASH_REGION_HASH_SIZE : beg + width;

    return hash_region_t<key_range_t>(beg, end, key_range_t::universe());
}

memcached_protocol_t::store_t::store_t(io_backender_t *io_backender, const std::string& filename,
                                       bool create, perfmon_collection_t *_perfmon_collection)
    : store_view_t<memcached_protocol_t>(hash_region_t<key_range_t>::universe()),
      perfmon_collection(),
      perfmon_collection_membership(_perfmon_collection, &perfmon_collection, filename) {

    if (create) {

        standard_serializer_t::create(
            standard_serializer_t::dynamic_config_t(),
            io_backender,
            standard_serializer_t::private_dynamic_config_t(filename),
            standard_serializer_t::static_config_t(),
            &perfmon_collection
            );
    }

    serializer.init(new standard_serializer_t(
        standard_serializer_t::dynamic_config_t(),
        io_backender,
        standard_serializer_t::private_dynamic_config_t(filename),
        &perfmon_collection
        ));

    if (create) {
        mirrored_cache_static_config_t cache_static_config;
        cache_t::create(serializer.get(), &cache_static_config);
    }

    cache_dynamic_config.max_size = GIGABYTE;
    cache_dynamic_config.max_dirty_size = GIGABYTE / 2;
    cache.init(new cache_t(serializer.get(), &cache_dynamic_config, &perfmon_collection));

    if (create) {
        btree_slice_t::create(cache.get());
    }

    btree.init(new btree_slice_t(cache.get(), &perfmon_collection));

    if (create) {
        // Initialize metainfo to an empty `binary_blob_t` because its domain is
        // required to be `hash_region_t<key_range_t>::universe()` at all times
        /* Wow, this is a lot of lines of code for a simple concept. Can we do better? */

        scoped_ptr_t<transaction_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        order_token_t order_token = order_source.check_in("memcached_protocol_t::store_t::store_t");
        order_token = btree->order_checkpoint_.check_through(order_token);
        get_btree_superblock(btree.get(), rwi_write, 1, repli_timestamp_t::invalid, order_token, &superblock, &txn);
        buf_lock_t* sb_buf = superblock->get();
        clear_superblock_metainfo(txn.get(), sb_buf);

        vector_stream_t key;
        write_message_t msg;
        hash_region_t<key_range_t> kr = hash_region_t<key_range_t>::universe();   // `operator<<` needs a non-const reference  // TODO <- what
        msg << kr;
        DEBUG_ONLY_VAR int res = send_write_message(&key, &msg);
        rassert(!res);
        set_superblock_metainfo(txn.get(), sb_buf, key.vector(), std::vector<char>());
    }
}

memcached_protocol_t::store_t::~store_t() {
    assert_thread();
}

void memcached_protocol_t::store_t::new_read_token(scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> *token_out) {
    fifo_enforcer_read_token_t token = token_source.enter_read();
    token_out->init(new fifo_enforcer_sink_t::exit_read_t(&token_sink, token));
}

void memcached_protocol_t::store_t::new_write_token(scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *token_out) {
    fifo_enforcer_write_token_t token = token_source.enter_write();
    token_out->init(new fifo_enforcer_sink_t::exit_write_t(&token_sink, token));
}

void memcached_protocol_t::store_t::acquire_superblock_for_read(
        access_t access,
        scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> *token,
        scoped_ptr_t<transaction_t> *txn_out,
        scoped_ptr_t<real_superblock_t> *sb_out,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    btree->assert_thread();

    scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> local_token(token->release());
    wait_interruptible(local_token.get(), interruptor);

    order_token_t order_token = order_source.check_in("memcached_protocol_t::store_t::acquire_superblock_for_read");
    order_token = btree->order_checkpoint_.check_through(order_token);

    get_btree_superblock_for_reading(btree.get(), access, order_token, CACHE_SNAPSHOTTED_NO, sb_out, txn_out);
}

void memcached_protocol_t::store_t::acquire_superblock_for_backfill(
        scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> *token,
        scoped_ptr_t<transaction_t> *txn_out,
        scoped_ptr_t<real_superblock_t> *sb_out,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    btree->assert_thread();

    scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> local_token(token->release());
    wait_interruptible(local_token.get(), interruptor);

    order_token_t order_token = order_source.check_in("memcached_protocol_t::store_t::acquire_superblock_for_backfill");
    order_token = btree->order_checkpoint_.check_through(order_token);

    get_btree_superblock_for_backfilling(btree.get(), order_token, sb_out, txn_out);
}

void memcached_protocol_t::store_t::acquire_superblock_for_write(
        access_t access,
        int expected_change_count,
        scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *token,
        scoped_ptr_t<transaction_t> *txn_out,
        scoped_ptr_t<real_superblock_t> *sb_out,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    btree->assert_thread();

    scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> local_token(token->release());
    wait_interruptible(local_token.get(), interruptor);

    order_token_t order_token = order_source.check_in("memcached_protocol_t::store_t::acquire_superblock_for_write");
    order_token = btree->order_checkpoint_.check_through(order_token);

    get_btree_superblock(btree.get(), access, expected_change_count, repli_timestamp_t::invalid, order_token, sb_out, txn_out);
}

memcached_protocol_t::store_t::metainfo_t
memcached_protocol_t::store_t::get_metainfo(UNUSED order_token_t order_token,  // TODO
                                            scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> *token,
                                            signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_read(rwi_read, token, &txn, &superblock, interruptor);

    return get_metainfo_internal(txn.get(), superblock->get());
}

region_map_t<memcached_protocol_t, binary_blob_t> memcached_protocol_t::store_t::get_metainfo_internal(transaction_t *txn, buf_lock_t *sb_buf) const THROWS_NOTHING {
    std::vector<std::pair<std::vector<char>, std::vector<char> > > kv_pairs;
    get_superblock_metainfo(txn, sb_buf, &kv_pairs);   // FIXME: this is inefficient, cut out the middleman (vector)

    std::vector<std::pair<memcached_protocol_t::region_t, binary_blob_t> > result;
    for (std::vector<std::pair<std::vector<char>, std::vector<char> > >::iterator i = kv_pairs.begin(); i != kv_pairs.end(); ++i) {
        const std::vector<char> &value = i->second;

        memcached_protocol_t::region_t region;
        {
            vector_read_stream_t key(&i->first);
            DEBUG_ONLY_VAR archive_result_t res = deserialize(&key, &region);
            rassert(!res, "res = %d", res);
        }

        result.push_back(std::make_pair(
            region,
            binary_blob_t(value.begin(), value.end())
        ));
    }
    region_map_t<memcached_protocol_t, binary_blob_t> res(result.begin(), result.end());
    // TODO: What?  Why is res.get_domain() equal to universe?
    rassert(res.get_domain() == hash_region_t<key_range_t>::universe());
    return res;
}

void memcached_protocol_t::store_t::set_metainfo(const metainfo_t &new_metainfo,
                                                 UNUSED order_token_t order_token,  // TODO
                                                 scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *token,
                                                 signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_write(rwi_write, 1, token, &txn, &superblock, interruptor);

    region_map_t<memcached_protocol_t, binary_blob_t> old_metainfo = get_metainfo_internal(txn.get(), superblock->get());
    update_metainfo(old_metainfo, new_metainfo, txn.get(), superblock.get());
}

struct read_visitor_t : public boost::static_visitor<memcached_protocol_t::read_response_t> {

    memcached_protocol_t::read_response_t operator()(const get_query_t& get) {
        return memcached_protocol_t::read_response_t(memcached_get(get.key, btree, effective_time, txn, superblock));
    }
    memcached_protocol_t::read_response_t operator()(const rget_query_t& rget) {
        return memcached_protocol_t::read_response_t(memcached_rget_slice(btree, rget.range, rget.maximum, effective_time, txn, superblock));
    }
    memcached_protocol_t::read_response_t operator()(const distribution_get_query_t& dget) {
        distribution_result_t dstr = memcached_distribution_get(btree, dget.max_depth, dget.range.left, effective_time, txn, superblock);

        for (std::map<store_key_t, int>::iterator it  = dstr.key_counts.begin();
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


    read_visitor_t(btree_slice_t *btree_, transaction_t *txn_, superblock_t *superblock_, exptime_t effective_time_) :
        btree(btree_), txn(txn_), superblock(superblock_), effective_time(effective_time_) { }

private:
    btree_slice_t *btree;
    transaction_t *txn;
    superblock_t *superblock;
    exptime_t effective_time;
};

memcached_protocol_t::read_response_t memcached_protocol_t::store_t::read(
        DEBUG_ONLY(const metainfo_checker_t<memcached_protocol_t>& metainfo_checker, )
        const memcached_protocol_t::read_t &read,
        UNUSED order_token_t order_token,  // TODO
        scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> *token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_read(rwi_read, token, &txn, &superblock, interruptor);

    check_metainfo(DEBUG_ONLY(metainfo_checker, ) txn.get(), superblock.get());

    scoped_ptr_t<superblock_t> superblock2;
    superblock2.init(superblock.release());

    read_visitor_t v(btree.get(), txn.get(), superblock2.get(), read.effective_time);
    return boost::apply_visitor(v, read.query);
}

struct write_visitor_t : public boost::static_visitor<memcached_protocol_t::write_response_t> {
    memcached_protocol_t::write_response_t operator()(const get_cas_mutation_t &m) {
        return memcached_protocol_t::write_response_t(
            memcached_get_cas(m.key, btree, proposed_cas, effective_time, timestamp, txn, superblock));
    }
    memcached_protocol_t::write_response_t operator()(const sarc_mutation_t &m) {
        return memcached_protocol_t::write_response_t(
            memcached_set(m.key, btree, m.data, m.flags, m.exptime, m.add_policy, m.replace_policy, m.old_cas, proposed_cas, effective_time, timestamp, txn, superblock));
    }
    memcached_protocol_t::write_response_t operator()(const incr_decr_mutation_t &m) {
        return memcached_protocol_t::write_response_t(
            memcached_incr_decr(m.key, btree, (m.kind == incr_decr_INCR), m.amount, proposed_cas, effective_time, timestamp, txn, superblock));
    }
    memcached_protocol_t::write_response_t operator()(const append_prepend_mutation_t &m) {
        return memcached_protocol_t::write_response_t(
            memcached_append_prepend(m.key, btree, m.data, (m.kind == append_prepend_APPEND), proposed_cas, effective_time, timestamp, txn, superblock));
    }
    memcached_protocol_t::write_response_t operator()(const delete_mutation_t &m) {
        rassert(proposed_cas == INVALID_CAS);
        return memcached_protocol_t::write_response_t(
            memcached_delete(m.key, m.dont_put_in_delete_queue, btree, effective_time, timestamp, txn, superblock));
    }

    write_visitor_t(btree_slice_t *btree_, transaction_t *txn_, superblock_t *superblock_, cas_t proposed_cas_, exptime_t effective_time_, repli_timestamp_t timestamp_) : btree(btree_), txn(txn_), superblock(superblock_), proposed_cas(proposed_cas_), effective_time(effective_time_), timestamp(timestamp_) { }

private:
    btree_slice_t *btree;
    transaction_t *txn;
    superblock_t *superblock;
    cas_t proposed_cas;
    exptime_t effective_time;
    repli_timestamp_t timestamp;
};

memcached_protocol_t::write_response_t memcached_protocol_t::store_t::write(
        DEBUG_ONLY(const metainfo_checker_t<memcached_protocol_t>& metainfo_checker, )
        const metainfo_t& new_metainfo,
        const memcached_protocol_t::write_t &write,
        transition_timestamp_t timestamp,
        UNUSED order_token_t order_token,  // TODO
        scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    const int expected_change_count = 2; // FIXME: this is incorrect, but will do for now
    acquire_superblock_for_write(rwi_write, expected_change_count, token, &txn, &superblock, interruptor);

    check_and_update_metainfo(DEBUG_ONLY(metainfo_checker, ) new_metainfo, txn.get(), superblock.get());

    write_visitor_t v(btree.get(), txn.get(), superblock.get(), write.proposed_cas, write.effective_time, timestamp.to_repli_timestamp());
    return boost::apply_visitor(v, write.mutation);
}

class memcached_backfill_callback_t : public backfill_callback_t {
    typedef memcached_protocol_t::backfill_chunk_t chunk_t;
public:
    explicit memcached_backfill_callback_t(const boost::function<void(chunk_t)> &chunk_fun)
        : chunk_fun_(chunk_fun) { }

    void on_delete_range(const key_range_t &range) {
        chunk_fun_(chunk_t::delete_range(hash_region_t<key_range_t>(range)));
    }

    void on_deletion(const btree_key_t *key, UNUSED repli_timestamp_t recency) {
        chunk_fun_(chunk_t::delete_key(to_store_key(key), recency));
    }

    void on_keyvalue(const backfill_atom_t& atom) {
        chunk_fun_(chunk_t::set_key(atom));
    }
    ~memcached_backfill_callback_t() { }

protected:
    store_key_t to_store_key(const btree_key_t *key) {
        return store_key_t(key->size, key->contents);
    }

private:
    const boost::function<void(chunk_t)> &chunk_fun_;

    DISABLE_COPYING(memcached_backfill_callback_t);
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

static void call_memcached_backfill(int i, btree_slice_t *btree, const std::vector<std::pair<hash_region_t<key_range_t>, state_timestamp_t> > &regions,
        memcached_backfill_callback_t *callback, transaction_t *txn, superblock_t *superblock, memcached_protocol_t::backfill_progress_t *progress) {
    parallel_traversal_progress_t *p = new parallel_traversal_progress_t;
    progress->add_constituent(p);
    repli_timestamp_t timestamp = regions[i].second.to_repli_timestamp();
    memcached_backfill(btree, regions[i].first.inner, timestamp, callback, txn, superblock, p);
}

// TODO: Figure out wtf does the backfill filtering, figure out wtf constricts delete range operations to hit only a certain hash-interval, figure out what filters keys.
bool memcached_protocol_t::store_t::send_backfill(
        const region_map_t<memcached_protocol_t, state_timestamp_t> &start_point,
        const boost::function<bool(const metainfo_t&)> &should_backfill,
        const boost::function<void(memcached_protocol_t::backfill_chunk_t)> &chunk_fun,
        backfill_progress_t *progress,
        scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> *token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_backfill(token, &txn, &superblock, interruptor);

    metainfo_t metainfo = get_metainfo_internal(txn.get(), superblock->get()).mask(start_point.get_domain());
    if (should_backfill(metainfo)) {
        std::vector<std::pair<hash_region_t<key_range_t>, state_timestamp_t> > regions(start_point.begin(), start_point.end());

        if (regions.size() > 0) {
            memcached_backfill_callback_t callback(chunk_fun);

            // pmapping by regions.size() is now the arguably wrong
            // thing to do, because regions are not separate key
            // ranges.  On the other hand it's harmless, because
            // caching is basically perfect.
            refcount_superblock_t refcount_wrapper(superblock.get(), regions.size());
            pmap(regions.size(), boost::bind(&call_memcached_backfill, _1,
                                             btree.get(), regions, &callback, txn.get(), &refcount_wrapper, progress));
        }

        return true;
    } else {
        return false;
    }
}

struct receive_backfill_visitor_t : public boost::static_visitor<> {
    receive_backfill_visitor_t(btree_slice_t *_btree, transaction_t *_txn, superblock_t *_superblock, signal_t *_interruptor) : btree(_btree), txn(_txn), superblock(_superblock), interruptor(_interruptor) { }
    void operator()(const memcached_protocol_t::backfill_chunk_t::delete_key_t& delete_key) const {
        // FIXME: we ignored delete_key.recency here. Should we use it in place of repli_timestamp_t::invalid?
        memcached_delete(delete_key.key, true, btree, 0, repli_timestamp_t::invalid, txn, superblock);
    }
    void operator()(const memcached_protocol_t::backfill_chunk_t::delete_range_t& delete_range) const {
        hash_range_key_tester_t tester(delete_range.range);
        memcached_erase_range(btree, &tester, delete_range.range.inner, txn, superblock);
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
    struct hash_range_key_tester_t : public key_tester_t {
        explicit hash_range_key_tester_t(const hash_region_t<key_range_t>& delete_range) : delete_range_(delete_range) { }
        bool key_should_be_erased(const btree_key_t *key) {
            uint64_t h = hash_region_hasher(key->contents, key->size);
            return delete_range_.beg <= h && h < delete_range_.end
                && delete_range_.inner.contains_key(key->contents, key->size);
        }

        const hash_region_t<key_range_t>& delete_range_;

    private:
        DISABLE_COPYING(hash_range_key_tester_t);
    };
    btree_slice_t *btree;
    transaction_t *txn;
    superblock_t *superblock;
    signal_t *interruptor;  // FIXME: interruptors are not used in btree code, so this one ignored.
};

void memcached_protocol_t::store_t::receive_backfill(
        const memcached_protocol_t::backfill_chunk_t &chunk,
        scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    const int expected_change_count = 1; // FIXME: this is probably not correct

    acquire_superblock_for_write(rwi_write, expected_change_count, token, &txn, &superblock, interruptor);

    boost::apply_visitor(receive_backfill_visitor_t(btree.get(), txn.get(), superblock.get(), interruptor), chunk.val);
}

// TODO: Maybe hash_range_key_tester_t is redundant with this, since
// the key range test is redundant.
struct hash_key_tester_t : public key_tester_t {
    hash_key_tester_t(uint64_t beg, uint64_t end) : beg_(beg), end_(end) { }
    bool key_should_be_erased(const btree_key_t *key) {
        uint64_t h = hash_region_hasher(key->contents, key->size);
        return beg_ <= h && h < end_;
    }

private:
    uint64_t beg_;
    uint64_t end_;

    DISABLE_COPYING(hash_key_tester_t);
};

void memcached_protocol_t::store_t::reset_data(
        const hash_region_t<key_range_t>& subregion,
        const metainfo_t &new_metainfo,
        scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;

    // We're passing 2 for the expected_change_count based on the
    // reasoning that we're probably going to touch a leaf-node-sized
    // range of keys and that it won't be aligned right on a leaf node
    // boundary.
    // TODO that's not reasonable; reset_data() is sometimes used to wipe out
    // entire databases.
    const int expected_change_count = 2;
    acquire_superblock_for_write(rwi_write, expected_change_count, token, &txn, &superblock, interruptor);

    region_map_t<memcached_protocol_t, binary_blob_t> old_metainfo = get_metainfo_internal(txn.get(), superblock->get());
    update_metainfo(old_metainfo, new_metainfo, txn.get(), superblock.get());

    hash_key_tester_t key_tester(subregion.beg, subregion.end);
    memcached_erase_range(btree.get(), &key_tester, subregion.inner, txn.get(), superblock.get());
}

void memcached_protocol_t::store_t::check_and_update_metainfo(
        DEBUG_ONLY(const metainfo_checker_t<memcached_protocol_t>& metainfo_checker, )
        const metainfo_t &new_metainfo,
        transaction_t *txn,
        real_superblock_t *superblock) const
        THROWS_NOTHING {

    metainfo_t old_metainfo = check_metainfo(DEBUG_ONLY(metainfo_checker, ) txn, superblock);
    update_metainfo(old_metainfo, new_metainfo, txn, superblock);
}

memcached_protocol_t::store_t::metainfo_t memcached_protocol_t::store_t::check_metainfo(
        DEBUG_ONLY(const metainfo_checker_t<memcached_protocol_t>& metainfo_checker, )
        transaction_t *txn,
        real_superblock_t *superblock) const
        THROWS_NOTHING {

    region_map_t<memcached_protocol_t, binary_blob_t> old_metainfo = get_metainfo_internal(txn, superblock->get());
#ifndef NDEBUG
    metainfo_checker.check_metainfo(old_metainfo.mask(metainfo_checker.get_domain()));
#endif
    return old_metainfo;
}

void memcached_protocol_t::store_t::update_metainfo(const metainfo_t &old_metainfo, const metainfo_t &new_metainfo, transaction_t *txn, real_superblock_t *superblock) const THROWS_NOTHING {
    region_map_t<memcached_protocol_t, binary_blob_t> updated_metadata = old_metainfo;
    updated_metadata.update(new_metainfo);

    rassert(updated_metadata.get_domain() == hash_region_t<key_range_t>::universe());

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

class generic_debug_print_visitor_t : public boost::static_visitor<void> {
public:
    explicit generic_debug_print_visitor_t(append_only_printf_buffer_t *buf) : buf_(buf) { }

    template <class T>
    void operator()(const T& x) {
        debug_print(buf_, x);
    }

private:
    append_only_printf_buffer_t *buf_;
    DISABLE_COPYING(generic_debug_print_visitor_t);
};


// Debug printing impls
void debug_print(append_only_printf_buffer_t *buf, const memcached_protocol_t::write_t& write) {
    buf->appendf("mcwrite{");
    generic_debug_print_visitor_t v(buf);
    boost::apply_visitor(v, write.mutation);
    if (write.proposed_cas) {
        buf->appendf(", cas=%lu", write.proposed_cas);
    }
    if (write.effective_time) {
        buf->appendf(", efftime=%lu", write.effective_time);
    }

    buf->appendf("}");
}

void debug_print(append_only_printf_buffer_t *buf, const get_cas_mutation_t& mut) {
    buf->appendf("get_cas{");
    debug_print(buf, mut.key);
    buf->appendf("}");
}

void debug_print(append_only_printf_buffer_t *buf, const sarc_mutation_t& mut) {
    buf->appendf("sarc{");
    debug_print(buf, mut.key);
    // We don't print everything in the sarc yet.
    buf->appendf(", ...}");
}

void debug_print(append_only_printf_buffer_t *buf, const delete_mutation_t& mut) {
    buf->appendf("delete{");
    debug_print(buf, mut.key);
    buf->appendf(", dpidq=%s}", mut.dont_put_in_delete_queue ? "true" : "false");
}

void debug_print(append_only_printf_buffer_t *buf, const incr_decr_mutation_t& mut) {
    buf->appendf("incr_decr{%s, %lu, ", mut.kind == incr_decr_INCR ? "INCR" : mut.kind == incr_decr_DECR ? "DECR" : "???", mut.amount);
    debug_print(buf, mut.key);
    buf->appendf("}");
}

void debug_print(append_only_printf_buffer_t *buf, const append_prepend_mutation_t& mut) {
    buf->appendf("append_prepend{%s, ", mut.kind == append_prepend_APPEND ? "APPEND" : mut.kind == append_prepend_PREPEND ? "PREPEND" : "???");
    debug_print(buf, mut.key);
    // We don't print the data yet.
    buf->appendf(", ...}");
}

void debug_print(append_only_printf_buffer_t *buf, const memcached_protocol_t::backfill_chunk_t& chunk) {
    generic_debug_print_visitor_t v(buf);
    boost::apply_visitor(v, chunk.val);
}

void debug_print(append_only_printf_buffer_t *buf, const memcached_protocol_t::backfill_chunk_t::delete_key_t& del) {
    buf->appendf("bf::delete_key_t{key=");
    debug_print(buf, del.key);
    buf->appendf(", recency=");
    debug_print(buf, del.recency);
    buf->appendf("}");
}

void debug_print(append_only_printf_buffer_t *buf, const memcached_protocol_t::backfill_chunk_t::delete_range_t& del) {
    buf->appendf("bf::delete_range_t{range=");
    debug_print(buf, del.range);
    buf->appendf("}");
}

void debug_print(append_only_printf_buffer_t *buf, const memcached_protocol_t::backfill_chunk_t::key_value_pair_t& kvpair) {
    buf->appendf("bf::kv{atom=");
    debug_print(buf, kvpair.backfill_atom);
    buf->appendf("}");
}
