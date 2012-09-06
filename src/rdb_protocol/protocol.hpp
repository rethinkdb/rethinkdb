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

#include "backfill_progress.hpp"
#include "btree/btree_store.hpp"
#include "btree/keys.hpp"
#include "buffer_cache/types.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/stl_types.hpp"
#include "extproc/pool.hpp"
#include "hash_region.hpp"
#include "http/json.hpp"
#include "http/json/cJSON.hpp"
#include "protocol_api.hpp"
#include "rdb_protocol/exceptions.hpp"
#include "rdb_protocol/query_language.pb.h"
#include "rdb_protocol/rdb_protocol_json.hpp"
#include "rpc/semilattice/watchable.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rdb_protocol/serializable_environment.hpp"

using query_language::scopes_t;
using query_language::backtrace_t;
using query_language::shared_scoped_less_t;
using query_language::runtime_exc_t;

class cluster_directory_metadata_t;

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

RDB_DECLARE_SERIALIZABLE(Builtin_Range);
RDB_DECLARE_SERIALIZABLE(Builtin_Filter);
RDB_DECLARE_SERIALIZABLE(Builtin_Map);
RDB_DECLARE_SERIALIZABLE(Builtin_ConcatMap);
RDB_DECLARE_SERIALIZABLE(Builtin_GroupedMapReduce);
RDB_DECLARE_SERIALIZABLE(Reduction);
RDB_DECLARE_SERIALIZABLE(WriteQuery_ForEach);

namespace rdb_protocol_details {

struct backfill_atom_t {
    store_key_t key;
    boost::shared_ptr<scoped_cJSON_t> value;
    repli_timestamp_t recency;

    backfill_atom_t() { }
    backfill_atom_t(const store_key_t& key_, const boost::shared_ptr<scoped_cJSON_t>& value_, const repli_timestamp_t& recency_)
        : key(key_), value(value_), recency(recency_)
    { }

    RDB_MAKE_ME_SERIALIZABLE_3(key, value, recency);
};

typedef boost::variant<Builtin_Filter, Builtin_Map, Builtin_ConcatMap, Builtin_Range>  transform_atom_t;

typedef std::list<transform_atom_t> transform_t;

/* There's no protocol buffer for Length (because there's not data associated
 * with it but to make things work with the variant we create a nice empty
 * class. */

struct Length {
    RDB_MAKE_ME_SERIALIZABLE_0();
};

typedef boost::variant<Builtin_GroupedMapReduce, Reduction, Length, WriteQuery_ForEach> terminal_t;


} // namespace rdb_protocol_details

class cluster_semilattice_metadata_t;

struct rdb_protocol_t {
    static const std::string protocol_name;
    typedef hash_region_t<key_range_t> region_t;

    // Construct a region containing only the specified key
    static region_t monokey_region(const store_key_t &k);

    struct context_t {
        context_t();
        context_t(extproc::pool_group_t *_pool_group,
                  namespace_repo_t<rdb_protocol_t> *_ns_repo,
                  boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > _semilattice_metadata,
                  directory_read_manager_t<cluster_directory_metadata_t> *_directory_read_manager,
                  machine_id_t _machine_id);
        ~context_t();

        extproc::pool_group_t *pool_group;
        namespace_repo_t<rdb_protocol_t> *ns_repo;

        /* These arrays contain a watchable for each thread.
         * ie cross_thread_namespace_watchables[0] is a watchable for thread 0. */
        scoped_array_t<scoped_ptr_t<cross_thread_watchable_variable_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > > > cross_thread_namespace_watchables;
        scoped_array_t<scoped_ptr_t<cross_thread_watchable_variable_t<databases_semilattice_metadata_t> > > cross_thread_database_watchables;
        boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > semilattice_metadata;
        directory_read_manager_t<cluster_directory_metadata_t> *directory_read_manager;
        cond_t interruptor; //TODO figure out where we're going to want to interrupt this from and put this there instead
        scoped_array_t<scoped_ptr_t<cross_thread_signal_t> > signals;
        machine_id_t machine_id;
    };

    struct point_read_response_t {
        boost::shared_ptr<scoped_cJSON_t> data;
        point_read_response_t() { }
        explicit point_read_response_t(boost::shared_ptr<scoped_cJSON_t> _data)
            : data(_data)
        { }

        RDB_MAKE_ME_SERIALIZABLE_1(data);
    };

    struct rget_read_response_t {
        typedef std::vector<std::pair<store_key_t, boost::shared_ptr<scoped_cJSON_t> > > stream_t; //Present if there was no terminal
        typedef std::map<boost::shared_ptr<scoped_cJSON_t>, boost::shared_ptr<scoped_cJSON_t>, shared_scoped_less_t> groups_t; //Present if the terminal was a groupedmapreduce
        typedef boost::shared_ptr<scoped_cJSON_t> atom_t; //Present if the terminal was a reduction

        struct length_t {
            int length;
            RDB_MAKE_ME_SERIALIZABLE_1(length);
        };

        struct inserted_t {
            int inserted;
            RDB_MAKE_ME_SERIALIZABLE_1(inserted);
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

