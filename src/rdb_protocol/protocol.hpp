// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_PROTOCOL_HPP_
#define RDB_PROTOCOL_PROTOCOL_HPP_

#include <algorithm>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "utils.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include <boost/optional.hpp>

#include "btree/btree_store.hpp"
#include "btree/keys.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/cond_var.hpp"
#include "containers/archive/stl_types.hpp"
#include "hash_region.hpp"
#include "http/json.hpp"
#include "http/json/cJSON.hpp"
#include "memcached/region.hpp"
#include "protocol_api.hpp"
#include "rdb_protocol/exceptions.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/rdb_protocol_json.hpp"
#include "rdb_protocol/serializable_environment.hpp"

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

namespace extproc { class pool_group_t; }

using query_language::scopes_t;
using query_language::backtrace_t;
using query_language::shared_scoped_less_t;
using query_language::runtime_exc_t;

enum point_write_result_t {
    STORED,
    DUPLICATE
};
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(point_write_result_t, int8_t, STORED, DUPLICATE);

enum point_delete_result_t {
    DELETED,
    MISSING
};
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(point_delete_result_t, int8_t, DELETED, MISSING);

RDB_DECLARE_SERIALIZABLE(Term);
RDB_DECLARE_SERIALIZABLE(Datum);

namespace rdb_protocol_details {

struct backfill_atom_t {
    store_key_t key;
    boost::shared_ptr<scoped_cJSON_t> value;
    repli_timestamp_t recency;

    backfill_atom_t() { }
    backfill_atom_t(const store_key_t& _key,
                    const boost::shared_ptr<scoped_cJSON_t>& _value,
                    const repli_timestamp_t& _recency) :
        key(_key),
        value(_value),
        recency(_recency)
    { }
};

RDB_DECLARE_SERIALIZABLE(backfill_atom_t);
typedef boost::variant<ql::map_wire_func_t,
                       ql::filter_wire_func_t,
                       ql::concatmap_wire_func_t> transform_variant_t;

struct transform_atom_t {
    transform_atom_t() { }
    transform_atom_t(const transform_variant_t &tv, const scopes_t &s, const backtrace_t &b) :
        variant(tv), scopes(s), backtrace(b) { }

    transform_variant_t variant;
    scopes_t scopes;
    backtrace_t backtrace;
};

RDB_DECLARE_SERIALIZABLE(transform_atom_t);

typedef std::list<transform_atom_t> transform_t;

typedef boost::variant<ql::gmr_wire_func_t,
                       ql::count_wire_func_t,
                       ql::reduce_wire_func_t> terminal_variant_t;

struct terminal_t {
    terminal_t() { }
    terminal_t(const terminal_variant_t &tv, const scopes_t &s, const backtrace_t &b) :
        variant(tv), scopes(s), backtrace(b) { }

    terminal_variant_t variant;
    scopes_t scopes;
    backtrace_t backtrace;
};

RDB_DECLARE_SERIALIZABLE(terminal_t);

void bring_sindexes_up_to_date(
        const std::set<std::string> &sindexes_to_bring_up_to_date,
        btree_store_t<rdb_protocol_t> *store,
        buf_lock_t *sindex_block)
    THROWS_NOTHING;

} // namespace rdb_protocol_details


class cluster_semilattice_metadata_t;

struct rdb_protocol_t {
    static const size_t MAX_PRIMARY_KEY_SIZE = 128;

    static const std::string protocol_name;
    typedef hash_region_t<key_range_t> region_t;

    // Construct a region containing only the specified key
    static region_t monokey_region(const store_key_t &k);

    // Constructs a region which will query an sindex for matches to a specific
    // key
    // TODO consider relocating this
    static key_range_t sindex_key_range(const store_key_t &k);

    struct context_t {
        context_t();
        context_t(extproc::pool_group_t *_pool_group,
                  namespace_repo_t<rdb_protocol_t> *_ns_repo,
                  boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > _semilattice_metadata,
                  directory_read_manager_t<cluster_directory_metadata_t> *_directory_read_manager,
                  uuid_u _machine_id);
        ~context_t();

        extproc::pool_group_t *pool_group;
        namespace_repo_t<rdb_protocol_t> *ns_repo;

        /* These arrays contain a watchable for each thread.
         * ie cross_thread_namespace_watchables[0] is a watchable for thread 0. */
        scoped_array_t<scoped_ptr_t<cross_thread_watchable_variable_t<cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > > > > cross_thread_namespace_watchables;
        scoped_array_t<scoped_ptr_t<cross_thread_watchable_variable_t<databases_semilattice_metadata_t> > > cross_thread_database_watchables;
        boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > semilattice_metadata;
        directory_read_manager_t<cluster_directory_metadata_t> *directory_read_manager;
        cond_t interruptor; //TODO figure out where we're going to want to interrupt this from and put this there instead
        scoped_array_t<scoped_ptr_t<cross_thread_signal_t> > signals;
        uuid_u machine_id;
    };

