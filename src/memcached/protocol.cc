// Copyright 2010-2013 RethinkDB, all rights reserved.
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

write_message_t &operator<<(write_message_t &msg, const counted_t<data_buffer_t> &buf) {
    if (buf.has()) {
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

archive_result_t deserialize(read_stream_t *s, counted_t<data_buffer_t> *buf) {
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
        guarantee(num_read == size);
    }
    return ARCHIVE_SUCCESS;
}

RDB_IMPL_SERIALIZABLE_1(get_query_t, key);
RDB_IMPL_SERIALIZABLE_2(rget_query_t, region, maximum);
RDB_IMPL_SERIALIZABLE_3(distribution_get_query_t, max_depth, result_limit, region);
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

struct read_shard_visitor_t : public boost::static_visitor<bool> {
    read_shard_visitor_t(exptime_t et, const region_t *_region,
                         read_t *_read_out)
        : effective_time(et), region(_region), read_out(_read_out) { }

    bool operator()(const get_query_t &get) const {
        const bool ret = region_contains_key(*region, get.key);
        if (ret) {
            *read_out = read_t(get, effective_time);
        }
        return ret;
    }

    template <class T>
    bool rangey_query(const T &arg) const {
        const hash_region_t<key_range_t> intersection
            = region_intersection(*region, arg.region);
        if (!region_is_empty(intersection)) {
            T tmp = arg;
            tmp.region = intersection;
            *read_out = read_t(tmp, effective_time);
            return true;
        } else {
            return false;
        }
    }

    bool operator()(const rget_query_t &rget) const {
        return rangey_query(rget);
    }

    bool operator()(const distribution_get_query_t &distribution_get) const {
        return rangey_query(distribution_get);
    }

private:
    const exptime_t effective_time;
    const region_t *region;
    read_t *read_out;
};

}   /* anonymous namespace */

bool read_t::shard(const region_t &region,
                   read_t *read_out) const THROWS_NOTHING {
    return boost::apply_visitor(read_shard_visitor_t(effective_time, &region, read_out), query);
}




/* `read_t::unshard()` */

namespace {

class key_with_data_buffer_less_t {
public:
    bool operator()(const key_with_data_buffer_t& x, const key_with_data_buffer_t& y) {
        int cmp = x.key.compare(y.key);

        // We should never have equal keys.
        guarantee(cmp != 0);
        return cmp < 0;
    }
};

class distribution_result_less_t {
public:
    bool operator()(const distribution_result_t& x, const distribution_result_t& y) {
        if (x.region.inner == y.region.inner) {
            return x.region < y.region;
        } else {
            return x.region.inner < y.region.inner;
        }
    }
};

// Scale the distribution down by combining ranges to fit it within the limit of the query
void scale_down_distribution(size_t result_limit, std::map<store_key_t, int64_t> *key_counts) {
    const size_t combine = (key_counts->size() / result_limit); // Combine this many other ranges into the previous range
    for (std::map<store_key_t, int64_t>::iterator it = key_counts->begin(); it != key_counts->end(); ) {
        std::map<store_key_t, int64_t>::iterator next = it;
        ++next;
        for (size_t i = 0; i < combine && next != key_counts->end(); ++i) {
            it->second += next->second;
            key_counts->erase(next++);
        }
        it = next;
    }
}

// TODO: get rid of this extra response_t copy on the stack
struct read_unshard_visitor_t : public boost::static_visitor<read_response_t> {
    const read_response_t *bits;
    const size_t count;

