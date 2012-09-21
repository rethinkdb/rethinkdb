#include "memcached/protocol.hpp"

#include "errors.hpp"
#include <boost/variant.hpp>
#include <boost/bind.hpp>

#include "btree/operations.hpp"
#include "btree/parallel_traversal.hpp"
#include "btree/slice.hpp"
#include "btree/superblock.hpp"
#include "concurrency/access.hpp"
#include "concurrency/pmap.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/iterators.hpp"
#include "containers/scoped.hpp"
#include "memcached/memcached_btree/append_prepend.hpp"
#include "memcached/memcached_btree/delete.hpp"
#include "memcached/memcached_btree/distribution.hpp"
#include "memcached/memcached_btree/erase_range.hpp"
#include "memcached/memcached_btree/get.hpp"
#include "memcached/memcached_btree/get_cas.hpp"
#include "memcached/memcached_btree/incr_decr.hpp"
#include "memcached/memcached_btree/rget.hpp"
#include "memcached/memcached_btree/set.hpp"
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
RDB_IMPL_SERIALIZABLE_2(rget_query_t, region, maximum);
RDB_IMPL_SERIALIZABLE_2(distribution_get_query_t, max_depth, region);
RDB_IMPL_SERIALIZABLE_3(get_result_t, value, flags, cas);
RDB_IMPL_SERIALIZABLE_3(key_with_data_buffer_t, key, mcflags, value_provider);
RDB_IMPL_SERIALIZABLE_2(rget_result_t, pairs, truncated);
RDB_IMPL_SERIALIZABLE_2(distribution_result_t, region, key_counts);
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

/* Wrap all our local types in anonymous namespaces so the linker doesn't
confuse them with other local types with the same name */
namespace {

struct read_get_region_visitor_t : public boost::static_visitor<region_t> {
    region_t operator()(get_query_t get) {
        return monokey_region(get.key);
    }
    region_t operator()(rget_query_t rget) {
        return rget.region;
    }
    region_t operator()(distribution_get_query_t dst_get) {
        return dst_get.region;
    }
};

}   /* anonymous namespace */

region_t read_t::get_region() const THROWS_NOTHING {
    read_get_region_visitor_t v;
    return boost::apply_visitor(v, query);
}

/* `read_t::shard()` */

namespace {

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
        rassert(region_is_superset(rget.region, region));
        rget.region = region;
        return read_t(rget, effective_time);
    }
    read_t operator()(distribution_get_query_t distribution_get) {
        rassert(region_is_superset(distribution_get.region, region));
        distribution_get.region = region;
        return read_t(distribution_get, effective_time);
    }
};

}   /* anonymous namespace */

read_t read_t::shard(const region_t &r) const THROWS_NOTHING {
    read_shard_visitor_t v(r, effective_time);
    return boost::apply_visitor(v, query);
}

/* `read_t::unshard()` */

namespace {

class key_with_data_buffer_less_t {
public:
    bool operator()(const key_with_data_buffer_t& x, const key_with_data_buffer_t& y) {
        int cmp = x.key.compare(y.key);

        // We should never have equal keys.
        rassert(cmp != 0);
        return cmp < 0;
    }
};

class distribution_result_less_t {
public:
    bool operator()(const distribution_result_t& x, const distribution_result_t& y) {
        return x.region < y.region;
    }
};

// TODO: get rid of this extra response_t copy on the stack
struct read_unshard_visitor_t : public boost::static_visitor<read_response_t> {
    const read_response_t *bits;
    const size_t count;

