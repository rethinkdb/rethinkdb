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
#include "containers/archive/stl_types.hpp"
#include "extproc/pool.hpp"
#include "http/json.hpp"
#include "http/json/cJSON.hpp"
#include "memcached/region.hpp"
#include "hash_region.hpp"
#include "protocol_api.hpp"
#include "rdb_protocol/exceptions.hpp"
#include "rdb_protocol/query_language.pb.h"
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

namespace point_modify_ns {
enum result_t { MODIFIED, INSERTED, SKIPPED, DELETED, NOP, ERROR };
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(result_t, int8_t, MODIFIED, ERROR);
enum op_t { UPDATE, MUTATE };
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(op_t, int8_t, UPDATE, MUTATE);
static inline std::string op_name(op_t op) {
    if (op == UPDATE) return "update";
    if (op == MUTATE) return "mutate";
    unreachable();
}
}

RDB_DECLARE_SERIALIZABLE(Builtin_Range);
RDB_DECLARE_SERIALIZABLE(Builtin_Filter);
RDB_DECLARE_SERIALIZABLE(Builtin_ConcatMap);
RDB_DECLARE_SERIALIZABLE(Builtin_GroupedMapReduce);
RDB_DECLARE_SERIALIZABLE(Mapping);
RDB_DECLARE_SERIALIZABLE(Reduction);
RDB_DECLARE_SERIALIZABLE(WriteQuery_ForEach);

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

typedef boost::variant<Builtin_Filter, Mapping, Builtin_ConcatMap, Builtin_Range>  transform_variant_t;

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

/* There's no protocol buffer for Length (because there's not data associated
 * with it but to make things work with the variant we create a nice empty
 * class. */

struct Length { };

RDB_DECLARE_SERIALIZABLE(Length);

typedef boost::variant<Builtin_GroupedMapReduce, Reduction, Length, WriteQuery_ForEach> terminal_variant_t;

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
        const std::set<uuid_u> &sindexes_to_bring_up_to_date,
        btree_store_t<rdb_protocol_t> *store,
        buf_lock_t *sindex_block)
    THROWS_ONLY(interrupted_exc_t);

} // namespace rdb_protocol_details


class cluster_semilattice_metadata_t;

struct rdb_protocol_t {
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


        typedef boost::variant<stream_t, groups_t, atom_t, length_t, inserted_t, runtime_exc_t> result_t;

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

    struct read_response_t {
    private:
        typedef boost::variant<point_read_response_t, rget_read_response_t, distribution_read_response_t> _response_t;
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
                    uuid_u _sindex)
            : region(region_t::universe()), sindex(_sindex),
              sindex_region(rdb_protocol_t::sindex_key_range(key)) { }

        rget_read_t(const region_t &_sindex_region,
                    uuid_u _sindex)
            : region(region_t::universe()), sindex(_sindex),
              sindex_region(_sindex_region) { }

        rget_read_t(const region_t &_sindex_region,
                    uuid_u _sindex,
                    const rdb_protocol_details::transform_t &_transform)
            : region(region_t::universe()), sindex(_sindex), 
              sindex_region(_sindex_region), transform(_transform) { }

        rget_read_t(const region_t &_region,
                    const rdb_protocol_details::transform_t &_transform)
            : region(_region), transform(_transform)
        { }

        rget_read_t(const region_t &_region,
                    const boost::optional<rdb_protocol_details::terminal_t> &_terminal)
            : region(_region), terminal(_terminal)
        { }

        rget_read_t(const region_t &_region,
                    const rdb_protocol_details::transform_t &_transform,
                    const boost::optional<rdb_protocol_details::terminal_t> &_terminal)
            : region(_region), transform(_transform),
              terminal(_terminal)
        { }

        region_t region;
        boost::optional<uuid_u> sindex;
        boost::optional<region_t> sindex_region;

        rdb_protocol_details::transform_t transform;
        boost::optional<rdb_protocol_details::terminal_t> terminal;

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


    struct read_t {
    private:
        typedef boost::variant<point_read_t, rget_read_t, distribution_read_t> _read_t;
    public:
        _read_t read;

        region_t get_region() const THROWS_NOTHING;
        read_t shard(const region_t &region) const THROWS_NOTHING;
        void unshard(read_response_t *responses, size_t count, read_response_t *response, context_t *ctx) const THROWS_NOTHING;

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

    struct point_modify_response_t {
        point_modify_ns::result_t result;
        query_language::runtime_exc_t exc;
        point_modify_response_t() { }
        explicit point_modify_response_t(point_modify_ns::result_t _result) : result(_result) {
            guarantee(result != point_modify_ns::ERROR);
        }
        explicit point_modify_response_t(const query_language::runtime_exc_t &_exc)
            : result(point_modify_ns::ERROR), exc(_exc) { }

        RDB_DECLARE_ME_SERIALIZABLE;
    };

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
        boost::variant<point_write_response_t, point_modify_response_t, point_delete_response_t,
                       sindex_create_response_t, sindex_drop_response_t> response;

        write_response_t() { }
        write_response_t(const write_response_t& w) : response(w.response) { }
        explicit write_response_t(const point_write_response_t& w) : response(w) { }
        explicit write_response_t(const point_modify_response_t& m) : response(m) { }
        explicit write_response_t(const point_delete_response_t& d) : response(d) { }

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    class point_modify_t {
    public:
        point_modify_t() { }
        point_modify_t(const std::string &_primary_key, const store_key_t &_key, const point_modify_ns::op_t &_op,
                       const query_language::scopes_t &_scopes, const backtrace_t &_backtrace, const Mapping &_mapping)
            : primary_key(_primary_key), key(_key), op(_op), scopes(_scopes), backtrace(_backtrace), mapping(_mapping) { }

        std::string primary_key;
        store_key_t key;
        point_modify_ns::op_t op;
        query_language::scopes_t scopes;
        backtrace_t backtrace;
        Mapping mapping;

        RDB_DECLARE_ME_SERIALIZABLE;
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
        sindex_create_t(uuid_u _id, const Mapping &_mapping)
            : id(_id), mapping(_mapping), region(region_t::universe())
        { }

        uuid_u id;
        Mapping mapping;
        region_t region;

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    class sindex_drop_t {
    public:
        sindex_drop_t() { }
        explicit sindex_drop_t(uuid_u _id)
            : id(_id), region(region_t::universe())
        { }

        uuid_u id;
        region_t region;

        RDB_DECLARE_ME_SERIALIZABLE;
    };

    struct write_t {
        boost::variant<point_write_t, point_delete_t, point_modify_t, sindex_create_t, sindex_drop_t> write;

        region_t get_region() const THROWS_NOTHING;
        write_t shard(const region_t &region) const THROWS_NOTHING;
        void unshard(const write_response_t *responses, size_t count, write_response_t *response, context_t *cache) const THROWS_NOTHING;

        write_t() { }
        write_t(const write_t& w) : write(w.write) { }
        explicit write_t(const point_write_t &w) : write(w) { }
        explicit write_t(const point_delete_t &d) : write(d) { }
        explicit write_t(const point_modify_t &m) : write(m) { }
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
            std::map<uuid_u, secondary_index_t> sindexes;

            sindexes_t() { }
            explicit sindexes_t(const std::map<uuid_u, secondary_index_t> &_sindexes)
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

        static backfill_chunk_t sindexes(const std::map<uuid_u, secondary_index_t> &sindexes) {
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
                io_backender_t *io);
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
