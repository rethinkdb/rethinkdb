// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_PROTOCOL_HPP_
#define RDB_PROTOCOL_PROTOCOL_HPP_

#include <algorithm>
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include <boost/optional.hpp>

#include "btree/btree_store.hpp"
#include "btree/depth_first_traversal.hpp"
#include "btree/keys.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/cond_var.hpp"
#include "hash_region.hpp"
#include "http/json.hpp"
#include "http/json/cJSON.hpp"
#include "memcached/region.hpp"
#include "protocol_api.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/profile.hpp"
#include "rdb_protocol/rdb_protocol_json.hpp"
#include "rdb_protocol/wire_func.hpp"
#include "rdb_protocol/batching.hpp"
#include "utils.hpp"

class extproc_pool_t;
class cluster_directory_metadata_t;
template <class> class cow_ptr_t;
template <class> class cross_thread_watchable_variable_t;
class cross_thread_signal_t;
class databases_semilattice_metadata_t;
template <class> class directory_read_manager_t;
template <class> class namespace_repo_t;
template <class> class namespaces_semilattice_metadata_t;
template <class> class semilattice_readwrite_view_t;
class traversal_progress_combiner_t;

using query_language::shared_scoped_less_t;

enum class profile_bool_t {
    PROFILE,
    DONT_PROFILE
};
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        profile_bool_t, int8_t,
        profile_bool_t::PROFILE, profile_bool_t::DONT_PROFILE);

enum class point_write_result_t {
    STORED,
    DUPLICATE
};
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        point_write_result_t, int8_t,
        point_write_result_t::STORED, point_write_result_t::DUPLICATE);

enum class point_delete_result_t {
    DELETED,
    MISSING
};
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        point_delete_result_t, int8_t,
        point_delete_result_t::DELETED, point_delete_result_t::MISSING);

RDB_DECLARE_SERIALIZABLE(Term);
RDB_DECLARE_SERIALIZABLE(Datum);
RDB_DECLARE_SERIALIZABLE(Backtrace);

enum class sorting_t {
    UNORDERED,
    ASCENDING,
    DESCENDING
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        sorting_t, int8_t,
        sorting_t::UNORDERED, sorting_t::DESCENDING);

namespace ql {
class datum_t;
class env_t;
class primary_readgen_t;
class readgen_t;
class sindex_readgen_t;
} // namespace ql

class datum_range_t {
public:
    datum_range_t();
    datum_range_t(
        counted_t<const ql::datum_t> left_bound,
        key_range_t::bound_t left_bound_type,
        counted_t<const ql::datum_t> right_bound,
        key_range_t::bound_t right_bound_type);
    // Range that includes just one value.
    explicit datum_range_t(counted_t<const ql::datum_t> val);
    static datum_range_t universe();

    bool contains(counted_t<const ql::datum_t> val) const;
    bool is_universe() const;
    RDB_DECLARE_ME_SERIALIZABLE;
private:
    // Only `readgen_t` and its subclasses should do anything fancy with a range.
    friend class ql::readgen_t;
    friend class ql::primary_readgen_t;
    friend class ql::sindex_readgen_t;
    key_range_t to_primary_keyrange() const;
    key_range_t to_sindex_keyrange() const;
    counted_t<const ql::datum_t> left_bound, right_bound;
    key_range_t::bound_t left_bound_type, right_bound_type;
};

struct filter_transform_t {
    filter_transform_t() { }
    filter_transform_t(const ql::wire_func_t &_filter_func,
                       const boost::optional<ql::wire_func_t> &_default_filter_val)
        : filter_func(_filter_func),
          default_filter_val(_default_filter_val) { }

    ql::wire_func_t filter_func;
    boost::optional<ql::wire_func_t> default_filter_val;
};

RDB_DECLARE_SERIALIZABLE(filter_transform_t);

namespace rdb_protocol_details {

struct backfill_atom_t {
    store_key_t key;
    counted_t<const ql::datum_t> value;
    repli_timestamp_t recency;