    explicit read_unshard_visitor_t(const read_response_t *b, size_t c) : bits(b), count(c) { }
    read_response_t operator()(UNUSED get_query_t get) {
        rassert(count == 1);
        return read_response_t(boost::get<get_result_t>(bits[0].result));
    }
    read_response_t operator()(rget_query_t rget) {
        // TODO: do this without dynamic memory?
        std::vector<key_with_data_buffer_t> pairs;
        for (size_t i = 0; i < count; ++i) {
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
        pairs.resize(ix);
        result.pairs.swap(pairs);
        result.truncated = (cumulative_size >= rget_max_chunk_size);

        return read_response_t(result);
    }

    read_response_t operator()(UNUSED distribution_get_query_t dget) {
        // TODO: do this without copying so much and/or without dynamic memory
        // Sort results by region
        std::vector<distribution_result_t> results(count);
        rassert(count > 0);

        for (size_t i = 0; i < count; ++i) {
            const distribution_result_t *result = boost::get<distribution_result_t>(&bits[i].result);
            rassert(result);
            results[i] = *result;
        }

        std::sort(results.begin(), results.end(), distribution_result_less_t());

        distribution_result_t res;
        size_t i = 0;
        while (i < results.size()) {
            // Find the largest hash shard for this key range
            key_range_t range = results[i].region.inner;
            size_t largest_index = i;
            size_t largest_size = 0;
            size_t total_range_keys = 0;

            while (i < results.size() && results[i].region.inner == range) {
                size_t tmp_total_keys = 0;
                for (std::map<store_key_t, int>::const_iterator mit = results[i].key_counts.begin();
                     mit != results[i].key_counts.end();
                     ++mit) {
                    tmp_total_keys += mit->second;
                }

                if (tmp_total_keys > largest_size) {
                    largest_size = tmp_total_keys;
                    largest_index = i;
                }

                total_range_keys += tmp_total_keys;
                ++i;
            }

            // Scale up the selected hash shard
            if (largest_size > 0) {
                double scale_factor = static_cast<double>(total_range_keys) / static_cast<double>(largest_size);

                rassert(scale_factor >= 1.0);  // Directly provable from the code above.

                for (std::map<store_key_t, int>::iterator mit = results[largest_index].key_counts.begin();
                     mit != results[largest_index].key_counts.end();
                     ++mit) {
                    mit->second = static_cast<int>(mit->second * scale_factor);
                }

                res.key_counts.insert(results[largest_index].key_counts.begin(), results[largest_index].key_counts.end());
            }
        }

        return read_response_t(res);
    }
};

}   /* anonymous namespace */

void read_t::unshard(const read_response_t *responses, size_t count, read_response_t *response, UNUSED context_t *ctx) const THROWS_NOTHING {
    read_unshard_visitor_t v(responses, count);
    *response = boost::apply_visitor(v, query);
}

/* `write_t::get_region()` */

namespace {

struct write_get_region_visitor_t : public boost::static_visitor<region_t> {
    /* All the types of mutation have a member called `key` */
    template<class mutation_t>
    region_t operator()(mutation_t mut) {
        return monokey_region(mut.key);
    }
};

}   /* anonymous namespace */

region_t write_t::get_region() const THROWS_NOTHING {
    write_get_region_visitor_t v;
    return boost::apply_visitor(v, mutation);
}

/* `write_t::shard()` */

write_t write_t::shard(DEBUG_VAR const region_t &region) const THROWS_NOTHING {
    rassert(region == get_region());
    return *this;
}

/* `write_response_t::unshard()` */

void write_t::unshard(const write_response_t *responses, DEBUG_VAR size_t count, write_response_t *response, UNUSED context_t *ctx) const THROWS_NOTHING {
    /* TODO: Make sure the request type matches the response type */
    rassert(count == 1);
    *response = responses[0];
}

namespace {

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

}   /* anonymous namespace */

region_t backfill_chunk_t::get_region() const THROWS_NOTHING {
    backfill_chunk_get_region_visitor_t v;
    return boost::apply_visitor(v, val);
}

namespace {

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

}   /* anonymous namespace */

backfill_chunk_t backfill_chunk_t::shard(const region_t &region) const THROWS_NOTHING {
    backfill_chunk_shard_visitor_t v(region);
    return boost::apply_visitor(v, val);
}

namespace {

struct backfill_chunk_get_btree_repli_timestamp_visitor_t : public boost::static_visitor<repli_timestamp_t> {
public:
    repli_timestamp_t operator()(const backfill_chunk_t::delete_key_t &del) {
        return del.recency;
    }
    repli_timestamp_t operator()(const backfill_chunk_t::delete_range_t &) {
        return repli_timestamp_t::invalid;
    }
    repli_timestamp_t operator()(const backfill_chunk_t::key_value_pair_t &kv) {
        return kv.backfill_atom.recency;
    }
};

}   /* anonymous namespace */

repli_timestamp_t backfill_chunk_t::get_btree_repli_timestamp() const THROWS_NOTHING {
    backfill_chunk_get_btree_repli_timestamp_visitor_t v;
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
                 int64_t cache_size,
                 bool create,
                 perfmon_collection_t *parent_perfmon_collection,
                 context_t *ctx) :
    btree_store_t<memcached_protocol_t>(io_backend, filename, cache_size, create, parent_perfmon_collection, ctx) { }

store_t::~store_t() {
    assert_thread();
}

namespace {

struct read_visitor_t : public boost::static_visitor<read_response_t> {
    read_response_t operator()(const get_query_t& get) {
        return read_response_t(
            memcached_get(get.key, btree, effective_time, txn, superblock));
    }

