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
#include "containers/archive/boost_types.hpp"
#include "containers/archive/vector_stream.hpp"
#include "containers/iterators.hpp"
#include "containers/scoped.hpp"
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

typedef memcached_protocol_t::store_t store_t;
typedef memcached_protocol_t::region_t region_t;

typedef memcached_protocol_t::read_t read_t;
typedef memcached_protocol_t::read_response_t read_response_t;

typedef memcached_protocol_t::write_t write_t;
typedef memcached_protocol_t::write_response_t write_response_t;

typedef memcached_protocol_t::backfill_chunk_t backfill_chunk_t;

const std::string memcached_protocol_t::protocol_name("memcached");

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

RDB_IMPL_SERIALIZABLE_1(memcached_protocol_t::read_response_t, result);
RDB_IMPL_SERIALIZABLE_2(memcached_protocol_t::read_t, query, effective_time);
RDB_IMPL_SERIALIZABLE_1(memcached_protocol_t::write_response_t, result);
RDB_IMPL_SERIALIZABLE_3(memcached_protocol_t::write_t, mutation, proposed_cas, effective_time);
RDB_IMPL_SERIALIZABLE_1(memcached_protocol_t::backfill_chunk_t::delete_key_t, key);
RDB_IMPL_SERIALIZABLE_1(memcached_protocol_t::backfill_chunk_t::delete_range_t, range);
RDB_IMPL_SERIALIZABLE_1(memcached_protocol_t::backfill_chunk_t::key_value_pair_t, backfill_atom);
RDB_IMPL_SERIALIZABLE_1(memcached_protocol_t::backfill_chunk_t, val);



region_t monokey_region(const store_key_t &k) {
    uint64_t h = hash_region_hasher(k.contents(), k.size());
    return region_t(h, h + 1, key_range_t(key_range_t::closed, k, key_range_t::closed, k));
}

/* `read_t::get_region()` */

struct read_get_region_visitor_t : public boost::static_visitor<region_t> {
    region_t operator()(get_query_t get) {
        return monokey_region(get.key);
    }
    region_t operator()(rget_query_t rget) {
        // TODO: I bet this causes problems.
        return region_t(rget.range);
    }
    region_t operator()(distribution_get_query_t dst_get) {
        // TODO: Similarly, I bet this causes problems.
        return region_t(dst_get.range);
    }
};

region_t read_t::get_region() const THROWS_NOTHING {
    read_get_region_visitor_t v;
    return boost::apply_visitor(v, query);
}

/* `read_t::shard()` */

struct read_shard_visitor_t : public boost::static_visitor<read_t> {
    read_shard_visitor_t(const region_t &r, exptime_t et) :
        region(r), effective_time(et) { }

    const region_t &region;
    exptime_t effective_time;

    read_t operator()(get_query_t get) {
        rassert(region == monokey_region(get.key));
        return read_t(get, effective_time);
    }
    read_t operator()(rget_query_t rget) {
        rassert(region_is_superset(region_t(rget.range), region));
        // TODO: Reevaluate this code.  Should rget_query_t really have a key_range_t range?
        rget.range = region.inner;
        return read_t(rget, effective_time);
    }
    read_t operator()(distribution_get_query_t distribution_get) {
        rassert(region_is_superset(region_t(distribution_get.range), region));

        // TODO: Reevaluate this code.  Should distribution_get_query_t really have a key_range_t range?
        distribution_get.range = region.inner;
        return read_t(distribution_get, effective_time);
    }
};

read_t read_t::shard(const region_t &r) const THROWS_NOTHING {
    read_shard_visitor_t v(r, effective_time);
    return boost::apply_visitor(v, query);
}

/* `read_t::unshard()` */

class key_with_data_buffer_less_t {
public:
    bool operator()(const key_with_data_buffer_t& x, const key_with_data_buffer_t& y) {
        int cmp = x.key.compare(y.key);

        // We should never have equal keys.
        rassert(cmp != 0);
        return cmp < 0;
    }
};

struct read_unshard_visitor_t : public boost::static_visitor<read_response_t> {
    const std::vector<read_response_t> &bits;

    explicit read_unshard_visitor_t(const std::vector<read_response_t> &b) : bits(b) { }
    read_response_t operator()(UNUSED get_query_t get) {
        rassert(bits.size() == 1);
        return read_response_t(boost::get<get_result_t>(bits[0].result));
    }
    read_response_t operator()(rget_query_t rget) {
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
        return read_response_t(result);
    }
    read_response_t operator()(UNUSED distribution_get_query_t dget) {
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

        return read_response_t(res);
    }
};

