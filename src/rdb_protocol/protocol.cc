#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "btree/erase_range.hpp"
#include "btree/slice.hpp"
#include "concurrency/pmap.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/archive/vector_stream.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/query_language.hpp"
#include "serializer/config.hpp"

typedef rdb_protocol_details::backfill_atom_t rdb_backfill_atom_t;

typedef rdb_protocol_t::store_t store_t;
typedef rdb_protocol_t::region_t region_t;

typedef rdb_protocol_t::read_t read_t;
typedef rdb_protocol_t::read_response_t read_response_t;

typedef rdb_protocol_t::point_read_t point_read_t;
typedef rdb_protocol_t::point_read_response_t point_read_response_t;

typedef rdb_protocol_t::rget_read_t rget_read_t;
typedef rdb_protocol_t::rget_read_response_t rget_read_response_t;

typedef rdb_protocol_t::write_t write_t;
typedef rdb_protocol_t::write_response_t write_response_t;

typedef rdb_protocol_t::point_write_t point_write_t;
typedef rdb_protocol_t::point_write_response_t point_write_response_t;

typedef rdb_protocol_t::point_delete_t point_delete_t;
typedef rdb_protocol_t::point_delete_response_t point_delete_response_t;

typedef rdb_protocol_t::backfill_chunk_t backfill_chunk_t;

typedef rdb_protocol_t::backfill_progress_t backfill_progress_t;

typedef rdb_protocol_t::rget_read_response_t::stream_t stream_t;

const std::string rdb_protocol_t::protocol_name("rdb");

// Construct a region containing only the specified key
region_t rdb_protocol_t::monokey_region(const store_key_t &k) {
    uint64_t h = hash_region_hasher(k.contents(), k.size());
    return region_t(h, h + 1, key_range_t(key_range_t::closed, k, key_range_t::closed, k));
}

/* read_t::get_region implementation */
struct r_get_region_visitor : public boost::static_visitor<region_t> {
    region_t operator()(const point_read_t &pr) const {
        return rdb_protocol_t::monokey_region(pr.key);
    }

    region_t operator()(const rget_read_t &rg) const {
        // TODO: Sam bets this causes problems
        return region_t(rg.key_range);
    }
};

region_t read_t::get_region() const THROWS_NOTHING {
    return boost::apply_visitor(r_get_region_visitor(), read);
}

/* read_t::shard implementation */

struct r_shard_visitor : public boost::static_visitor<read_t> {
    explicit r_shard_visitor(const region_t &_region)
        : region(_region)
    { }

    read_t operator()(const point_read_t &pr) const {
        rassert(rdb_protocol_t::monokey_region(pr.key) == region);
        return read_t(pr);
    }

    read_t operator()(const rget_read_t &rg) const {
        rassert(region_is_superset(region_t(rg.key_range), region));
        // TODO: Reevaluate this code.  Should rget_query_t really have a region_t range?
        rget_read_t _rg(rg);
        _rg.key_range = region.inner;
        return read_t(_rg);
    }
    const region_t &region;
};

read_t read_t::shard(const region_t &region) const THROWS_NOTHING {
    return boost::apply_visitor(r_shard_visitor(region), read);
}

/* read_t::unshard implementation */
bool read_response_cmp(const read_response_t &l, const read_response_t &r) {
    const rget_read_response_t *lr = boost::get<rget_read_response_t>(&l.response);
    rassert(lr);
    const rget_read_response_t *rr = boost::get<rget_read_response_t>(&r.response);
    rassert(rr);
    return lr->key_range < rr->key_range;
}

read_response_t read_t::unshard(std::vector<read_response_t> responses, temporary_cache_t *) const THROWS_NOTHING {
    const point_read_t *pr = boost::get<point_read_t>(&read);
    if (pr) {
        rassert(responses.size() == 1);
        rassert(boost::get<point_read_response_t>(&responses[0].response));
        return responses[0];
    }

    const rget_read_t *rg = boost::get<rget_read_t>(&read);
    if (rg) {
        std::sort(responses.begin(), responses.end(), read_response_cmp);
        rget_read_response_t rg_response;
        rg_response.result = stream_t();
        stream_t *res_stream = boost::get<stream_t>(&rg_response.result);
        rg_response.truncated = false;
        rg_response.key_range = get_region().inner;
        typedef std::vector<read_response_t>::iterator rri_t;
        for(rri_t i = responses.begin(); i != responses.end(); ++i) {
            // TODO: we're ignoring the limit when recombining.
            const rget_read_response_t *_rr = boost::get<rget_read_response_t>(&i->response);
            rassert(_rr);

            const stream_t *stream = boost::get<stream_t>(&(_rr->result));

            res_stream->insert(res_stream->end(), stream->begin(), stream->end());
            rg_response.truncated = rg_response.truncated || _rr->truncated;
        }
        return read_response_t(rg_response);
    }

    unreachable("Unknown read response.");
}

