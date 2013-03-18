// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/protocol.hpp"

#include <algorithm>

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/make_shared.hpp>

#include "btree/erase_range.hpp"
#include "btree/parallel_traversal.hpp"
#include "btree/slice.hpp"
#include "btree/superblock.hpp"
#include "clustering/administration/metadata.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "concurrency/pmap.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/archive/vector_stream.hpp"
#include "protob/protob.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/env.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "rpc/semilattice/watchable.hpp"
#include "serializer/config.hpp"


typedef rdb_protocol_details::backfill_atom_t rdb_backfill_atom_t;

typedef rdb_protocol_t::context_t context_t;

typedef rdb_protocol_t::store_t store_t;
typedef rdb_protocol_t::region_t region_t;

typedef rdb_protocol_t::read_t read_t;
typedef rdb_protocol_t::read_response_t read_response_t;

typedef rdb_protocol_t::point_read_t point_read_t;
typedef rdb_protocol_t::point_read_response_t point_read_response_t;
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::point_read_response_t, data);


typedef rdb_protocol_t::rget_read_t rget_read_t;
typedef rdb_protocol_t::rget_read_response_t rget_read_response_t;

typedef rdb_protocol_t::distribution_read_t distribution_read_t;
typedef rdb_protocol_t::distribution_read_response_t distribution_read_response_t;

typedef rdb_protocol_t::write_t write_t;
typedef rdb_protocol_t::write_response_t write_response_t;

typedef rdb_protocol_t::point_replace_t point_replace_t;
typedef rdb_protocol_t::point_replace_response_t point_replace_response_t;

typedef rdb_protocol_t::batched_replaces_t batched_replaces_t;
// SAMRSI: Where's the batched_replaces_response_t?

typedef rdb_protocol_t::point_write_t point_write_t;
typedef rdb_protocol_t::point_write_response_t point_write_response_t;

typedef rdb_protocol_t::batched_writes_t batched_writes_t;
// SAMRSI: Where's the batched_writes_response_t?

typedef rdb_protocol_t::point_delete_t point_delete_t;
typedef rdb_protocol_t::point_delete_response_t point_delete_response_t;

typedef rdb_protocol_t::backfill_chunk_t backfill_chunk_t;

typedef rdb_protocol_t::backfill_progress_t backfill_progress_t;

typedef rdb_protocol_t::rget_read_response_t::stream_t stream_t;
typedef rdb_protocol_t::rget_read_response_t::groups_t groups_t;
typedef rdb_protocol_t::rget_read_response_t::atom_t atom_t;
typedef rdb_protocol_t::rget_read_response_t::length_t length_t;
typedef rdb_protocol_t::rget_read_response_t::inserted_t inserted_t;

const std::string rdb_protocol_t::protocol_name("rdb");

RDB_IMPL_PROTOB_SERIALIZABLE(Term);
RDB_IMPL_PROTOB_SERIALIZABLE(Datum);


namespace rdb_protocol_details {

RDB_IMPL_SERIALIZABLE_3(backfill_atom_t, key, value, recency);
RDB_IMPL_SERIALIZABLE_3(transform_atom_t, variant, scopes, backtrace);
RDB_IMPL_SERIALIZABLE_3(terminal_t, variant, scopes, backtrace);

}  // namespace rdb_protocol_details

rdb_protocol_t::context_t::context_t()
    : pool_group(NULL), ns_repo(NULL),
    cross_thread_namespace_watchables(get_num_threads()),
    cross_thread_database_watchables(get_num_threads()),
    directory_read_manager(NULL),
    signals(get_num_threads())
{ }

rdb_protocol_t::context_t::context_t(
    extproc::pool_group_t *_pool_group,
    namespace_repo_t<rdb_protocol_t> *_ns_repo,
    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
        _semilattice_metadata,
    directory_read_manager_t<cluster_directory_metadata_t>
        *_directory_read_manager,
    machine_id_t _machine_id)
    : pool_group(_pool_group), ns_repo(_ns_repo),
      cross_thread_namespace_watchables(get_num_threads()),
      cross_thread_database_watchables(get_num_threads()),
      semilattice_metadata(_semilattice_metadata),
      directory_read_manager(_directory_read_manager),
      signals(get_num_threads()),
      machine_id(_machine_id)
{
    for (int thread = 0; thread < get_num_threads(); ++thread) {
        cross_thread_namespace_watchables[thread].init(new cross_thread_watchable_variable_t<cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > >(
                                                    clone_ptr_t<semilattice_watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > > >
                                                        (new semilattice_watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > >(
                                                            metadata_field(&cluster_semilattice_metadata_t::rdb_namespaces, _semilattice_metadata))), thread));

        cross_thread_database_watchables[thread].init(new cross_thread_watchable_variable_t<databases_semilattice_metadata_t>(
                                                    clone_ptr_t<semilattice_watchable_t<databases_semilattice_metadata_t> >
                                                        (new semilattice_watchable_t<databases_semilattice_metadata_t>(
                                                            metadata_field(&cluster_semilattice_metadata_t::databases, _semilattice_metadata))), thread));

        signals[thread].init(new cross_thread_signal_t(&interruptor, thread));
    }
}