    struct point_read_response_t {
        boost::shared_ptr<scoped_cJSON_t> data;
        point_read_response_t() { }
        explicit point_read_response_t(boost::shared_ptr<scoped_cJSON_t> _data)
            : data(_data)
        { }

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    struct rget_read_response_t {
        typedef std::vector<std::pair<store_key_t, boost::shared_ptr<scoped_cJSON_t> > > stream_t; //Present if there was no terminal
        typedef std::map<boost::shared_ptr<scoped_cJSON_t>, boost::shared_ptr<scoped_cJSON_t>, shared_scoped_less_t> groups_t; //Present if the terminal was a groupedmapreduce
        typedef boost::shared_ptr<scoped_cJSON_t> atom_t; //Present if the terminal was a reduction

        struct length_t {
            int length;
            RDB_DECLARE_ME_SERIALIZABLE;
        };

        struct inserted_t {
            int inserted;
            RDB_DECLARE_ME_SERIALIZABLE;
        };


        typedef std::vector<boost::shared_ptr<scoped_cJSON_t> > vec_t;
        class empty_t { RDB_MAKE_ME_SERIALIZABLE_0() };
        typedef boost::variant<

            stream_t,
            groups_t,
            atom_t,
            length_t,
            inserted_t,
            runtime_exc_t,
            ql::exc_t,
            ql::wire_datum_t,
            std::vector<ql::wire_datum_t>,
            ql::wire_datum_map_t, // a map from datum_t * -> datum_t *
            std::vector<ql::wire_datum_map_t>,
            empty_t,
            vec_t

            > result_t;

        key_range_t key_range;
        result_t result;
        int errors;
        bool truncated;
        store_key_t last_considered_key;

        rget_read_response_t() { }
        rget_read_response_t(const key_range_t &_key_range, const result_t _result, int _errors, bool _truncated, const store_key_t &_last_considered_key)
            : key_range(_key_range), result(_result), errors(_errors), truncated(_truncated),
              last_considered_key(_last_considered_key)
        { }

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    struct distribution_read_response_t {
        //Supposing the map has keys:
        //k1, k2 ... kn
        //with k1 < k2 < .. < kn
        //Then k1 == left_key
        //and key_counts[ki] = the number of keys in [ki, ki+1) if i < n
        //key_counts[kn] = the number of keys in [kn, right_key)
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
    private:
        typedef boost::variant<point_read_response_t,
                               rget_read_response_t,
                               distribution_read_response_t,
                               sindex_list_response_t> _response_t;
    public:
        _response_t response;

        read_response_t() { }
        read_response_t(const read_response_t& r) : response(r.response) { }
        explicit read_response_t(const _response_t &r) : response(r) { }

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    class point_read_t {
    public:
        point_read_t() { }
        explicit point_read_t(const store_key_t& _key) : key(_key) { }

        store_key_t key;

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    class rget_read_t {
    public:
        rget_read_t() { }
        explicit rget_read_t(const region_t &_region)
            : region(_region) { }

        rget_read_t(const store_key_t &key,
                    const std::string &_sindex)
            : region(region_t::universe()), sindex(_sindex),
              sindex_region(rdb_protocol_t::sindex_key_range(key)) { }

        rget_read_t(const region_t &_sindex_region,
                    const std::string &_sindex)
            : region(region_t::universe()), sindex(_sindex),
              sindex_region(_sindex_region) { }

        rget_read_t(const region_t &_sindex_region,
                    const std::string &_sindex,
                    const rdb_protocol_details::transform_t &_transform,
                    const std::map<std::string, ql::wire_func_t> &_optargs)
            : region(region_t::universe()), sindex(_sindex),
              sindex_region(_sindex_region),
              transform(_transform), optargs(_optargs) { }

        rget_read_t(const region_t &_region,
                    const rdb_protocol_details::transform_t &_transform,
                    const std::map<std::string, ql::wire_func_t> &_optargs)
            : region(_region), transform(_transform), optargs(_optargs)
        { }

        rget_read_t(const region_t &_region,
                    const boost::optional<rdb_protocol_details::terminal_t> &_terminal,
                    const std::map<std::string, ql::wire_func_t> &_optargs)
            : region(_region), terminal(_terminal), optargs(_optargs)
        { }

        rget_read_t(const region_t &_region,
                    const rdb_protocol_details::transform_t &_transform,
                    const boost::optional<rdb_protocol_details::terminal_t> &_terminal,
                    const std::map<std::string, ql::wire_func_t> &_optargs)
            : region(_region), transform(_transform),
              terminal(_terminal), optargs(_optargs)
        { }

        /* This region is in the primary indexe's keyspace. */
        region_t region;

        /* `sindex` and `sindex_region` are both non null if the instance
        represents a sindex read (notice all sindex reads are range reads).
        And both null if the instance represents a normal rget. Notice that
        even if they are set and the instance represents a sindex read `region`
        is still used due to sharding. */

        /* The sindex from which we're reading. */
        boost::optional<std::string> sindex;

        /* The region of that sindex we're reading use `sindex_key_range` to
        read a single key. */
        boost::optional<region_t> sindex_region;

        rdb_protocol_details::transform_t transform;
        boost::optional<rdb_protocol_details::terminal_t> terminal;
        std::map<std::string, ql::wire_func_t> optargs;

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    class distribution_read_t {
    public:
        distribution_read_t()
            : max_depth(0), result_limit(0), region(region_t::universe())
        { }
        distribution_read_t(int _max_depth, size_t _result_limit)
            : max_depth(_max_depth), result_limit(_result_limit), region(region_t::universe())
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
    private:
        typedef boost::variant<point_read_t, rget_read_t, distribution_read_t, sindex_list_t> _read_t;
    public:
        _read_t read;

        region_t get_region() const THROWS_NOTHING;
        read_t shard(const region_t &region) const THROWS_NOTHING;
        void unshard(read_response_t *responses, size_t count, read_response_t *response,
                context_t *ctx, signal_t *interruptor) const
            THROWS_ONLY(interrupted_exc_t);

        read_t() { }
        read_t(const read_t& r) : read(r.read) { }
        explicit read_t(const _read_t &r) : read(r) { }

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

    typedef Datum point_replace_response_t;

    //TODO we're reusing the enums from row writes and reads to avoid name
    //shadowing. Nothing really wrong with this but maybe they could have a
    //more generic name.
    struct sindex_create_response_t { 
        RDB_DECLARE_ME_SERIALIZABLE;
    };

    struct sindex_drop_response_t {
        RDB_DECLARE_ME_SERIALIZABLE;
    };

    struct write_response_t {
        boost::variant<point_write_response_t,
                       point_delete_response_t,
                       point_replace_response_t,
                       sindex_create_response_t,
                       sindex_drop_response_t> response;

        write_response_t() { }
        write_response_t(const write_response_t& w) : response(w.response) { }
        explicit write_response_t(const point_write_response_t& w) : response(w) { }
        explicit write_response_t(const point_delete_response_t& d) : response(d) { }
        explicit write_response_t(const Datum& d) : response(d) { }

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    class point_replace_t {
    public:
        point_replace_t() { }
        point_replace_t(const std::string &_primary_key, const store_key_t &_key,
                        const ql::map_wire_func_t &_f,
                        const std::map<std::string, ql::wire_func_t> &_optargs)
            : primary_key(_primary_key), key(_key), f(_f), optargs(_optargs) { }

        std::string primary_key;
        store_key_t key;
        ql::map_wire_func_t f;
        std::map<std::string, ql::wire_func_t> optargs;

        RDB_MAKE_ME_SERIALIZABLE_4(primary_key, key, f, optargs);
    };

    class point_write_t {
    public:
        point_write_t() { }
        point_write_t(const store_key_t& _key, boost::shared_ptr<scoped_cJSON_t> _data, bool _overwrite = true)
            : key(_key), data(_data), overwrite(_overwrite) { }

        store_key_t key;
        boost::shared_ptr<scoped_cJSON_t> data;
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
        sindex_create_t(const std::string &_id, const ql::map_wire_func_t &_mapping)
            : id(_id), mapping(_mapping), region(region_t::universe())
        { }

        std::string id;
        ql::map_wire_func_t mapping;
        region_t region;

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

    struct write_t {
        boost::variant<point_write_t,
                       point_delete_t,
                       point_replace_t,
                       sindex_create_t,
                       sindex_drop_t> write;

        region_t get_region() const THROWS_NOTHING;
        write_t shard(const region_t &region) const THROWS_NOTHING;
        void unshard(const write_response_t *responses, size_t count, write_response_t *response, context_t *cache, signal_t *) const THROWS_NOTHING;

        write_t() { }
        write_t(const write_t& w) : write(w.write) { }
        explicit write_t(const point_write_t &w) : write(w) { }
        explicit write_t(const point_delete_t &d) : write(d) { }
        explicit write_t(const point_replace_t &r) : write(r) { }
        explicit write_t(const sindex_create_t &c) : write(c) { }
        explicit write_t(const sindex_drop_t &c) : write(c) { }

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

        region_t get_region() const;

        rdb_protocol_t::backfill_chunk_t shard(const rdb_protocol_t::region_t &region) const THROWS_NOTHING;

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
                            superblock_t *superblock,
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
                                 write_token_pair_t *token_pair);
        context_t *ctx;
    };


    static region_t cpu_sharding_subspace(int subregion_number, int num_cpu_shards);
};

#endif  // RDB_PROTOCOL_PROTOCOL_HPP_