read_response_t read_t::unshard(const std::vector<read_response_t>& responses, UNUSED temporary_cache_t *cache) const THROWS_NOTHING {
    read_unshard_visitor_t v(responses);
    return boost::apply_visitor(v, query);
}

struct read_multistore_unshard_visitor_t : public boost::static_visitor<read_response_t> {
    const std::vector<read_response_t> &bits;

    explicit read_multistore_unshard_visitor_t(const std::vector<read_response_t> &b) : bits(b) { }
    read_response_t operator()(UNUSED get_query_t get) {
        rassert(bits.size() == 1);
        return read_response_t(boost::get<get_result_t>(bits[0].result));
    }
    read_response_t operator()(rget_query_t rget) {
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

        return read_response_t(result);
    }
    read_response_t operator()(UNUSED distribution_get_query_t dget) {
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
            return read_response_t(res);
        }

        double scale_factor = double(total_num_keys) / double(total_keys_in_res);

        rassert(scale_factor >= 1.0);  // Directly provable from the code above.

        for (std::map<store_key_t, int>::iterator it = res.key_counts.begin();
             it != res.key_counts.end();
             ++it) {
            it->second = static_cast<int>(it->second * scale_factor);
        }

        return read_response_t(res);
    }
};


read_response_t read_t::multistore_unshard(const std::vector<read_response_t>& responses, UNUSED temporary_cache_t *cache) const THROWS_NOTHING {
    read_multistore_unshard_visitor_t v(responses);
    return boost::apply_visitor(v, query);
}

/* `write_t::get_region()` */

struct write_get_region_visitor_t : public boost::static_visitor<region_t> {
    /* All the types of mutation have a member called `key` */
    template<class mutation_t>
    region_t operator()(mutation_t mut) {
        return monokey_region(mut.key);
    }
};

region_t write_t::get_region() const THROWS_NOTHING {
    write_get_region_visitor_t v;
    return boost::apply_visitor(v, mutation);
}

/* `write_t::shard()` */

write_t write_t::shard(DEBUG_ONLY_VAR const region_t &region) const THROWS_NOTHING {
    rassert(region == get_region());
    return *this;
}

/* `write_response_t::unshard()` */

write_response_t write_t::unshard(const std::vector<write_response_t>& responses, UNUSED temporary_cache_t *cache) const THROWS_NOTHING {
    /* TODO: Make sure the request type matches the response type */
    rassert(responses.size() == 1);
    return responses[0];
}

write_response_t write_t::multistore_unshard(const std::vector<write_response_t>& responses, temporary_cache_t *cache) const THROWS_NOTHING {
    return unshard(responses, cache);
}


struct backfill_chunk_get_region_visitor_t : public boost::static_visitor<region_t> {
    region_t operator()(const backfill_chunk_t::delete_key_t &del) {
        return monokey_region(del.key);
    }

    region_t operator()(const backfill_chunk_t::delete_range_t &del) {
        return del.range;
    }

    region_t operator()(const backfill_chunk_t::key_value_pair_t &kv) {
        return monokey_region(kv.backfill_atom.key);
    }
};


region_t backfill_chunk_t::get_region() const THROWS_NOTHING {
    backfill_chunk_get_region_visitor_t v;
    return boost::apply_visitor(v, val);
}

struct backfill_chunk_shard_visitor_t : public boost::static_visitor<backfill_chunk_t> {
public:
    explicit backfill_chunk_shard_visitor_t(const region_t &_region) : region(_region) { }
    backfill_chunk_t operator()(const backfill_chunk_t::delete_key_t &del) {
        backfill_chunk_t ret(del);
        rassert(region_is_superset(region, ret.get_region()));
        return ret;
    }
    backfill_chunk_t operator()(const backfill_chunk_t::delete_range_t &del) {
        region_t r = region_intersection(del.range, region);
        rassert(!region_is_empty(r));
        return backfill_chunk_t(backfill_chunk_t::delete_range_t(r));
    }
    backfill_chunk_t operator()(const backfill_chunk_t::key_value_pair_t &kv) {
        backfill_chunk_t ret(kv);
        rassert(region_is_superset(region, ret.get_region()));
        return ret;
    }
private:
    const region_t &region;
};

backfill_chunk_t backfill_chunk_t::shard(const region_t &region) const THROWS_NOTHING {
    backfill_chunk_shard_visitor_t v(region);
    return boost::apply_visitor(v, val);
}

region_t memcached_protocol_t::cpu_sharding_subspace(int subregion_number, int num_cpu_shards) {
    rassert(subregion_number >= 0);
    rassert(subregion_number < num_cpu_shards);

    // We have to be careful with the math here, to avoid overflow.
    uint64_t width = HASH_REGION_HASH_SIZE / num_cpu_shards;

    uint64_t beg = width * subregion_number;
    uint64_t end = subregion_number + 1 == num_cpu_shards ? HASH_REGION_HASH_SIZE : beg + width;

    return region_t(beg, end, key_range_t::universe());
}