    read_unshard_visitor_t(const read_response_t *b, size_t c) : bits(b), count(c) { }
    read_response_t operator()(UNUSED get_query_t get) {
        guarantee(count == 1);
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

    read_response_t operator()(distribution_get_query_t dget) {
        // TODO: do this without copying so much and/or without dynamic memory
        // Sort results by region
        std::vector<distribution_result_t> results(count);
        guarantee(count > 0);

        for (size_t i = 0; i < count; ++i) {
            const distribution_result_t *result = boost::get<distribution_result_t>(&bits[i].result);
            guarantee(result, "Bad boost::get\n");
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
                for (std::map<store_key_t, int64_t>::const_iterator mit = results[i].key_counts.begin();
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

                guarantee(scale_factor >= 1.0);  // Directly provable from the code above.

                for (std::map<store_key_t, int64_t>::iterator mit = results[largest_index].key_counts.begin();
                     mit != results[largest_index].key_counts.end();
                     ++mit) {
                    mit->second = static_cast<int>(mit->second * scale_factor);
                }

                res.key_counts.insert(results[largest_index].key_counts.begin(), results[largest_index].key_counts.end());
            }
        }

        // If the result is larger than the requested limit, scale it down
        if (dget.result_limit > 0 && res.key_counts.size() > dget.result_limit) {
            scale_down_distribution(dget.result_limit, &res.key_counts);
        }

        return read_response_t(res);
    }
};

}   /* anonymous namespace */

void read_t::unshard(const read_response_t *responses, size_t count, read_response_t *response, UNUSED context_t *ctx, signal_t *) const THROWS_NOTHING {
    read_unshard_visitor_t v(responses, count);
    *response = boost::apply_visitor(v, query);
}

struct write_get_key_visitor_t : public boost::static_visitor<store_key_t> {
    /* All the types of mutation have a member called `key` */
    template<class mutation_t>
    store_key_t operator()(const mutation_t &mut) const {
        return mut.key;
    }
};

store_key_t write_get_key(const write_t &write) {
    return boost::apply_visitor(write_get_key_visitor_t(), write.mutation);
}

region_t write_t::get_region() const THROWS_NOTHING {
    return monokey_region(write_get_key(*this));
}

/* `write_t::shard()` */

bool write_t::shard(const region_t &region, write_t *write_out) const THROWS_NOTHING {
    if (region_contains_key(region, write_get_key(*this))) {
        *write_out = *this;
        return true;
    } else {
        return false;
    }
}

/* `write_response_t::unshard()` */

void write_t::unshard(const write_response_t *responses, size_t count, write_response_t *response, UNUSED context_t *ctx, signal_t *) const THROWS_NOTHING {
    /* TODO: Make sure the request type matches the response type */
    guarantee(count == 1);
    *response = responses[0];
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
    guarantee(subregion_number >= 0);
    guarantee(subregion_number < num_cpu_shards);

    // We have to be careful with the math here, to avoid overflow.
    uint64_t width = HASH_REGION_HASH_SIZE / num_cpu_shards;

    uint64_t beg = width * subregion_number;
    uint64_t end = subregion_number + 1 == num_cpu_shards ? HASH_REGION_HASH_SIZE : beg + width;

    return region_t(beg, end, key_range_t::universe());
}

store_t::store_t(serializer_t *serializer,
                 const std::string &perfmon_name,
                 int64_t cache_size,
                 bool create,
                 perfmon_collection_t *parent_perfmon_collection,
                 context_t *ctx,
                 io_backender_t *io,
                 const base_path_t &base_path) 
    : btree_store_t<memcached_protocol_t>(
            serializer, perfmon_name, cache_size, 
            create, parent_perfmon_collection, ctx, io,
            base_path) 
{ }

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
        for (std::map<store_key_t, int64_t>::iterator it = dstr.key_counts.begin(); it != dstr.key_counts.end(); ) {
            if (!dget.region.inner.contains_key(store_key_t(it->first))) {
                dstr.key_counts.erase(it++);
            } else {
                ++it;
            }
        }

        // If the result is larger than the requested limit, scale it down
        if (dget.result_limit > 0 && dstr.key_counts.size() > dget.result_limit) {
            scale_down_distribution(dget.result_limit, &dstr.key_counts);
        }

        dstr.region = dget.region;