    backfill_atom_t() { }
    backfill_atom_t(const store_key_t &_key,
                    const counted_t<const ql::datum_t> &_value,
                    const repli_timestamp_t &_recency) :
        key(_key),
        value(_value),
        recency(_recency)
    { }
};

RDB_DECLARE_SERIALIZABLE(backfill_atom_t);

typedef boost::variant<ql::map_wire_func_t,
                       filter_transform_t,
                       ql::concatmap_wire_func_t> transform_variant_t;
typedef std::list<transform_variant_t> transform_t;

typedef boost::variant<ql::gmr_wire_func_t,
                       ql::count_wire_func_t,
                       ql::reduce_wire_func_t> terminal_variant_t;
typedef terminal_variant_t terminal_t;

void bring_sindexes_up_to_date(
        const std::set<std::string> &sindexes_to_bring_up_to_date,
        btree_store_t<rdb_protocol_t> *store,
        buf_lock_t *sindex_block,
        transaction_t *txn)
    THROWS_NOTHING;

struct rget_item_t {
    rget_item_t() { }
    rget_item_t(const store_key_t &_key, counted_t<const ql::datum_t> _data)
        : key(_key), data(_data) { }

    rget_item_t(const store_key_t &_key, counted_t<const ql::datum_t> _sindex_key,
                counted_t<const ql::datum_t> _data)
        : key(_key), sindex_key(_sindex_key), data(_data) { }

    RDB_DECLARE_ME_SERIALIZABLE;

    store_key_t key;
    boost::optional<counted_t<const ql::datum_t> > sindex_key;
    counted_t<const ql::datum_t> data;
};

} // namespace rdb_protocol_details

enum class sindex_multi_bool_t { SINGLE = 0, MULTI = 1};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(sindex_multi_bool_t, int8_t,
        sindex_multi_bool_t::SINGLE, sindex_multi_bool_t::MULTI);

class cluster_semilattice_metadata_t;
class auth_semilattice_metadata_t;

struct rdb_protocol_t {
    static const size_t MAX_PRIMARY_KEY_SIZE = 128;

    static const std::string protocol_name;
    typedef hash_region_t<key_range_t> region_t;

    // Construct a region containing only the specified key
    static region_t monokey_region(const store_key_t &k);

    // Constructs a region which will query an sindex for matches to a specific key
    // TODO consider relocating this
    static key_range_t sindex_key_range(const store_key_t &start,
                                        const store_key_t &end);

    struct context_t {
        context_t();
        context_t(extproc_pool_t *_extproc_pool,
                  namespace_repo_t<rdb_protocol_t> *_ns_repo,
                  boost::shared_ptr<semilattice_readwrite_view_t<
                      cluster_semilattice_metadata_t> > _cluster_metadata,
                  boost::shared_ptr<semilattice_readwrite_view_t<
                      auth_semilattice_metadata_t> > _auth_metadata,
                  directory_read_manager_t<
                      cluster_directory_metadata_t> *_directory_read_manager,
                  uuid_u _machine_id);
        ~context_t();

        extproc_pool_t *extproc_pool;
        namespace_repo_t<rdb_protocol_t> *ns_repo;

        /* These arrays contain a watchable for each thread.
         * ie cross_thread_namespace_watchables[0] is a watchable for thread 0. */
        scoped_array_t<scoped_ptr_t<cross_thread_watchable_variable_t<cow_ptr_t<
            namespaces_semilattice_metadata_t<rdb_protocol_t> > > > >
                cross_thread_namespace_watchables;
        scoped_array_t<scoped_ptr_t<cross_thread_watchable_variable_t<
            databases_semilattice_metadata_t> > > cross_thread_database_watchables;
        boost::shared_ptr<semilattice_readwrite_view_t<
            cluster_semilattice_metadata_t> > cluster_metadata;
        boost::shared_ptr<semilattice_readwrite_view_t<auth_semilattice_metadata_t> >
            auth_metadata;
        directory_read_manager_t<cluster_directory_metadata_t> *directory_read_manager;
        // TODO figure out where we're going to want to interrupt this from and
        // put this there instead
        cond_t interruptor;
        scoped_array_t<scoped_ptr_t<cross_thread_signal_t> > signals;
        uuid_u machine_id;
    };

    struct point_read_response_t {
        counted_t<const ql::datum_t> data;
        point_read_response_t() { }
        explicit point_read_response_t(counted_t<const ql::datum_t> _data)
            : data(_data) { }
        RDB_DECLARE_ME_SERIALIZABLE;
    };

    struct rget_read_response_t {
         // Present if there was no terminal
        typedef std::vector<rdb_protocol_details::rget_item_t> stream_t;

        class empty_t { RDB_MAKE_ME_SERIALIZABLE_0() };

        typedef boost::variant<
            // Error.
            ql::exc_t,
            ql::datum_exc_t,

            // Result of a terminal.
            counted_t<const ql::datum_t>,
            empty_t, // for `reduce`, sometimes
            ql::wire_datum_map_t, // for `gmr`, always