store_t::store_t(io_backender_t *io_backend,
                 const std::string& filename,
                 bool create,
                 perfmon_collection_t *parent_perfmon_collection,
                 context_t *ctx) :
    btree_store_t<memcached_protocol_t>(io_backend, filename, create, parent_perfmon_collection, ctx) { }

store_t::~store_t() {
    assert_thread();
}

struct read_visitor_t : public boost::static_visitor<read_response_t> {
    read_response_t operator()(const get_query_t& get) {
        return read_response_t(
            memcached_get(get.key, btree, effective_time, txn, superblock));
    }

    read_response_t operator()(const rget_query_t& rget) {
        return read_response_t(
            memcached_rget_slice(btree, rget.range, rget.maximum, effective_time, txn, superblock));
    }

    read_response_t operator()(const distribution_get_query_t& dget) {
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

        return read_response_t(dstr);
    }

    read_visitor_t(btree_slice_t *btree_, transaction_t *txn_, superblock_t *superblock_, exptime_t effective_time_) :
        btree(btree_), txn(txn_), superblock(superblock_), effective_time(effective_time_) { }

private:
    btree_slice_t *btree;
    transaction_t *txn;
    superblock_t *superblock;
    exptime_t effective_time;
};

read_response_t store_t::protocol_read(const read_t &read,
                                       btree_slice_t *btree,
                                       transaction_t *txn,
                                       superblock_t *superblock) {
    read_visitor_t v(btree, txn, superblock, read.effective_time);
    return boost::apply_visitor(v, read.query);
}