        RDB_MAKE_ME_SERIALIZABLE_5(result, errors, key_range, truncated, last_considered_key);
    };

    struct distribution_read_response_t {
        //Supposing the map has keys:
        //k1, k2 ... kn
        //with k1 < k2 < .. < kn
        //Then k1 == left_key
        //and key_counts[ki] = the number of keys in [ki, ki+1) if i < n
        //key_counts[kn] = the number of keys in [kn, right_key)
        // TODO: Just make this use an int64_t.
        std::map<store_key_t, int> key_counts;

        RDB_MAKE_ME_SERIALIZABLE_1(key_counts);
    };

    struct read_response_t {
    private:
        typedef boost::variant<point_read_response_t, rget_read_response_t, distribution_read_response_t> _response_t;
    public:
        _response_t response;

        read_response_t() { }
        read_response_t(const read_response_t& r) : response(r.response) { }
        explicit read_response_t(const _response_t &r) : response(r) { }

        RDB_MAKE_ME_SERIALIZABLE_1(response);
    };

    class point_read_t {
    public:
        point_read_t() { }
        explicit point_read_t(const store_key_t& _key) : key(_key) { }

        store_key_t key;

        RDB_MAKE_ME_SERIALIZABLE_1(key);
    };

    class rget_read_t {
    public:
        rget_read_t() { }
        rget_read_t(const key_range_t &_key_range, int _maximum)
            : key_range(_key_range), maximum(_maximum) { }

        rget_read_t(const key_range_t &_key_range, int _maximum,
                    const rdb_protocol_details::transform_t &_transform,
                    const scopes_t &_scopes,
                    const backtrace_t &_backtrace)
            : key_range(_key_range), maximum(_maximum),
              transform(_transform), scopes(_scopes), backtrace(_backtrace)
        { }

        rget_read_t(const key_range_t &_key_range, int _maximum,
                    const boost::optional<rdb_protocol_details::terminal_t> &_terminal,
                    const scopes_t &_scopes,
                    const backtrace_t &_backtrace)
            : key_range(_key_range), maximum(_maximum),
              terminal(_terminal), scopes(_scopes), backtrace(_backtrace)
        { }

        rget_read_t(const key_range_t &_key_range, int _maximum,
                    const rdb_protocol_details::transform_t &_transform,
                    const boost::optional<rdb_protocol_details::terminal_t> &_terminal,
                    const scopes_t &_scopes,
                    const backtrace_t &_backtrace)
            : key_range(_key_range), maximum(_maximum),
              transform(_transform), terminal(_terminal), scopes(_scopes),
              backtrace(_backtrace)
        { }

        key_range_t key_range;
        size_t maximum;

        rdb_protocol_details::transform_t transform;
        boost::optional<rdb_protocol_details::terminal_t> terminal;
        scopes_t scopes;
        backtrace_t backtrace;

        RDB_MAKE_ME_SERIALIZABLE_6(key_range, maximum, transform, terminal, scopes, backtrace);
    };

    class distribution_read_t {
    public:
        distribution_read_t()
            : max_depth(0), range(key_range_t::universe())
        { }
        explicit distribution_read_t(int _max_depth)
            : max_depth(_max_depth), range(key_range_t::universe())
        { }

        int max_depth;
        key_range_t range;

        RDB_MAKE_ME_SERIALIZABLE_2(max_depth, range);
    };


    struct read_t {
    private:
        typedef boost::variant<point_read_t, rget_read_t, distribution_read_t> _read_t;
    public:
        _read_t read;

        region_t get_region() const THROWS_NOTHING;
        read_t shard(const region_t &region) const THROWS_NOTHING;
        void unshard(read_response_t *responses, size_t count, read_response_t *response, context_t *ctx) const THROWS_NOTHING;
        void multistore_unshard(read_response_t *responses, size_t count, read_response_t *response, context_t *ctx) const THROWS_NOTHING;

        read_t() { }
        read_t(const read_t& r) : read(r.read) { }
        explicit read_t(const _read_t &r) : read(r) { }

        RDB_MAKE_ME_SERIALIZABLE_1(read);
    };

    struct point_write_response_t {
        point_write_result_t result;

        point_write_response_t() { }
        explicit point_write_response_t(point_write_result_t _result)
            : result(_result)
        { }

        RDB_MAKE_ME_SERIALIZABLE_1(result);
    };

    struct point_delete_response_t {
        point_delete_result_t result;

        point_delete_response_t() {}
        explicit point_delete_response_t(point_delete_result_t _result)
            : result(_result)
        { }

        RDB_MAKE_ME_SERIALIZABLE_1(result);
    };

    struct write_response_t {
        boost::variant<point_write_response_t, point_delete_response_t> response;

        write_response_t() { }
        write_response_t(const write_response_t& w) : response(w.response) { }
        explicit write_response_t(const point_write_response_t& w) : response(w) { }
        explicit write_response_t(const point_delete_response_t& d) : response(d) { }

        RDB_MAKE_ME_SERIALIZABLE_1(response);
    };