rdb_protocol_t::context_t::~context_t() { }

// Construct a region containing only the specified key
region_t rdb_protocol_t::monokey_region(const store_key_t &k) {
    uint64_t h = hash_region_hasher(k.contents(), k.size());
    return region_t(h, h + 1, key_range_t(key_range_t::closed, k, key_range_t::closed, k));
}

/* read_t::get_region implementation */
struct rdb_r_get_region_visitor : public boost::static_visitor<region_t> {
    region_t operator()(const point_read_t &pr) const {
        return rdb_protocol_t::monokey_region(pr.key);
    }

    region_t operator()(const rget_read_t &rg) const {
        return rg.region;
    }

    region_t operator()(const distribution_read_t &dg) const {
        return dg.region;
    }
};

region_t read_t::get_region() const THROWS_NOTHING {
    return boost::apply_visitor(rdb_r_get_region_visitor(), read);
}

struct rdb_r_shard_visitor : public boost::static_visitor<read_t> {
    explicit rdb_r_shard_visitor(const region_t &_region)
        : region(_region)
    { }

    read_t operator()(const point_read_t &pr) const {
        rassert(rdb_protocol_t::monokey_region(pr.key) == region);
        return read_t(pr);
    }

    read_t operator()(const rget_read_t &rg) const {
        rassert(region_is_superset(rg.region, region));
        rget_read_t _rg(rg);
        _rg.region = region;
        return read_t(_rg);
    }

    read_t operator()(const distribution_read_t &dg) const {
        rassert(region_is_superset(dg.region, region));
        distribution_read_t _dg(dg);
        _dg.region = region;
        return read_t(_dg);
    }

    const region_t &region;
};

read_t read_t::shard(const region_t &region) const THROWS_NOTHING {
    return boost::apply_visitor(rdb_r_shard_visitor(region), read);
}

/* read_t::unshard implementation */
bool read_response_cmp(const read_response_t &l, const read_response_t &r) {
    const rget_read_response_t *lr = boost::get<rget_read_response_t>(&l.response);
    guarantee(lr);
    const rget_read_response_t *rr = boost::get<rget_read_response_t>(&r.response);
    guarantee(rr);
    return lr->key_range < rr->key_range;
}

/* A visitor to handle this unsharding process for us. */

class distribution_read_response_less_t {
public:
    bool operator()(const distribution_read_response_t& x, const distribution_read_response_t& y) {
        if (x.region.inner == y.region.inner) {
            return x.region < y.region;
        } else {
            return x.region.inner < y.region.inner;
        }
    }
};

// Scale the distribution down by combining ranges to fit it within the limit of
// the query
void scale_down_distribution(size_t result_limit, std::map<store_key_t, int64_t> *key_counts) {
    guarantee(result_limit > 0);
    const size_t combine = (key_counts->size() / result_limit); // Combine this many other ranges into the previous range
    for (std::map<store_key_t, int64_t>::iterator it = key_counts->begin(); it != key_counts->end(); ) {
        std::map<store_key_t, int64_t>::iterator next = it;
        ++next;
        for (size_t i = 0; i < combine && next != key_counts->end(); ++i) {
            it->second += next->second;
            std::map<store_key_t, int64_t>::iterator tmp = next;
            ++next;
            key_counts->erase(tmp);
        }
        it = next;
    }
}

class rdb_r_unshard_visitor_t : public boost::static_visitor<void> {
public:
    rdb_r_unshard_visitor_t(const read_response_t *_responses,
                            size_t _count,
                            read_response_t *_response_out)
        : responses(_responses), count(_count), response_out(_response_out) { }

    void operator()(const point_read_t &) {
        guarantee(count == 1);
        guarantee(NULL != boost::get<point_read_response_t>(&responses[0].response));
        *response_out = responses[0];
    }