    read_response_t operator()(const rget_query_t& rget) {
        return read_response_t(
            memcached_rget_slice(btree, rget.region.inner, rget.maximum, effective_time, txn, superblock));
    }

    read_response_t operator()(const distribution_get_query_t& dget) {
        distribution_result_t dstr = memcached_distribution_get(btree, dget.max_depth, dget.region.inner.left, effective_time, txn, superblock);
        for (std::map<store_key_t, int>::iterator it  = dstr.key_counts.begin();
                                                  it != dstr.key_counts.end();
                                                  /* increments done in loop */) {
            if (!dget.region.inner.contains_key(store_key_t(it->first))) {
                dstr.key_counts.erase(it++);
            } else {
                ++it;
            }
        }

        dstr.region = dget.region;

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

}   /* anonymous namespace */

void store_t::protocol_read(const read_t &read,
                            read_response_t *response,
                            btree_slice_t *btree,
                            transaction_t *txn,
                            superblock_t *superblock) {
    read_visitor_t v(btree, txn, superblock, read.effective_time);
    *response = boost::apply_visitor(v, read.query);
}

namespace {

// TODO: get rid of this extra response_t copy on the stack
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

}   /* anonymous namespace */

void store_t::protocol_write(const write_t &write,
                             write_response_t *response,
                             transition_timestamp_t timestamp,
                             btree_slice_t *btree,
                             transaction_t *txn,
                             superblock_t *superblock) {
    // TODO: should this be calling to_repli_timestamp on a transition_timestamp_t?  Does this not use the timestamp-before, when we'd want the timestamp-after?
    write_visitor_t v(btree, txn, superblock, write.proposed_cas, write.effective_time, timestamp.to_repli_timestamp());
    *response = boost::apply_visitor(v, write.mutation);
}

class memcached_backfill_callback_t : public backfill_callback_t {
    typedef backfill_chunk_t chunk_t;
public:
    explicit memcached_backfill_callback_t(chunk_fun_callback_t<memcached_protocol_t> *chunk_fun_cb)
        : chunk_fun_cb_(chunk_fun_cb) { }

    void on_delete_range(const key_range_t &range, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        chunk_fun_cb_->send_chunk(chunk_t::delete_range(region_t(range)), interruptor);
    }

    void on_deletion(const btree_key_t *key, repli_timestamp_t recency, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        chunk_fun_cb_->send_chunk(chunk_t::delete_key(to_store_key(key), recency), interruptor);
    }

    void on_keyvalue(const backfill_atom_t& atom, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        chunk_fun_cb_->send_chunk(chunk_t::set_key(atom), interruptor);
    }
    ~memcached_backfill_callback_t() { }

protected:
    store_key_t to_store_key(const btree_key_t *key) {
        return store_key_t(key->size, key->contents);
    }

private:
    chunk_fun_callback_t<memcached_protocol_t> *chunk_fun_cb_;

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
                                     chunk_fun_callback_t<memcached_protocol_t> *chunk_fun_cb,
                                     superblock_t *superblock,
                                     btree_slice_t *btree,
                                     transaction_t *txn,
                                     backfill_progress_t *progress,
                                     signal_t *interruptor)
                                     THROWS_ONLY(interrupted_exc_t) {
    std::vector<std::pair<region_t, state_timestamp_t> > regions(start_point.begin(), start_point.end());

    if (regions.size() > 0) {
        memcached_backfill_callback_t callback(chunk_fun_cb);

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

namespace {

struct receive_backfill_visitor_t : public boost::static_visitor<> {
    receive_backfill_visitor_t(btree_slice_t *_btree, transaction_t *_txn, superblock_t *_superblock, signal_t *_interruptor) : btree(_btree), txn(_txn), superblock(_superblock), interruptor(_interruptor) { }
    void operator()(const backfill_chunk_t::delete_key_t& delete_key) const {
        memcached_delete(delete_key.key, true, btree, 0, delete_key.recency, txn, superblock);
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
            bf_atom.cas_or_zero, 0, bf_atom.recency,
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

}   /* anonymous namespace */

void store_t::protocol_receive_backfill(btree_slice_t *btree,
                                        transaction_t *txn,
                                        superblock_t *superblock,
                                        signal_t *interruptor,
                                        const backfill_chunk_t &chunk) {
    boost::apply_visitor(receive_backfill_visitor_t(btree, txn, superblock, interruptor), chunk.val);
}

namespace {

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

}   /* anonymous namespace */

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