struct write_visitor_t : public boost::static_visitor<write_response_t> {
    write_response_t operator()(const get_cas_mutation_t &m) {
        return write_response_t(
            memcached_get_cas(m.key, btree, proposed_cas, effective_time, timestamp, txn, superblock));
    }
    write_response_t operator()(const sarc_mutation_t &m) {
        return write_response_t(
            memcached_set(m.key, btree, m.data, m.flags, m.exptime, m.add_policy, m.replace_policy, m.old_cas, proposed_cas, effective_time, timestamp, txn, superblock));
    }
    write_response_t operator()(const incr_decr_mutation_t &m) {
        return write_response_t(
            memcached_incr_decr(m.key, btree, (m.kind == incr_decr_INCR), m.amount, proposed_cas, effective_time, timestamp, txn, superblock));
    }
    write_response_t operator()(const append_prepend_mutation_t &m) {
        return write_response_t(
            memcached_append_prepend(m.key, btree, m.data, (m.kind == append_prepend_APPEND), proposed_cas, effective_time, timestamp, txn, superblock));
    }
    write_response_t operator()(const delete_mutation_t &m) {
        rassert(proposed_cas == INVALID_CAS);
        return write_response_t(
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

write_response_t store_t::protocol_write(const write_t &write,
                                         transition_timestamp_t timestamp,
                                         btree_slice_t *btree,
                                         transaction_t *txn,
                                         superblock_t *superblock) {
    write_visitor_t v(btree, txn, superblock, write.proposed_cas, write.effective_time, timestamp.to_repli_timestamp());
    return boost::apply_visitor(v, write.mutation);
}

class memcached_backfill_callback_t : public backfill_callback_t {
    typedef backfill_chunk_t chunk_t;
public:
    explicit memcached_backfill_callback_t(const boost::function<void(chunk_t)> &chunk_fun)
        : chunk_fun_(chunk_fun) { }

    void on_delete_range(const key_range_t &range) {
        chunk_fun_(chunk_t::delete_range(region_t(range)));
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

static void call_memcached_backfill(int i, btree_slice_t *btree, const std::vector<std::pair<region_t, state_timestamp_t> > &regions,
        memcached_backfill_callback_t *callback, transaction_t *txn, superblock_t *superblock, memcached_protocol_t::backfill_progress_t *progress,
        signal_t *interruptor) {
    parallel_traversal_progress_t *p = new parallel_traversal_progress_t;
    scoped_ptr_t<traversal_progress_t> p_owner(p);
    progress->add_constituent(&p_owner);
    repli_timestamp_t timestamp = regions[i].second.to_repli_timestamp();
    try {
        memcached_backfill(btree, regions[i].first.inner, timestamp, callback, txn, superblock, p, interruptor);
    } catch (interrupted_exc_t) {
        /* do nothing; `protocol_send_backfill()` will notice and deal with it.
        */
    }
}

// TODO: Figure out wtf does the backfill filtering, figure out wtf constricts delete range operations to hit only a certain hash-interval, figure out what filters keys.
void store_t::protocol_send_backfill(const region_map_t<memcached_protocol_t, state_timestamp_t> &start_point,
                                     const boost::function<void(backfill_chunk_t)> &chunk_fun,
                                     superblock_t *superblock,
                                     btree_slice_t *btree,
                                     transaction_t *txn,
                                     backfill_progress_t *progress,
                                     signal_t *interruptor)
                                     THROWS_ONLY(interrupted_exc_t)
{
    std::vector<std::pair<region_t, state_timestamp_t> > regions(start_point.begin(), start_point.end());

    if (regions.size() > 0) {
        memcached_backfill_callback_t callback(chunk_fun);

        // pmapping by regions.size() is now the arguably wrong thing to do,
        // because adjacent regions often have the same value. On the other hand
        // it's harmless, because caching is basically perfect.
        refcount_superblock_t refcount_wrapper(superblock, regions.size());
        pmap(regions.size(), boost::bind(&call_memcached_backfill, _1,
                                         btree, regions, &callback, txn, &refcount_wrapper, progress, interruptor));

        /* if interruptor was pulsed in `call_memcached_backfill()`, it returned
        normally anyway. So now we have to check manually. */
        if (interruptor->is_pulsed()) {
            throw interrupted_exc_t();
        }
    }
}

struct receive_backfill_visitor_t : public boost::static_visitor<> {
    receive_backfill_visitor_t(btree_slice_t *_btree, transaction_t *_txn, superblock_t *_superblock, signal_t *_interruptor) : btree(_btree), txn(_txn), superblock(_superblock), interruptor(_interruptor) { }
    void operator()(const backfill_chunk_t::delete_key_t& delete_key) const {
        // FIXME: we ignored delete_key.recency here. Should we use it in place of repli_timestamp_t::invalid?
        memcached_delete(delete_key.key, true, btree, 0, repli_timestamp_t::invalid, txn, superblock);
    }
    void operator()(const backfill_chunk_t::delete_range_t& delete_range) const {
        hash_range_key_tester_t tester(delete_range.range);
        memcached_erase_range(btree, &tester, delete_range.range.inner, txn, superblock);
    }
    void operator()(const backfill_chunk_t::key_value_pair_t& kv) const {
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
        explicit hash_range_key_tester_t(const region_t &delete_range) : delete_range_(delete_range) { }
        bool key_should_be_erased(const btree_key_t *key) {
            uint64_t h = hash_region_hasher(key->contents, key->size);
            return delete_range_.beg <= h && h < delete_range_.end
                && delete_range_.inner.contains_key(key->contents, key->size);
        }

        const region_t &delete_range_;

    private:
        DISABLE_COPYING(hash_range_key_tester_t);
    };
    btree_slice_t *btree;
    transaction_t *txn;
    superblock_t *superblock;
    signal_t *interruptor;  // FIXME: interruptors are not used in btree code, so this one ignored.
};

void store_t::protocol_receive_backfill(btree_slice_t *btree,
                                        transaction_t *txn,
                                        superblock_t *superblock,
                                        signal_t *interruptor,
                                        const backfill_chunk_t &chunk) {
    boost::apply_visitor(receive_backfill_visitor_t(btree, txn, superblock, interruptor), chunk.val);
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

void store_t::protocol_reset_data(const region_t& subregion,
                                  btree_slice_t *btree,
                                  transaction_t *txn,
                                  superblock_t *superblock) {
    hash_key_tester_t key_tester(subregion.beg, subregion.end);
    memcached_erase_range(btree, &key_tester, subregion.inner, txn, superblock);
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
void debug_print(append_only_printf_buffer_t *buf, const write_t& write) {
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

void debug_print(append_only_printf_buffer_t *buf, const backfill_chunk_t& chunk) {
    generic_debug_print_visitor_t v(buf);
    boost::apply_visitor(v, chunk.val);
}

void debug_print(append_only_printf_buffer_t *buf, const backfill_chunk_t::delete_key_t& del) {
    buf->appendf("bf::delete_key_t{key=");
    debug_print(buf, del.key);
    buf->appendf(", recency=");
    debug_print(buf, del.recency);
    buf->appendf("}");
}

void debug_print(append_only_printf_buffer_t *buf, const backfill_chunk_t::delete_range_t& del) {
    buf->appendf("bf::delete_range_t{range=");
    debug_print(buf, del.range);
    buf->appendf("}");
}

void debug_print(append_only_printf_buffer_t *buf, const backfill_chunk_t::key_value_pair_t& kvpair) {
    buf->appendf("bf::kv{atom=");
    debug_print(buf, kvpair.backfill_atom);
    buf->appendf("}");
}