    void operator()(const rget_read_t &rg) {
        response_out->response = rget_read_response_t();
        rget_read_response_t *rg_response = boost::get<rget_read_response_t>(&response_out->response);
        rg_response->truncated = false;
        rg_response->key_range = read_t(rg).get_region().inner;
        rg_response->last_considered_key = read_t(rg).get_region().inner.left;

        try {
            /* First check to see if any of the responses we're unsharding threw. */
            for (size_t i = 0; i < count; ++i) {
                // TODO: we're ignoring the limit when recombining.
                const rget_read_response_t *rr = boost::get<rget_read_response_t>(&responses[i].response);
                guarantee(rr != NULL);

                if (const runtime_exc_t *e = boost::get<runtime_exc_t>(&rr->result)) {
                    throw *e;
                } else if (const ql::exc_t *e2 = boost::get<ql::exc_t>(&rr->result)) {
                    throw *e2;
                }
            }

            if (!rg.terminal) {
                //A vanilla range get
                //First we need to determine the cutoff key:
                rg_response->last_considered_key = store_key_t::max();
                for (size_t i = 0; i < count; ++i) {
                    const rget_read_response_t *rr = boost::get<rget_read_response_t>(&responses[i].response);
                    guarantee(rr != NULL);

                    if (rg_response->last_considered_key > rr->last_considered_key && rr->truncated) {
                        rg_response->last_considered_key = rr->last_considered_key;
                    }
                }

                rg_response->result = stream_t();
                stream_t *res_stream = boost::get<stream_t>(&rg_response->result);
                for (size_t i = 0; i < count; ++i) {
                    // TODO: we're ignoring the limit when recombining.
                    const rget_read_response_t *rr = boost::get<rget_read_response_t>(&responses[i].response);
                    guarantee(rr != NULL);

                    const stream_t *stream = boost::get<stream_t>(&(rr->result));

                    for (stream_t::const_iterator it = stream->begin(); it != stream->end(); ++it) {
                        if (it->first <= rg_response->last_considered_key) {
                            res_stream->push_back(*it);
                        }
                    }

                    rg_response->truncated = rg_response->truncated || rr->truncated;
                }
            } else if (boost::get<ql::reduce_wire_func_t>(&rg.terminal->variant) != NULL
                       || boost::get<ql::count_wire_func_t>(&rg.terminal->variant) != NULL) {
                typedef std::vector<ql::wire_datum_t> wire_data_t;
                rg_response->result = wire_data_t();
                wire_data_t *out_vec = boost::get<wire_data_t>(&rg_response->result);
                for (size_t i = 0; i < count; ++i) {
                    const rget_read_response_t *rr = boost::get<rget_read_response_t>(&responses[i].response);
                    guarantee(rr != NULL);
                    if (const ql::wire_datum_t *d = boost::get<ql::wire_datum_t>(&rr->result)) {
                        out_vec->push_back(*d);
                    } else {
                        guarantee(boost::get<rget_read_response_t::empty_t>(&rr->result));
                    }
                }
            } else if (boost::get<ql::gmr_wire_func_t>(&rg.terminal->variant) != NULL) {
                typedef std::vector<ql::wire_datum_map_t> wire_datum_maps_t;
                rg_response->result = wire_datum_maps_t();
                wire_datum_maps_t *out_vec = boost::get<wire_datum_maps_t>(&rg_response->result);
                for (size_t i = 0; i < count; ++i) {
                    const rget_read_response_t *rr = boost::get<rget_read_response_t>(&responses[i].response);
                    guarantee(rr != NULL);
                    const ql::wire_datum_map_t *dm = boost::get<ql::wire_datum_map_t>(&rr->result);
                    r_sanity_check(dm);
                    out_vec->push_back(*dm);
                }
            } else {
                unreachable();
            }
        } catch (const runtime_exc_t &e) {
            rg_response->result = e;
        } catch (const ql::exc_t &e) {
            rg_response->result = e;
        }
    }

    void operator()(const distribution_read_t &dg) {
        // TODO: do this without copying so much and/or without dynamic memory
        // Sort results by region
        std::vector<distribution_read_response_t> results(count);
        guarantee(count > 0);

        for (size_t i = 0; i < count; ++i) {
            const distribution_read_response_t *result = boost::get<distribution_read_response_t>(&responses[i].response);
            guarantee(result != NULL, "Bad boost::get\n");
            results[i] = *result;
        }

        std::sort(results.begin(), results.end(), distribution_read_response_less_t());

        distribution_read_response_t res;
        size_t i = 0;
        while (i < results.size()) {
            // Find the largest hash shard for this key range
            key_range_t range = results[i].region.inner;
            size_t largest_index = i;
            size_t largest_size = 0;
            size_t total_range_keys = 0;

            while (i < results.size() && results[i].region.inner == range) {
                size_t tmp_total_keys = 0;
                for (std::map<store_key_t, int64_t>::const_iterator mit = results[i].key_counts.begin(); mit != results[i].key_counts.end(); ++mit) {
                    tmp_total_keys += mit->second;
                }

                if (tmp_total_keys > largest_size) {
                    largest_size = tmp_total_keys;
                    largest_index = i;
                }

                total_range_keys += tmp_total_keys;
                ++i;
            }

            if (largest_size > 0) {
                // Scale up the selected hash shard
                double scale_factor = static_cast<double>(total_range_keys) / static_cast<double>(largest_size);

                guarantee(scale_factor >= 1.0);  // Directly provable from code above.

                for (std::map<store_key_t, int64_t>::iterator mit = results[largest_index].key_counts.begin();
                     mit != results[largest_index].key_counts.end();
                     ++mit) {
                    mit->second = static_cast<int64_t>(mit->second * scale_factor);
                }

                res.key_counts.insert(results[largest_index].key_counts.begin(), results[largest_index].key_counts.end());
            }
        }

        // If the result is larger than the requested limit, scale it down
        if (dg.result_limit > 0 && res.key_counts.size() > dg.result_limit) {
            scale_down_distribution(dg.result_limit, &res.key_counts);
        }

        response_out->response = res;
    }

private:
    const read_response_t *responses;
    size_t count;
    read_response_t *response_out;
};