bool rget_data_cmp(const std::pair<store_key_t, boost::shared_ptr<scoped_cJSON_t> >& a,
                   const std::pair<store_key_t, boost::shared_ptr<scoped_cJSON_t> >& b) {
    return a.first < b.first;
}

void merge_slices_onto_result(std::vector<read_response_t>::iterator begin,
                              std::vector<read_response_t>::iterator end,
                              rget_read_response_t *response) {
    std::vector<std::pair<store_key_t, boost::shared_ptr<scoped_cJSON_t> > > merged;

    for (std::vector<read_response_t>::iterator i = begin; i != end; ++i) {
        const rget_read_response_t *delta = boost::get<rget_read_response_t>(&i->response);
        rassert(delta);
        const stream_t *stream = boost::get<stream_t>(&delta->result);
        guarantee(stream);
        merged.insert(merged.end(), stream->begin(), stream->end());
    }
    std::sort(merged.begin(), merged.end(), rget_data_cmp);
    stream_t *stream = boost::get<stream_t>(&response->result);
    guarantee(stream);
    stream->insert(stream->end(), merged.begin(), merged.end());
}

read_response_t read_t::multistore_unshard(std::vector<read_response_t> responses, UNUSED temporary_cache_t *cache) const THROWS_NOTHING {
    const point_read_t *pr = boost::get<point_read_t>(&read);
    if (pr) {
        rassert(responses.size() == 1);
        rassert(boost::get<point_read_response_t>(&responses[0].response));
        return responses[0];
    }

    const rget_read_t *rg = boost::get<rget_read_t>(&read);
    if (rg) {
        std::sort(responses.begin(), responses.end(), read_response_cmp);
        rget_read_response_t rg_response;
        rg_response.truncated = false;
        rg_response.key_range = get_region().inner;
        typedef std::vector<read_response_t>::iterator rri_t;
        for(rri_t i = responses.begin(); i != responses.end();) {
            // TODO: we're ignoring the limit when recombining.
            const rget_read_response_t *_rr = boost::get<rget_read_response_t>(&i->response);
            rassert(_rr);
            rg_response.truncated = rg_response.truncated || _rr->truncated;

            // Collect all the responses from the same shard and merge their responses into the final response
            rri_t shard_end = i;
            ++shard_end;
            while (shard_end != responses.end()) {
                const rget_read_response_t *_rrr = boost::get<rget_read_response_t>(&shard_end->response);
                rassert(_rrr);

                if (_rrr->key_range != _rr->key_range) {
                    break;
                }

                rg_response.truncated = rg_response.truncated || _rrr->truncated;
                ++shard_end;
            }

            merge_slices_onto_result(i, shard_end, &rg_response);
            i = shard_end;
        }

        stream_t *stream = boost::get<stream_t>(&rg_response.result);
        for (std::vector<std::pair<store_key_t, boost::shared_ptr<scoped_cJSON_t> > >::iterator i = stream->begin();
             i != stream->end(); ++i) {
            rassert(i->second);
        }

        return read_response_t(rg_response);
    }

    unreachable("Unknown read response.");
}


/* write_t::get_region() implementation */

struct w_get_region_visitor : public boost::static_visitor<region_t> {
    region_t operator()(const point_write_t &pw) const {
        return rdb_protocol_t::monokey_region(pw.key);
    }

    region_t operator()(const point_delete_t &pd) const {
        return rdb_protocol_t::monokey_region(pd.key);
    }
};

region_t write_t::get_region() const THROWS_NOTHING {
    return boost::apply_visitor(w_get_region_visitor(), write);
}

/* write_t::shard implementation */

struct w_shard_visitor : public boost::static_visitor<write_t> {
    explicit w_shard_visitor(const region_t &_region)
        : region(_region)
    { }

    write_t operator()(const point_write_t &pw) const {
        rassert(rdb_protocol_t::monokey_region(pw.key) == region);
        return write_t(pw);
    }
    write_t operator()(const point_delete_t &pd) const {
        rassert(rdb_protocol_t::monokey_region(pd.key) == region);
        return write_t(pd);
    }
    const region_t &region;
};