    class point_write_t {
    public:
        point_write_t() { }
        point_write_t(const store_key_t& key_, boost::shared_ptr<scoped_cJSON_t> data_)
            : key(key_), data(data_) { }

        store_key_t key;

        boost::shared_ptr<scoped_cJSON_t> data;

        RDB_MAKE_ME_SERIALIZABLE_2(key, data);
    };

    class point_delete_t {
    public:
        point_delete_t() { }
        explicit point_delete_t(const store_key_t& key_)
            : key(key_) { }

        store_key_t key;

        RDB_MAKE_ME_SERIALIZABLE_1(key);
    };

    struct write_t {
        boost::variant<point_write_t, point_delete_t> write;

        region_t get_region() const THROWS_NOTHING;
        write_t shard(const region_t &region) const THROWS_NOTHING;
        void unshard(const write_response_t *responses, size_t count, write_response_t *response, context_t *cache) const THROWS_NOTHING;
        void multistore_unshard(const write_response_t *responses, size_t count, write_response_t *response, context_t *cache) const THROWS_NOTHING;

        write_t() { }
        write_t(const write_t& w) : write(w.write) { }
        explicit write_t(const point_write_t &w) : write(w) { }
        explicit write_t(const point_delete_t &d) : write(d) { }

        RDB_MAKE_ME_SERIALIZABLE_1(write);
    };

    struct backfill_chunk_t {
        struct delete_key_t {
            store_key_t key;
            repli_timestamp_t recency;

            delete_key_t() { }
            delete_key_t(const store_key_t& key_, const repli_timestamp_t& recency_) : key(key_), recency(recency_) { }

            RDB_MAKE_ME_SERIALIZABLE_1(key);
        };
        struct delete_range_t {
            region_t range;

            delete_range_t() { }
            explicit delete_range_t(const region_t& _range) : range(_range) { }

            RDB_MAKE_ME_SERIALIZABLE_1(range);
        };
        struct key_value_pair_t {
            rdb_protocol_details::backfill_atom_t backfill_atom;

            key_value_pair_t() { }
            explicit key_value_pair_t(const rdb_protocol_details::backfill_atom_t& backfill_atom_) : backfill_atom(backfill_atom_) { }

            RDB_MAKE_ME_SERIALIZABLE_1(backfill_atom);
        };

        backfill_chunk_t() { }
        explicit backfill_chunk_t(boost::variant<delete_range_t, delete_key_t, key_value_pair_t> val_) : val(val_) { }
        boost::variant<delete_range_t, delete_key_t, key_value_pair_t> val;

        static backfill_chunk_t delete_range(const region_t& range) {
            return backfill_chunk_t(delete_range_t(range));
        }
        static backfill_chunk_t delete_key(const store_key_t& key, const repli_timestamp_t& recency) {
            return backfill_chunk_t(delete_key_t(key, recency));
        }
        static backfill_chunk_t set_key(const rdb_protocol_details::backfill_atom_t& key) {
            return backfill_chunk_t(key_value_pair_t(key));
        }

        region_t get_region() const;

        rdb_protocol_t::backfill_chunk_t shard(const rdb_protocol_t::region_t &region) const THROWS_NOTHING;

        /* This is for `btree_store_t`; it's not part of the ICL protocol API. */
        repli_timestamp_t get_btree_repli_timestamp() const THROWS_NOTHING;

        RDB_MAKE_ME_SERIALIZABLE_1(val);
    };

    typedef traversal_progress_combiner_t backfill_progress_t;

    class store_t : public btree_store_t<rdb_protocol_t> {
    public:
        store_t(io_backender_t *io_backend,
                const std::string& filename,
                bool create,
                perfmon_collection_t *parent_perfmon_collection,
                context_t *ctx);
        ~store_t();

    private:
        void protocol_read(const read_t &read,
                           read_response_t *response,
                           btree_slice_t *btree,
                           transaction_t *txn,
                           superblock_t *superblock);

        void protocol_write(const write_t &write,
                            write_response_t *response,
                            transition_timestamp_t timestamp,
                            btree_slice_t *btree,
                            transaction_t *txn,
                            superblock_t *superblock);

        void protocol_send_backfill(const region_map_t<rdb_protocol_t, state_timestamp_t> &start_point,
                                    chunk_fun_callback_t<rdb_protocol_t> *chunk_fun_cb,
                                    superblock_t *superblock,
                                    btree_slice_t *btree,
                                    transaction_t *txn,
                                    backfill_progress_t *progress,
                                    signal_t *interruptor)
                                    THROWS_ONLY(interrupted_exc_t);

        void protocol_receive_backfill(btree_slice_t *btree,
                                       transaction_t *txn,
                                       superblock_t *superblock,
                                       signal_t *interruptor,
                                       const backfill_chunk_t &chunk);

        void protocol_reset_data(const region_t& subregion,
                                 btree_slice_t *btree,
                                 transaction_t *txn,
                                 superblock_t *superblock);
        context_t *ctx;
    };


    static region_t cpu_sharding_subspace(int subregion_number, int num_cpu_shards);
};

#endif  // RDB_PROTOCOL_PROTOCOL_HPP_