void read_t::unshard(read_response_t *responses, size_t count, read_response_t *response, UNUSED context_t *ctx) const THROWS_NOTHING {
    rdb_r_unshard_visitor_t v(responses, count, response);
    boost::apply_visitor(v, read);
}

bool rget_data_cmp(const std::pair<store_key_t, boost::shared_ptr<scoped_cJSON_t> >& a,
                   const std::pair<store_key_t, boost::shared_ptr<scoped_cJSON_t> >& b) {
    return a.first < b.first;
}

/* write_t::get_region() implementation */

// Returns the smallest region that contains the set of keys.  It might be
// useful to return something other than the universal region -- often batched
// writes end up hitting the same key range.
template <class store_key_iter_t>
hash_region_t<key_range_t> smallest_covering_region(store_key_iter_t keys_begin, store_key_iter_t keys_end) {
    if (keys_begin == keys_end) {
        // We can't find the minimum or maximum of an empty sequence, so we
        // special-case the empty case.
        return hash_region_t<key_range_t>();
    }

    store_key_t minimum_key = store_key_t::min();
    store_key_t maximum_key = store_key_t::max();
    uint64_t minimum_hash_value = HASH_REGION_HASH_SIZE - 1;
    uint64_t maximum_hash_value = 0;

    while (keys_begin != keys_end) {
        if (*keys_begin < minimum_key) {
            minimum_key = *keys_begin;
        }
        if (*keys_begin > maximum_key) {
            maximum_key = *keys_begin;
        }

        const uint64_t hash_value = hash_region_hasher(keys_begin->contents(), keys_begin->size());
        if (hash_value < minimum_hash_value) {
            minimum_hash_value = hash_value;
        }
        if (hash_value > maximum_hash_value) {
            maximum_hash_value = hash_value;
        }
    }

    return hash_region_t<key_range_t>(minimum_hash_value, maximum_hash_value + 1,
                                      key_range_t(key_range_t::closed, minimum_key, key_range_t::closed, maximum_key));
}

// TODO: This entire type is suspect, given that smallest_covering_region
// would have to be slow.  Is it used in anything other than assertions?
struct rdb_w_get_region_visitor : public boost::static_visitor<region_t> {
    region_t operator()(const point_replace_t &pr) const {
        return rdb_protocol_t::monokey_region(pr.key);
    }

    static const store_key_t &point_replace_key(const std::pair<int64_t, point_replace_t> &pair) {
        return pair.second.key;
    }

    region_t operator()(const batched_replaces_t &br) const {
        return smallest_covering_region(
            boost::make_transform_iterator(br.point_replaces.begin(), point_replace_key),
            boost::make_transform_iterator(br.point_replaces.end(), point_replace_key));
    }

    region_t operator()(const point_write_t &pw) const {
        return rdb_protocol_t::monokey_region(pw.key);
    }

    static const store_key_t &point_write_key(const std::pair<int64_t, point_write_t> &pair) {
        return pair.second.key;
    }

    region_t operator()(const batched_writes_t &bw) const {
        return smallest_covering_region(
            boost::make_transform_iterator(bw.point_writes.begin(), point_write_key),
            boost::make_transform_iterator(bw.point_writes.end(), point_write_key));
    }

    region_t operator()(const point_delete_t &pd) const {
        return rdb_protocol_t::monokey_region(pd.key);
    }
};

region_t write_t::get_region() const THROWS_NOTHING {
    return boost::apply_visitor(rdb_w_get_region_visitor(), write);
}

/* write_t::shard implementation */

bool region_contains_key(const hash_region_t<key_range_t> &region, const store_key_t &key) {
    const uint64_t hash_value = hash_region_hasher(key.contents(), key.size());
    if (region.beg <= hash_value && hash_value < region.end) {
        return region.inner.contains_key(key);
    } else {
        return false;
    }
}

struct rdb_w_shard_visitor : public boost::static_visitor<write_t> {
    explicit rdb_w_shard_visitor(const region_t &_region)
        : region(_region)
    { }

    write_t operator()(const point_replace_t &pr) const {
        rassert(rdb_protocol_t::monokey_region(pr.key) == region);
        return write_t(pr);
    }

    write_t operator()(const batched_replaces_t &br) const {
        write_t ret;
        ret.write = batched_replaces_t();
        batched_replaces_t *replaces = boost::get<batched_replaces_t>(&ret.write);

        for (auto point_replace = br.point_replaces.begin(); point_replace != br.point_replaces.end(); ++point_replace) {
            if (region_contains_key(region, point_replace->second.key)) {
                replaces->point_replaces.push_back(*point_replace);
            }
        }

        return ret;
    }

    write_t operator()(const point_write_t &pw) const {
        rassert(rdb_protocol_t::monokey_region(pw.key) == region);
        return write_t(pw);
    }