write_t write_t::shard(const region_t &region) const THROWS_NOTHING {
    return boost::apply_visitor(w_shard_visitor(region), write);
}

write_response_t write_t::unshard(std::vector<write_response_t> responses, temporary_cache_t *) const THROWS_NOTHING {
    rassert(responses.size() == 1);
    return responses[0];
}

write_response_t write_t::multistore_unshard(const std::vector<write_response_t>& responses, temporary_cache_t *cache) const THROWS_NOTHING {
    return unshard(responses, cache);
}

store_t::store_t(io_backender_t *io_backend,
                 const std::string& filename,
                 bool create,
                 perfmon_collection_t *parent_perfmon_collection) :
    btree_store_t<rdb_protocol_t>(io_backend, filename, create, parent_perfmon_collection) { }

store_t::~store_t() {
    assert_thread();
}

struct read_visitor_t : public boost::static_visitor<read_response_t> {
    read_response_t operator()(const point_read_t& get) {
        return read_response_t(rdb_get(get.key, btree, txn, superblock));
    }

    read_response_t operator()(const rget_read_t& rget) {
        return read_response_t(rdb_rget_slice(btree, rget.key_range, 1000, txn, superblock));
    }

    read_visitor_t(btree_slice_t *btree_, transaction_t *txn_, superblock_t *superblock_) :
        btree(btree_), txn(txn_), superblock(superblock_) { }

private:
    btree_slice_t *btree;
    transaction_t *txn;
    superblock_t *superblock;
};

read_response_t store_t::protocol_read(const read_t &read,
                                       btree_slice_t *btree,
                                       transaction_t *txn,
                                       superblock_t *superblock) {
    read_visitor_t v(btree, txn, superblock);
    return boost::apply_visitor(v, read.read);
}

struct write_visitor_t : public boost::static_visitor<write_response_t> {
    write_response_t operator()(const point_write_t &w) {
        return write_response_t(
            rdb_set(w.key, w.data, btree, timestamp, txn, superblock));
    }

    write_response_t operator()(const point_delete_t &d) {
        return write_response_t(
            rdb_delete(d.key, btree, timestamp, txn, superblock));
    }

    write_visitor_t(btree_slice_t *btree_, transaction_t *txn_, superblock_t *superblock_, repli_timestamp_t timestamp_) :
      btree(btree_), txn(txn_), superblock(superblock_), timestamp(timestamp_) { }

private:
    btree_slice_t *btree;
    transaction_t *txn;
    superblock_t *superblock;
    repli_timestamp_t timestamp;
};

write_response_t store_t::protocol_write(const write_t &write,
                                         transition_timestamp_t timestamp,
                                         btree_slice_t *btree,
                                         transaction_t *txn,
                                         superblock_t *superblock) {
    write_visitor_t v(btree, txn, superblock, timestamp.to_repli_timestamp());
    return boost::apply_visitor(v, write.write);
}

struct backfill_chunk_get_region_visitor_t : public boost::static_visitor<region_t> {
    region_t operator()(const backfill_chunk_t::delete_key_t &del) {
        return rdb_protocol_t::monokey_region(del.key);
    }

    region_t operator()(const backfill_chunk_t::delete_range_t &del) {
        return del.range;
    }

    region_t operator()(const backfill_chunk_t::key_value_pair_t &kv) {
        return rdb_protocol_t::monokey_region(kv.backfill_atom.key);
    }
};

region_t backfill_chunk_t::get_region() const {
    backfill_chunk_get_region_visitor_t v;
    return boost::apply_visitor(v, val);
}

struct rdb_backfill_callback_impl_t : public rdb_backfill_callback_t {
public:
    typedef backfill_chunk_t chunk_t;

    explicit rdb_backfill_callback_impl_t(const boost::function<void(rdb_protocol_t::backfill_chunk_t)> &chunk_fun_)
        : chunk_fun(chunk_fun_) { }
    ~rdb_backfill_callback_impl_t() { }

    void on_delete_range(const key_range_t &range) {
        chunk_fun(chunk_t::delete_range(region_t(range)));
    }

    void on_deletion(const btree_key_t *key, UNUSED repli_timestamp_t recency) {
        chunk_fun(chunk_t::delete_key(to_store_key(key), recency));
    }