        return read_response_t(dstr);
    }

    read_visitor_t(btree_slice_t *_btree,
                   transaction_t *_txn,
                   superblock_t *_superblock,
                   exptime_t _effective_time) :
        btree(_btree),
        txn(_txn),
        superblock(_superblock),
        effective_time(_effective_time) { }

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
                            superblock_t *superblock,
                            read_token_pair_t *token_pair,
                            UNUSED signal_t *interruptor) {
    /* Memcached doesn't have any secondary structures so right now we just
     * immediately destroy the token so that no one has to wait. */
    token_pair->sindex_read_token.reset();
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
        guarantee(proposed_cas == INVALID_CAS);
        return write_response_t(
            memcached_delete(m.key, m.dont_put_in_delete_queue, btree, effective_time, timestamp, txn, superblock));
    }

    write_visitor_t(btree_slice_t *_btree,
                    transaction_t *_txn,
                    superblock_t *_superblock,
                    cas_t _proposed_cas,
                    exptime_t _effective_time,
                    repli_timestamp_t _timestamp) :
        btree(_btree),
        txn(_txn),
        superblock(_superblock),
        proposed_cas(_proposed_cas),
        effective_time(_effective_time),
        timestamp(_timestamp) { }

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
                             scoped_ptr_t<superblock_t> *superblock,
                             write_token_pair_t *token_pair,
                             UNUSED signal_t *interruptor) {
    /* Memcached doesn't have any secondary structures so right now we just
     * immediately destroy the token so that no one has to wait. */
    token_pair->sindex_write_token.reset();

    // TODO: should this be calling to_repli_timestamp on a transition_timestamp_t?  Does this not use the timestamp-before, when we'd want the timestamp-after?
    write_visitor_t v(btree, txn, superblock->get(), write.proposed_cas, write.effective_time, timestamp.to_repli_timestamp());
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
        memcached_backfill_callback_t *callback, transaction_t *txn, superblock_t *superblock, buf_lock_t *sindex_block, memcached_protocol_t::backfill_progress_t *progress,
        signal_t *interruptor) {
    parallel_traversal_progress_t *p = new parallel_traversal_progress_t;
    scoped_ptr_t<traversal_progress_t> p_owner(p);
    progress->add_constituent(&p_owner);
    repli_timestamp_t timestamp = regions[i].second.to_repli_timestamp();
    try {
        memcached_backfill(btree, regions[i].first.inner, timestamp, callback, txn, superblock, sindex_block, p, interruptor);
    } catch (const interrupted_exc_t &) {
        /* do nothing; `protocol_send_backfill()` will notice and deal with it.
        */
    }
}

// TODO: Figure out wtf does the backfill filtering, figure out wtf constricts delete range operations to hit only a certain hash-interval, figure out what filters keys.
void store_t::protocol_send_backfill(const region_map_t<memcached_protocol_t, state_timestamp_t> &start_point,
                                     chunk_fun_callback_t<memcached_protocol_t> *chunk_fun_cb,
                                     superblock_t *superblock,
                                     buf_lock_t *sindex_block,
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
                                         btree, regions, &callback, txn, &refcount_wrapper, sindex_block, progress, interruptor));

        /* if interruptor was pulsed in `call_memcached_backfill()`, it returned
        normally anyway. So now we have to check manually. */
        if (interruptor->is_pulsed()) {
            throw interrupted_exc_t();
        }
    }
}

namespace {

struct receive_backfill_visitor_t : public boost::static_visitor<> {
    receive_backfill_visitor_t(btree_slice_t *_btree, transaction_t *_txn,
                               superblock_t *_superblock, signal_t *_interruptor)
        : btree(_btree), txn(_txn),
          superblock(_superblock), interruptor(_interruptor) { }