    write_t operator()(const batched_writes_t &bw) const {
        // TODO: Iterating through the list of batched writes over and over again for each region is wasteful.
        //
        // We could instead have a shard function whose type looks somewhat like
        // (write_t, vector<region_t>) -> vector<pair<region_t, write_t> >.
        std::vector<std::pair<int64_t, point_write_t> > writes;
        for (auto point_write = bw.point_writes.begin(); point_write != bw.point_writes.end(); ++point_write) {
            if (region_contains_key(region, point_write->second.key)) {
                // SAMRSI: So much needless copying.
                writes.push_back(*point_write);
            }
        }

        // SAMRSI: So much needless copying.
        return write_t(batched_writes_t(writes));
    }

    write_t operator()(const point_delete_t &pd) const {
        rassert(rdb_protocol_t::monokey_region(pd.key) == region);
        return write_t(pd);
    }

    const region_t &region;
};


write_t write_t::shard(const region_t &region) const THROWS_NOTHING {
    return boost::apply_visitor(rdb_w_shard_visitor(region), write);
}

template <class T>
bool first_less(const std::pair<int64_t, T> &left, const std::pair<int64_t, T> &right) {
    return left.first < right.first;
}


void write_t::unshard(const write_response_t *responses,  size_t count, write_response_t *response, UNUSED context_t *ctx) const THROWS_NOTHING {
    guarantee(count > 0);

    // Every response but batched_replaces_t and batched_writes_t have a count
    // of 1, since they have one key.  We catch batched_replaces_t and
    // batched_writes_t in this case too, because it's not incorrect.  (If it
    // were incorrect, sharding/unsharding would be badly designed.)
    if (count == 1) {
        *response = responses[0];
    } else {
        if (NULL != boost::get<batched_replaces_response_t>(&responses[0].response)) {
            std::vector<std::pair<int64_t, point_replace_response_t> > combined;

            for (size_t i = 0; i < count; ++i) {
                const batched_replaces_response_t *batched_response = boost::get<batched_replaces_response_t>(&responses[i].response);
                guarantee(batched_response != NULL, "unsharding nonhomogeneous responses");

                combined.insert(combined.end(),
                                batched_response->point_replace_responses.begin(),
                                batched_response->point_replace_responses.end());
            }

            std::sort(combined.begin(), combined.end(), first_less<point_replace_response_t>);

            *response = write_response_t(batched_replaces_response_t(combined));

        } else if (NULL != boost::get<batched_writes_response_t>(&responses[0].response)) {

            std::vector<std::pair<int64_t, point_write_response_t> > combined;

            for (size_t i = 0; i < count; ++i) {
                const batched_writes_response_t *batched_response = boost::get<batched_writes_response_t>(&responses[i].response);
                guarantee(batched_response != NULL, "unsharding nonhomogeneous responses");

                combined.insert(combined.end(),
                                batched_response->point_write_responses.begin(),
                                batched_response->point_write_responses.end());
            }

            std::sort(combined.begin(), combined.end(), first_less<point_write_response_t>);

            *response = write_response_t(batched_writes_response_t(combined));

        } else {
            crash("response with count %zu (greater than 1) returned for non-batched write.  ('which'=%d)\n", count, responses[0].response.which());
        }
    }
}

store_t::store_t(serializer_t *serializer,
                 const std::string &perfmon_name,
                 int64_t cache_target,
                 bool create,
                 perfmon_collection_t *parent_perfmon_collection,
                 context_t *_ctx) :
    btree_store_t<rdb_protocol_t>(serializer, perfmon_name, cache_target, create, parent_perfmon_collection, _ctx),
    ctx(_ctx)
{ }

store_t::~store_t() {
    assert_thread();
}

// TODO: get rid of this extra response_t copy on the stack
struct rdb_read_visitor_t : public boost::static_visitor<void> {
    void operator()(const point_read_t &get) {
        response->response = point_read_response_t();
        point_read_response_t *res = boost::get<point_read_response_t>(&response->response);
        rdb_get(get.key, btree, txn, superblock, res);
    }

    void operator()(const rget_read_t &rget) {
        ql_env.init_optargs(rget.optargs);
        response->response = rget_read_response_t();
        rget_read_response_t *res = boost::get<rget_read_response_t>(&response->response);
        rdb_rget_slice(btree, rget.region.inner, txn, superblock, &ql_env, rget.transform, rget.terminal, res);
    }

    void operator()(const distribution_read_t &dg) {
        response->response = distribution_read_response_t();
        distribution_read_response_t *res = boost::get<distribution_read_response_t>(&response->response);
        rdb_distribution_get(btree, dg.max_depth, dg.region.inner.left, txn, superblock, res);
        for (std::map<store_key_t, int64_t>::iterator it = res->key_counts.begin(); it != res->key_counts.end(); ) {
            if (!dg.region.inner.contains_key(store_key_t(it->first))) {
                std::map<store_key_t, int64_t>::iterator tmp = it;
                ++it;
                res->key_counts.erase(tmp);
            } else {
                ++it;
            }
        }

        // If the result is larger than the requested limit, scale it down
        if (dg.result_limit > 0 && res->key_counts.size() > dg.result_limit) {
            scale_down_distribution(dg.result_limit, &res->key_counts);
        }

        res->region = dg.region;
    }