    void on_keyvalue(const rdb_backfill_atom_t& atom) {
        chunk_fun(chunk_t::set_key(atom));
    }

protected:
    store_key_t to_store_key(const btree_key_t *key) {
        return store_key_t(key->size, key->contents);
    }

private:
    const boost::function<void(rdb_protocol_t::backfill_chunk_t)> &chunk_fun;

    DISABLE_COPYING(rdb_backfill_callback_impl_t);
};

static void call_rdb_backfill(int i, btree_slice_t *btree, const std::vector<std::pair<region_t, state_timestamp_t> > &regions,
        rdb_backfill_callback_t *callback, transaction_t *txn, superblock_t *superblock, backfill_progress_t *progress) {
    parallel_traversal_progress_t *p = new parallel_traversal_progress_t;
    scoped_ptr_t<traversal_progress_t> p_owned(p);
    progress->add_constituent(&p_owned);
    repli_timestamp_t timestamp = regions[i].second.to_repli_timestamp();
    rdb_backfill(btree, regions[i].first.inner, timestamp, callback, txn, superblock, p);
}

void store_t::protocol_send_backfill(const region_map_t<rdb_protocol_t, state_timestamp_t> &start_point,
                                     const boost::function<void(backfill_chunk_t)> &chunk_fun,
                                     superblock_t *superblock,
                                     btree_slice_t *btree,
                                     transaction_t *txn,
                                     backfill_progress_t *progress) {
    rdb_backfill_callback_impl_t callback(chunk_fun);
    std::vector<std::pair<region_t, state_timestamp_t> > regions(start_point.begin(), start_point.end());
    refcount_superblock_t refcount_wrapper(superblock, regions.size());
    pmap(regions.size(), boost::bind(&call_rdb_backfill, _1,
        btree, regions, &callback, txn, &refcount_wrapper, progress));
}

struct receive_backfill_visitor_t : public boost::static_visitor<> {
    receive_backfill_visitor_t(btree_slice_t *btree_, transaction_t *txn_, superblock_t *superblock_, signal_t *interruptor_) :
      btree(btree_), txn(txn_), superblock(superblock_), interruptor(interruptor_) { }

    void operator()(const backfill_chunk_t::delete_key_t& delete_key) const {
        // FIXME: we ignored delete_key.recency here. Should we use it in place of repli_timestamp_t::invalid?
        rdb_delete(delete_key.key, btree, repli_timestamp_t::invalid, txn, superblock);
    }

    void operator()(const backfill_chunk_t::delete_range_t& delete_range) const {
        range_key_tester_t tester(delete_range.range);
        rdb_erase_range(btree, &tester, delete_range.range.inner, txn, superblock);
    }

    void operator()(const backfill_chunk_t::key_value_pair_t& kv) const {
        const rdb_backfill_atom_t& bf_atom = kv.backfill_atom;
        rdb_set(bf_atom.key, bf_atom.value,
                btree, repli_timestamp_t::invalid,
                txn, superblock);
    }

private:
    /* TODO: This might be redundant. I thought that `key_tester_t` was only
    originally necessary because in v1.1.x the hashing scheme might be different
    between the source and destination machines. */
    struct range_key_tester_t : public key_tester_t {
        explicit range_key_tester_t(const region_t& delete_range_) : delete_range(delete_range_) { }
        bool key_should_be_erased(const btree_key_t *key) {
            uint64_t h = hash_region_hasher(key->contents, key->size);
            return delete_range.beg <= h && h < delete_range.end
                && delete_range.inner.contains_key(key->contents, key->size);
        }

        const region_t& delete_range;
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

void store_t::protocol_reset_data(const region_t& subregion,
                                  btree_slice_t *btree,
                                  transaction_t *txn,
                                  superblock_t *superblock) {
    always_true_key_tester_t key_tester;
    rdb_erase_range(btree, &key_tester, subregion.inner, txn, superblock);
}

region_t rdb_protocol_t::cpu_sharding_subspace(int subregion_number, UNUSED int num_cpu_shards) {
    rassert(subregion_number >= 0);
    rassert(subregion_number < num_cpu_shards);

    // We have to be careful with the math here, to avoid overflow.
    uint64_t width = HASH_REGION_HASH_SIZE / num_cpu_shards;

    uint64_t beg = width * subregion_number;
    uint64_t end = subregion_number + 1 == num_cpu_shards ? HASH_REGION_HASH_SIZE : beg + width;

    return region_t(beg, end, key_range_t::universe());
}