    void operator()(const backfill_chunk_t::delete_key_t& delete_key) const {
        memcached_delete(delete_key.key, true, btree, 0, delete_key.recency, txn, superblock);
    }
    void operator()(const backfill_chunk_t::delete_range_t& delete_range) const {
        hash_range_key_tester_t tester(delete_range.range);
        memcached_erase_range(btree, &tester, delete_range.range.inner, txn, superblock, interruptor);
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
    signal_t *interruptor;
};

}   /* anonymous namespace */

void store_t::protocol_receive_backfill(btree_slice_t *btree,
                                        transaction_t *txn,
                                        superblock_t *superblock,
                                        write_token_pair_t *token_pair,
                                        signal_t *interruptor,
                                        const backfill_chunk_t &chunk) {
    token_pair->sindex_write_token.reset();
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
                                  superblock_t *superblock,
                                  write_token_pair_t *token_pair,
                                  signal_t *interruptor) {
    token_pair->sindex_write_token.reset();

    hash_key_tester_t key_tester(subregion.beg, subregion.end);
    memcached_erase_range(btree, &key_tester, subregion.inner, txn, superblock, interruptor);
}

class generic_debug_print_visitor_t : public boost::static_visitor<void> {
public:
    explicit generic_debug_print_visitor_t(printf_buffer_t *buf) : buf_(buf) { }

    template <class T>
    void operator()(const T& x) {
        debug_print(buf_, x);
    }

private:
    printf_buffer_t *buf_;
    DISABLE_COPYING(generic_debug_print_visitor_t);
};


// Debug printing impls
void debug_print(printf_buffer_t *buf, const write_t& write) {
    buf->appendf("mcwrite{");
    generic_debug_print_visitor_t v(buf);
    boost::apply_visitor(v, write.mutation);
    if (write.proposed_cas) {
        buf->appendf(", cas=%" PRIu64, write.proposed_cas);
    }
    if (write.effective_time) {
        buf->appendf(", efftime=%" PRIu32, write.effective_time);
    }

    buf->appendf("}");
}

void debug_print(printf_buffer_t *buf, const get_cas_mutation_t& mut) {
    buf->appendf("get_cas{");
    debug_print(buf, mut.key);
    buf->appendf("}");
}

void debug_print(printf_buffer_t *buf, const sarc_mutation_t& mut) {
    buf->appendf("sarc{");
    debug_print(buf, mut.key);
    // We don't print everything in the sarc yet.
    buf->appendf(", ...}");
}

void debug_print(printf_buffer_t *buf, const delete_mutation_t& mut) {
    buf->appendf("delete{");
    debug_print(buf, mut.key);
    buf->appendf(", dpidq=%s}", mut.dont_put_in_delete_queue ? "true" : "false");
}

void debug_print(printf_buffer_t *buf, const incr_decr_mutation_t& mut) {
    buf->appendf("incr_decr{%s, %" PRIu64 ", ", mut.kind == incr_decr_INCR ? "INCR" : mut.kind == incr_decr_DECR ? "DECR" : "???", mut.amount);
    debug_print(buf, mut.key);
    buf->appendf("}");
}

void debug_print(printf_buffer_t *buf, const append_prepend_mutation_t& mut) {
    buf->appendf("append_prepend{%s, ", mut.kind == append_prepend_APPEND ? "APPEND" : mut.kind == append_prepend_PREPEND ? "PREPEND" : "???");
    debug_print(buf, mut.key);
    // We don't print the data yet.
    buf->appendf(", ...}");
}

void debug_print(printf_buffer_t *buf, const backfill_chunk_t& chunk) {
    generic_debug_print_visitor_t v(buf);
    boost::apply_visitor(v, chunk.val);
}

void debug_print(printf_buffer_t *buf, const backfill_chunk_t::delete_key_t& del) {
    buf->appendf("bf::delete_key_t{key=");
    debug_print(buf, del.key);
    buf->appendf(", recency=");
    debug_print(buf, del.recency);
    buf->appendf("}");
}

void debug_print(printf_buffer_t *buf, const backfill_chunk_t::delete_range_t& del) {
    buf->appendf("bf::delete_range_t{range=");
    debug_print(buf, del.range);
    buf->appendf("}");
}

void debug_print(printf_buffer_t *buf, const backfill_chunk_t::key_value_pair_t& kvpair) {
    buf->appendf("bf::kv{atom=");
    debug_print(buf, kvpair.backfill_atom);
    buf->appendf("}");
}