    rdb_read_visitor_t(btree_slice_t *_btree,
                       transaction_t *_txn,
                       superblock_t *_superblock,
                       rdb_protocol_t::context_t *ctx,
                       read_response_t *_response,
                       signal_t *_interruptor) :
        response(_response),
        btree(_btree),
        txn(_txn),
        superblock(_superblock),
        interruptor(_interruptor, ctx->signals[get_thread_id()].get()),
        ql_env(ctx->pool_group,
               ctx->ns_repo,
               ctx->cross_thread_namespace_watchables[get_thread_id()].get()
                   ->get_watchable(),
               ctx->cross_thread_database_watchables[get_thread_id()].get()
                   ->get_watchable(),
               ctx->semilattice_metadata,
               NULL,
               boost::make_shared<js::runner_t>(),
               &interruptor,
               ctx->machine_id,
               std::map<std::string, ql::wire_func_t>())
    { }

private:
    read_response_t *response;
    btree_slice_t *btree;
    transaction_t *txn;
    superblock_t *superblock;
    wait_any_t interruptor;
    ql::env_t ql_env;
};

void store_t::protocol_read(const read_t &read,
                            read_response_t *response,
                            btree_slice_t *btree,
                            transaction_t *txn,
                            superblock_t *superblock,
                            signal_t *interruptor) {
    rdb_read_visitor_t v(btree, txn, superblock, ctx, response, interruptor);
    boost::apply_visitor(v, read.read);
}

// TODO: get rid of this extra response_t copy on the stack
struct rdb_write_visitor_t : public boost::static_visitor<void> {
    void operator()(const point_replace_t &r) {
        ql_env.init_optargs(r.optargs);
        response->response = point_replace_response_t();
        point_replace_response_t *res = boost::get<point_replace_response_t>(&response->response);
        // TODO: modify surrounding code so we can dump this const_cast.
        ql::map_wire_func_t *f = const_cast<ql::map_wire_func_t *>(&r.f);
        rdb_replace(btree, timestamp, txn, superblock->get(),
                    r.primary_key, r.key, f, &ql_env, res);
    }

    void operator()(const batched_replaces_t &br) {
        response->response = batched_replaces_response_t();
        batched_replaces_response_t *res = boost::get<batched_replaces_response_t>(&response->response);
        rdb_batched_replace(br.point_replaces, btree, timestamp, txn, superblock,
                            &ql_env, res);
    }

    void operator()(const point_write_t &w) {
        response->response = point_write_response_t();
        point_write_response_t *res = boost::get<point_write_response_t>(&response->response);
        rdb_set(w.key, w.data, w.overwrite, btree, timestamp, txn, superblock->get(), res);
    }

    void operator()(const batched_writes_t &bw) {
        response->response = batched_writes_response_t();
        batched_writes_response_t *res = boost::get<batched_writes_response_t>(&response->response);
        rdb_batched_set(bw.point_writes, btree, timestamp, txn, superblock, res);
    }

    void operator()(const point_delete_t &d) {
        response->response = point_delete_response_t();
        point_delete_response_t *res = boost::get<point_delete_response_t>(&response->response);
        rdb_delete(d.key, btree, timestamp, txn, superblock->get(), res);
    }

    rdb_write_visitor_t(btree_slice_t *_btree,
                        transaction_t *_txn,
                        scoped_ptr_t<superblock_t> *_superblock,
                        repli_timestamp_t _timestamp,
                        rdb_protocol_t::context_t *ctx,
                        write_response_t *_response,
                        signal_t *_interruptor) :
        btree(_btree),
        txn(_txn),
        response(_response),
        superblock(_superblock),
        timestamp(_timestamp),
        interruptor(_interruptor, ctx->signals[get_thread_id()].get()),
        ql_env(ctx->pool_group,
               ctx->ns_repo,
               ctx->cross_thread_namespace_watchables[
                   get_thread_id()].get()->get_watchable(),
               ctx->cross_thread_database_watchables[
                   get_thread_id()].get()->get_watchable(),
               ctx->semilattice_metadata,
               0,
               boost::make_shared<js::runner_t>(),
               &interruptor,
               ctx->machine_id,
               std::map<std::string, ql::wire_func_t>())
    { }

private:
    btree_slice_t *btree;
    transaction_t *txn;
    write_response_t *response;
    scoped_ptr_t<superblock_t> *superblock;
    repli_timestamp_t timestamp;
    wait_any_t interruptor;
    ql::env_t ql_env;
};