            // Streaming Result.
            stream_t
            > result_t;

        key_range_t key_range;
        result_t result;
        bool truncated;
        store_key_t last_considered_key;

        // Code seems to depend on a default-initialized rget_read_response_t
        // having a `stream_t` in this variant.  TODO: wtf?
        rget_read_response_t() : result(stream_t()), truncated(false) { }
        rget_read_response_t(
            const key_range_t &_key_range, const result_t _result,
            bool _truncated, const store_key_t &_last_considered_key)
            : key_range(_key_range), result(_result),
              truncated(_truncated), last_considered_key(_last_considered_key) { }

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    struct distribution_read_response_t {
        // Supposing the map has keys:
        // k1, k2 ... kn
        // with k1 < k2 < .. < kn
        // Then k1 == left_key
        // and key_counts[ki] = the number of keys in [ki, ki+1) if i < n
        // key_counts[kn] = the number of keys in [kn, right_key)
        region_t region;
        std::map<store_key_t, int64_t> key_counts;

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    struct sindex_list_response_t {
        sindex_list_response_t() { }
        std::vector<std::string> sindexes;
        RDB_DECLARE_ME_SERIALIZABLE;
    };

    struct read_response_t {
        boost::variant<point_read_response_t,
                       rget_read_response_t,
                       distribution_read_response_t,
                       sindex_list_response_t> response;
        profile::event_log_t event_log;
        size_t n_shards;

        read_response_t() { }
        explicit read_response_t(
            const boost::variant<point_read_response_t,
                                 rget_read_response_t,
                                 distribution_read_response_t> &r)
            : response(r) { }

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    class point_read_t {
    public:
        point_read_t() { }
        explicit point_read_t(const store_key_t& _key) : key(_key) { }

        store_key_t key;

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    struct sindex_rangespec_t {
        sindex_rangespec_t() { }
        sindex_rangespec_t(const std::string &_id,
                           const region_t &_region,
                           const datum_range_t _original_range)
            : id(_id), region(_region), original_range(_original_range) { }
        std::string id; // What sindex we're using.
        region_t region; // What keyspace we're currently operating on.
        datum_range_t original_range; // For dealing with truncation.
        RDB_DECLARE_ME_SERIALIZABLE;
    };

    // TODO: this should maybe be multiple types.  Determining the type of read
    // by branching on whether an optional is full sucks.
    class rget_read_t {
    public:
        rget_read_t() : batchspec(ql::batchspec_t::empty()) { }

        rget_read_t(const region_t &_region,
                    const std::map<std::string, ql::wire_func_t> &_optargs,
                    const ql::batchspec_t &_batchspec,
                    const rdb_protocol_details::transform_t &_transform,
                    boost::optional<rdb_protocol_details::terminal_t> &&_terminal,
                    boost::optional<sindex_rangespec_t> &&_sindex,
                    sorting_t _sorting)
            : region(_region),
              optargs(_optargs),
              batchspec(_batchspec),
              transform(_transform),
              terminal(std::move(_terminal)),
              sindex(std::move(_sindex)),
              sorting(_sorting) { }

        region_t region; // We need this even for sindex reads due to sharding.
        std::map<std::string, ql::wire_func_t> optargs;
        ql::batchspec_t batchspec; // used to size batches

        // We use these two for lazy maps, reductions, etc.
        rdb_protocol_details::transform_t transform;
        boost::optional<rdb_protocol_details::terminal_t> terminal;

        // This is non-empty if we're doing an sindex read.
        boost::optional<sindex_rangespec_t> sindex;

        sorting_t sorting; // Optional sorting info (UNORDERED means no sorting).

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    class distribution_read_t {
    public:
        distribution_read_t()
            : max_depth(0), result_limit(0), region(region_t::universe())
        { }
        distribution_read_t(int _max_depth, size_t _result_limit)
            : max_depth(_max_depth), result_limit(_result_limit),
              region(region_t::universe())
        { }

        int max_depth;
        size_t result_limit;
        region_t region;

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    class sindex_list_t {
    public:
        sindex_list_t() { }
        RDB_DECLARE_ME_SERIALIZABLE;
    };

    struct read_t {
        boost::variant<point_read_t,
                       rget_read_t,
                       distribution_read_t,
                       sindex_list_t> read;
        profile_bool_t profile;

        region_t get_region() const THROWS_NOTHING;
        // Returns true if the read has any operation for this region.  Returns
        // false if read_out has not been touched.
        bool shard(const region_t &region,
                   read_t *read_out) const THROWS_NOTHING;

        void unshard(read_response_t *responses, size_t count,
                     read_response_t *response, context_t *ctx,
                     signal_t *interruptor) const
            THROWS_ONLY(interrupted_exc_t);

        read_t() { }
        read_t(const boost::variant<point_read_t,
                                    rget_read_t,
                                    distribution_read_t,
                                    sindex_list_t> &r,
               profile_bool_t _profile)
            : read(r), profile(_profile) { }

        // Only use snapshotting if we're doing a range get.
        bool use_snapshot() const { return boost::get<rget_read_t>(&read); }

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    struct point_write_response_t {
        point_write_result_t result;

        point_write_response_t() { }
        explicit point_write_response_t(point_write_result_t _result)
            : result(_result)
        { }

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    struct point_delete_response_t {
        point_delete_result_t result;

        point_delete_response_t() {}
        explicit point_delete_response_t(point_delete_result_t _result)
            : result(_result)
        { }

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    // TODO we're reusing the enums from row writes and reads to avoid name
    // shadowing. Nothing really wrong with this but maybe they could have a
    // more generic name.
    struct sindex_create_response_t {
        bool success;
        RDB_DECLARE_ME_SERIALIZABLE;
    };

    struct sindex_drop_response_t {
        bool success;
        RDB_DECLARE_ME_SERIALIZABLE;
    };

    struct sync_response_t {
        // sync always succeeds
        RDB_DECLARE_ME_SERIALIZABLE;
    };

    typedef counted_t<const ql::datum_t> batched_replace_response_t;
    struct write_response_t {
        boost::variant<batched_replace_response_t,
                       // batched_replace_response_t is also for batched_insert
                       point_write_response_t,
                       point_delete_response_t,
                       sindex_create_response_t,
                       sindex_drop_response_t,
                       sync_response_t> response;

        profile::event_log_t event_log;
        size_t n_shards;

        write_response_t() { }
        template<class T>
        explicit write_response_t(const T &t) : response(t) { }

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    struct batched_replace_t {
        batched_replace_t() { }
        batched_replace_t(
            std::vector<store_key_t> &&_keys,
            const std::string &_pkey,
            const counted_t<ql::func_t> &func,
            const std::map<std::string, ql::wire_func_t > &_optargs,
            bool _return_vals)
            : keys(std::move(_keys)), pkey(_pkey), f(func), optargs(_optargs),
              return_vals(_return_vals) {
            r_sanity_check(keys.size() != 0);
            r_sanity_check(keys.size() == 1 || !return_vals);
        }
        std::vector<store_key_t> keys;
        std::string pkey;
        ql::wire_func_t f;
        std::map<std::string, ql::wire_func_t > optargs;
        bool return_vals;
        RDB_DECLARE_ME_SERIALIZABLE;
    };

    struct batched_insert_t {
        batched_insert_t() { }
        batched_insert_t(
            std::vector<counted_t<const ql::datum_t> > &&_inserts,
            const std::string &_pkey, bool _upsert, bool _return_vals)
            : inserts(std::move(_inserts)), pkey(_pkey),
              upsert(_upsert), return_vals(_return_vals) {
            r_sanity_check(inserts.size() != 0);
            r_sanity_check(inserts.size() == 1 || !return_vals);
#ifndef NDEBUG
            // These checks are done above us, but in debug mode we do them
            // again.  (They're slow.)  We do them above us because the code in
            // val.cc knows enough to report the write errors correctly while
            // still doing the other writes.
            for (auto it = inserts.begin(); it != inserts.end(); ++it) {
                counted_t<const ql::datum_t> keyval = (*it)->get(pkey, ql::NOTHROW);
                r_sanity_check(keyval.has());
                try {
                    keyval->print_primary(); // ERROR CHECKING
                    continue;
                } catch (const ql::base_exc_t &e) {
                }
                r_sanity_check(false); // throws, so can't do this in exception handler
            }
#endif // NDEBUG
        }
        std::vector<counted_t<const ql::datum_t> > inserts;
        std::string pkey;
        bool upsert;
        bool return_vals;
        RDB_DECLARE_ME_SERIALIZABLE;
    };

    class point_write_t {
    public:
        point_write_t() { }
        point_write_t(const store_key_t& _key,
                      counted_t<const ql::datum_t> _data,
                      bool _overwrite = true)
            : key(_key), data(_data), overwrite(_overwrite) { }

        store_key_t key;
        counted_t<const ql::datum_t> data;
        bool overwrite;

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    class point_delete_t {
    public:
        point_delete_t() { }
        explicit point_delete_t(const store_key_t& _key)
            : key(_key) { }

        store_key_t key;

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    class sindex_create_t {
    public:
        sindex_create_t() { }
        sindex_create_t(const std::string &_id, const ql::map_wire_func_t &_mapping,
                        sindex_multi_bool_t _multi)
            : id(_id), mapping(_mapping), region(region_t::universe()), multi(_multi)
        { }

        std::string id;
        ql::map_wire_func_t mapping;
        region_t region;
        sindex_multi_bool_t multi;

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    class sindex_drop_t {
    public:
        sindex_drop_t() { }
        explicit sindex_drop_t(const std::string &_id)
            : id(_id), region(region_t::universe())
        { }

        std::string id;
        region_t region;

        RDB_DECLARE_ME_SERIALIZABLE;
    };
    
    class sync_t {
    public:
        sync_t()
            : region(region_t::universe())
        { }

        region_t region;

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    struct write_t {
        boost::variant<batched_replace_t,
                       batched_insert_t,
                       point_write_t,
                       point_delete_t,
                       sindex_create_t,
                       sindex_drop_t,
                       sync_t> write;

        durability_requirement_t durability_requirement;
        profile_bool_t profile;

        region_t get_region() const THROWS_NOTHING;
        // Returns true if the write had any side effects applicable to the
        // region, and a non-empty write was written to write_out.
        bool shard(const region_t &region,
                   write_t *write_out) const THROWS_NOTHING;
        void unshard(write_response_t *responses, size_t count,
                     write_response_t *response, context_t *cache, signal_t *)
            const THROWS_NOTHING;

        durability_requirement_t durability() const { return durability_requirement; }

        write_t() : durability_requirement(DURABILITY_REQUIREMENT_DEFAULT) { }
        write_t(const batched_replace_t &br,
                durability_requirement_t durability,
                profile_bool_t _profile)
            : write(br), durability_requirement(durability), profile(_profile) { }
        write_t(const batched_insert_t &bi,
                durability_requirement_t durability,
                profile_bool_t _profile)
            : write(bi), durability_requirement(durability), profile(_profile) { }
        write_t(const point_write_t &w,
                durability_requirement_t durability,
                profile_bool_t _profile)
            : write(w), durability_requirement(durability), profile(_profile) { }
        write_t(const point_delete_t &d,
                durability_requirement_t durability,
                profile_bool_t _profile)
            : write(d), durability_requirement(durability), profile(_profile) { }
        write_t(const sindex_create_t &c, profile_bool_t _profile)
            : write(c), durability_requirement(DURABILITY_REQUIREMENT_DEFAULT),
              profile(_profile) { }
        write_t(const sindex_drop_t &c, profile_bool_t _profile)
            : write(c), durability_requirement(DURABILITY_REQUIREMENT_DEFAULT),
              profile(_profile) { }
        write_t(const sindex_create_t &c,
                durability_requirement_t durability,
                profile_bool_t _profile)
            : write(c), durability_requirement(durability),
              profile(_profile) { }
        write_t(const sindex_drop_t &c,
                durability_requirement_t durability,
                profile_bool_t _profile)
            : write(c), durability_requirement(durability),
              profile(_profile) { }
        /*  Note that for durability != DURABILITY_REQUIREMENT_HARD, sync might
         *  not have the desired effect (of writing unsaved data to disk).
         *  However there are cases where we use sync internally (such as when
         *  splitting up batched replaces/inserts) and want it to only have an
         *  effect if DURABILITY_REQUIREMENT_DEFAULT resolves to hard
         *  durability. */
        write_t(const sync_t &c,
                durability_requirement_t durability,
                profile_bool_t _profile)
            : write(c), durability_requirement(durability), profile(_profile) { }

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    struct backfill_chunk_t {
        struct delete_key_t {
            store_key_t key;
            repli_timestamp_t recency;

            delete_key_t() { }
            delete_key_t(const store_key_t& _key, const repli_timestamp_t& _recency) : key(_key), recency(_recency) { }

            // TODO: Wtf?  recency is not being serialized.
            RDB_DECLARE_ME_SERIALIZABLE;
        };
        struct delete_range_t {
            region_t range;

            delete_range_t() { }
            explicit delete_range_t(const region_t& _range) : range(_range) { }

            RDB_DECLARE_ME_SERIALIZABLE;
        };
        struct key_value_pair_t {
            rdb_protocol_details::backfill_atom_t backfill_atom;

            key_value_pair_t() { }
            explicit key_value_pair_t(const rdb_protocol_details::backfill_atom_t& _backfill_atom) : backfill_atom(_backfill_atom) { }

            RDB_DECLARE_ME_SERIALIZABLE;
        };
        struct sindexes_t {
            std::map<std::string, secondary_index_t> sindexes;

            sindexes_t() { }
            explicit sindexes_t(const std::map<std::string, secondary_index_t> &_sindexes)
                : sindexes(_sindexes) { }

            RDB_DECLARE_ME_SERIALIZABLE;
        };

        typedef boost::variant<delete_range_t, delete_key_t, key_value_pair_t, sindexes_t> value_t;

        backfill_chunk_t() { }
        explicit backfill_chunk_t(const value_t &_val) : val(_val) { }
        value_t val;

        static backfill_chunk_t delete_range(const region_t& range) {
            return backfill_chunk_t(delete_range_t(range));
        }
        static backfill_chunk_t delete_key(const store_key_t& key, const repli_timestamp_t& recency) {
            return backfill_chunk_t(delete_key_t(key, recency));
        }
        static backfill_chunk_t set_key(const rdb_protocol_details::backfill_atom_t& key) {
            return backfill_chunk_t(key_value_pair_t(key));
        }

        static backfill_chunk_t sindexes(const std::map<std::string, secondary_index_t> &sindexes) {
            return backfill_chunk_t(sindexes_t(sindexes));
        }

        /* This is for `btree_store_t`; it's not part of the ICL protocol API. */
        repli_timestamp_t get_btree_repli_timestamp() const THROWS_NOTHING;

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    typedef traversal_progress_combiner_t backfill_progress_t;

    class store_t : public btree_store_t<rdb_protocol_t> {
    public:
        store_t(serializer_t *serializer,
                const std::string &perfmon_name,
                int64_t cache_target,
                bool create,
                perfmon_collection_t *parent_perfmon_collection,
                context_t *ctx,
                io_backender_t *io,
                const base_path_t &base_path);
        ~store_t();

    private:
        friend struct read_visitor_t;
        void protocol_read(const read_t &read,
                           read_response_t *response,
                           btree_slice_t *btree,
                           transaction_t *txn,
                           superblock_t *superblock,
                           read_token_pair_t *token_pair,
                           signal_t *interruptor);

        friend struct write_visitor_t;
        void protocol_write(const write_t &write,
                            write_response_t *response,
                            transition_timestamp_t timestamp,
                            btree_slice_t *btree,
                            transaction_t *txn,
                            scoped_ptr_t<superblock_t> *superblock,
                            write_token_pair_t *token_pair,
                            signal_t *interruptor);

        void protocol_send_backfill(const region_map_t<rdb_protocol_t, state_timestamp_t> &start_point,
                                    chunk_fun_callback_t<rdb_protocol_t> *chunk_fun_cb,
                                    superblock_t *superblock,
                                    buf_lock_t *sindex_block,
                                    btree_slice_t *btree,
                                    transaction_t *txn,
                                    backfill_progress_t *progress,
                                    signal_t *interruptor)
                                    THROWS_ONLY(interrupted_exc_t);

        void protocol_receive_backfill(btree_slice_t *btree,
                                       transaction_t *txn,
                                       superblock_t *superblock,
                                       write_token_pair_t *token_pair,
                                       signal_t *interruptor,
                                       const backfill_chunk_t &chunk);

        void protocol_reset_data(const region_t& subregion,
                                 btree_slice_t *btree,
                                 transaction_t *txn,
                                 superblock_t *superblock,
                                 write_token_pair_t *token_pair,
                                 signal_t *interruptor);
        context_t *ctx;
    };

    static region_t cpu_sharding_subspace(int subregion_number, int num_cpu_shards);
};

namespace rdb_protocol_details {
/* TODO: This might be redundant. I thought that `key_tester_t` was only
originally necessary because in v1.1.x the hashing scheme might be different
between the source and destination machines. */
struct range_key_tester_t : public key_tester_t {
    explicit range_key_tester_t(const rdb_protocol_t::region_t *_delete_range) : delete_range(_delete_range) { }
    bool key_should_be_erased(const btree_key_t *key);

    const rdb_protocol_t::region_t *delete_range;
};
} // namespace rdb_protocol_details

#endif  // RDB_PROTOCOL_PROTOCOL_HPP_