void store_t::protocol_write(const write_t &write,
                             write_response_t *response,
                             transition_timestamp_t timestamp,
                             btree_slice_t *btree,
                             transaction_t *txn,
                             scoped_ptr_t<superblock_t> *superblock,
                             signal_t *interruptor) {
    rdb_write_visitor_t v(btree, txn, superblock, timestamp.to_repli_timestamp(), ctx, response, interruptor);
    boost::apply_visitor(v, write.write);
}

struct rdb_backfill_chunk_get_region_visitor_t : public boost::static_visitor<region_t> {
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
    rdb_backfill_chunk_get_region_visitor_t v;
    return boost::apply_visitor(v, val);
}

struct rdb_backfill_chunk_get_btree_repli_timestamp_visitor_t : public boost::static_visitor<repli_timestamp_t> {
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

repli_timestamp_t backfill_chunk_t::get_btree_repli_timestamp() const THROWS_NOTHING {
    rdb_backfill_chunk_get_btree_repli_timestamp_visitor_t v;
    return boost::apply_visitor(v, val);
}

struct rdb_backfill_callback_impl_t : public rdb_backfill_callback_t {
public:
    typedef backfill_chunk_t chunk_t;

    explicit rdb_backfill_callback_impl_t(chunk_fun_callback_t<rdb_protocol_t> *_chunk_fun_cb)
        : chunk_fun_cb(_chunk_fun_cb) { }
    ~rdb_backfill_callback_impl_t() { }

    void on_delete_range(const key_range_t &range, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        chunk_fun_cb->send_chunk(chunk_t::delete_range(region_t(range)), interruptor);
    }

    void on_deletion(const btree_key_t *key, repli_timestamp_t recency, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        chunk_fun_cb->send_chunk(chunk_t::delete_key(to_store_key(key), recency), interruptor);
    }

    void on_keyvalue(const rdb_backfill_atom_t& atom, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        chunk_fun_cb->send_chunk(chunk_t::set_key(atom), interruptor);
    }

protected:
    store_key_t to_store_key(const btree_key_t *key) {
        return store_key_t(key->size, key->contents);
    }

private:
    chunk_fun_callback_t<rdb_protocol_t> *chunk_fun_cb;

    DISABLE_COPYING(rdb_backfill_callback_impl_t);
};

static void call_rdb_backfill(int i, btree_slice_t *btree, const std::vector<std::pair<region_t, state_timestamp_t> > &regions,
        rdb_backfill_callback_t *callback, transaction_t *txn, superblock_t *superblock, backfill_progress_t *progress,
        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    parallel_traversal_progress_t *p = new parallel_traversal_progress_t;
    scoped_ptr_t<traversal_progress_t> p_owned(p);
    progress->add_constituent(&p_owned);
    repli_timestamp_t timestamp = regions[i].second.to_repli_timestamp();
    try {
        rdb_backfill(btree, regions[i].first.inner, timestamp, callback, txn, superblock, p, interruptor);
    } catch (interrupted_exc_t) {
        /* do nothing; `protocol_send_backfill()` will notice that interruptor
        has been pulsed */
    }
}

void store_t::protocol_send_backfill(const region_map_t<rdb_protocol_t, state_timestamp_t> &start_point,
                                     chunk_fun_callback_t<rdb_protocol_t> *chunk_fun_cb,
                                     superblock_t *superblock,
                                     btree_slice_t *btree,
                                     transaction_t *txn,
                                     backfill_progress_t *progress,
                                     signal_t *interruptor)
                                     THROWS_ONLY(interrupted_exc_t) {
    rdb_backfill_callback_impl_t callback(chunk_fun_cb);
    std::vector<std::pair<region_t, state_timestamp_t> > regions(start_point.begin(), start_point.end());
    refcount_superblock_t refcount_wrapper(superblock, regions.size());
    pmap(regions.size(), boost::bind(&call_rdb_backfill, _1,
        btree, regions, &callback, txn, &refcount_wrapper, progress, interruptor));

    /* If interruptor was pulsed, `call_rdb_backfill()` exited silently, so we
    have to check directly. */
    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
}

struct rdb_receive_backfill_visitor_t : public boost::static_visitor<void> {
    rdb_receive_backfill_visitor_t(btree_slice_t *_btree,
                                   transaction_t *_txn,
                                   superblock_t *_superblock,
                                   UNUSED signal_t *_interruptor /* See the fixme below. */)
        : btree(_btree), txn(_txn), superblock(_superblock) { }

    void operator()(const backfill_chunk_t::delete_key_t& delete_key) const {
        point_delete_response_t response;
        rdb_delete(delete_key.key, btree, delete_key.recency,
                   txn, superblock, &response);
    }

    void operator()(const backfill_chunk_t::delete_range_t& delete_range) const {
        range_key_tester_t tester(delete_range.range);
        rdb_erase_range(btree, &tester, delete_range.range.inner, txn, superblock);
    }

    void operator()(const backfill_chunk_t::key_value_pair_t& kv) const {
        const rdb_backfill_atom_t& bf_atom = kv.backfill_atom;
        point_write_response_t response;
        rdb_set(bf_atom.key, bf_atom.value, true,
                btree, bf_atom.recency,
                txn, superblock, &response);
    }

private:
    /* TODO: This might be redundant. I thought that `key_tester_t` was only
    originally necessary because in v1.1.x the hashing scheme might be different
    between the source and destination machines. */
    struct range_key_tester_t : public key_tester_t {
        explicit range_key_tester_t(const region_t& _delete_range) : delete_range(_delete_range) { }
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

    /* FIXME: interruptors are not used in btree code, so this one ignored. */
    // signal_t *interruptor;
};

void store_t::protocol_receive_backfill(btree_slice_t *btree,
                                        transaction_t *txn,
                                        superblock_t *superblock,
                                        signal_t *interruptor,
                                        const backfill_chunk_t &chunk) {
    boost::apply_visitor(rdb_receive_backfill_visitor_t(btree, txn, superblock, interruptor), chunk.val);
}

void store_t::protocol_reset_data(const region_t& subregion,
                                  btree_slice_t *btree,
                                  transaction_t *txn,
                                  superblock_t *superblock) {
    always_true_key_tester_t key_tester;
    rdb_erase_range(btree, &key_tester, subregion.inner, txn, superblock);
}

region_t rdb_protocol_t::cpu_sharding_subspace(int subregion_number,
                                               int num_cpu_shards) {
    guarantee(subregion_number >= 0);
    guarantee(subregion_number < num_cpu_shards);

    // We have to be careful with the math here, to avoid overflow.
    uint64_t width = HASH_REGION_HASH_SIZE / num_cpu_shards;

    uint64_t beg = width * subregion_number;
    uint64_t end = subregion_number + 1 == num_cpu_shards ? HASH_REGION_HASH_SIZE : beg + width;

    return region_t(beg, end, key_range_t::universe());
}

struct rdb_backfill_chunk_shard_visitor_t : public boost::static_visitor<rdb_protocol_t::backfill_chunk_t> {
public:
    explicit rdb_backfill_chunk_shard_visitor_t(const rdb_protocol_t::region_t &_region) : region(_region) { }
    rdb_protocol_t::backfill_chunk_t operator()(const rdb_protocol_t::backfill_chunk_t::delete_key_t &del) {
        rdb_protocol_t::backfill_chunk_t ret(del);
        rassert(region_is_superset(region, ret.get_region()));
        return ret;
    }
    rdb_protocol_t::backfill_chunk_t operator()(const rdb_protocol_t::backfill_chunk_t::delete_range_t &del) {
        rdb_protocol_t::region_t r = region_intersection(del.range, region);
        rassert(!region_is_empty(r));
        return rdb_protocol_t::backfill_chunk_t(rdb_protocol_t::backfill_chunk_t::delete_range_t(r));
    }
    rdb_protocol_t::backfill_chunk_t operator()(const rdb_protocol_t::backfill_chunk_t::key_value_pair_t &kv) {
        rdb_protocol_t::backfill_chunk_t ret(kv);
        rassert(region_is_superset(region, ret.get_region()));
        return ret;
    }
private:
    const rdb_protocol_t::region_t &region;

    DISABLE_COPYING(rdb_backfill_chunk_shard_visitor_t);
};

rdb_protocol_t::backfill_chunk_t rdb_protocol_t::backfill_chunk_t::shard(const rdb_protocol_t::region_t &region) const THROWS_NOTHING {
    rdb_backfill_chunk_shard_visitor_t v(region);
    return boost::apply_visitor(v, val);
}

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::rget_read_response_t::length_t, length);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::rget_read_response_t::inserted_t, inserted);


RDB_IMPL_ME_SERIALIZABLE_5(rdb_protocol_t::rget_read_response_t,
                           result, errors, key_range, truncated, last_considered_key);
RDB_IMPL_ME_SERIALIZABLE_2(rdb_protocol_t::distribution_read_response_t, region, key_counts);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::read_response_t, response);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::point_read_t, key);
RDB_IMPL_ME_SERIALIZABLE_4(rdb_protocol_t::rget_read_t,
                           region, transform, terminal, optargs);

RDB_IMPL_ME_SERIALIZABLE_3(rdb_protocol_t::distribution_read_t, max_depth, result_limit, region);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::read_t, read);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::point_write_response_t, result);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::point_delete_response_t, result);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::write_response_t, response);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::batched_writes_response_t, point_write_responses);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::batched_replaces_response_t, point_replace_responses);


RDB_IMPL_ME_SERIALIZABLE_4(rdb_protocol_t::point_replace_t, primary_key, key, f, optargs);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::batched_replaces_t, point_replaces);
RDB_IMPL_ME_SERIALIZABLE_3(rdb_protocol_t::point_write_t, key, data, overwrite);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::batched_writes_t, point_writes);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::point_delete_t, key);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::write_t, write);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::backfill_chunk_t::delete_key_t, key);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::backfill_chunk_t::delete_range_t, range);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::backfill_chunk_t::key_value_pair_t, backfill_atom);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::backfill_chunk_t, val);
