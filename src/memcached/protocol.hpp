#ifndef MEMCACHED_PROTOCOL_HPP_
#define MEMCACHED_PROTOCOL_HPP_

#include <vector>

#include "errors.hpp"
#include <boost/variant.hpp>

#include "btree/backfill.hpp"
#include "btree/parallel_traversal.hpp"  // TODO: sigh
#include "buffer_cache/mirrored/config.hpp"
#include "buffer_cache/types.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/stl_types.hpp"
#include "hash_region.hpp"
#include "memcached/queries.hpp"
#include "memcached/region.hpp"
#include "protocol_api.hpp"
#include "rpc/serialize_macros.hpp"
#include "timestamps.hpp"
#include "perfmon/types.hpp"
#include "repli_timestamp.hpp"

class io_backender_t;
class real_superblock_t;

write_message_t &operator<<(write_message_t &msg, const intrusive_ptr_t<data_buffer_t> &buf);
archive_result_t deserialize(read_stream_t *s, intrusive_ptr_t<data_buffer_t> *buf);
write_message_t &operator<<(write_message_t &msg, const rget_result_t &iter);
archive_result_t deserialize(read_stream_t *s, rget_result_t *iter);

RDB_DECLARE_SERIALIZABLE(get_query_t);
RDB_DECLARE_SERIALIZABLE(rget_query_t);
RDB_DECLARE_SERIALIZABLE(distribution_get_query_t);
RDB_DECLARE_SERIALIZABLE(get_result_t);
RDB_DECLARE_SERIALIZABLE(key_with_data_buffer_t);
RDB_DECLARE_SERIALIZABLE(rget_result_t);
RDB_DECLARE_SERIALIZABLE(distribution_result_t);
RDB_DECLARE_SERIALIZABLE(get_cas_mutation_t);
RDB_DECLARE_SERIALIZABLE(sarc_mutation_t);
RDB_DECLARE_SERIALIZABLE(delete_mutation_t);
RDB_DECLARE_SERIALIZABLE(incr_decr_mutation_t);
RDB_DECLARE_SERIALIZABLE(incr_decr_result_t);
RDB_DECLARE_SERIALIZABLE(append_prepend_mutation_t);
RDB_DECLARE_SERIALIZABLE(backfill_atom_t);

/* `memcached_protocol_t` is a container struct. It's never actually
instantiated; it just exists to pass around all the memcached-related types and
functions that the query-routing logic needs to know about. */

class memcached_protocol_t {
public:
    typedef hash_region_t<key_range_t> region_t;

    struct temporary_cache_t { };

    struct read_response_t {
        typedef boost::variant<get_result_t, rget_result_t, distribution_result_t> result_t;

        read_response_t() { }
        read_response_t(const read_response_t& r) : result(r.result) { }
        explicit read_response_t(const result_t& r) : result(r) { }

        result_t result;
        RDB_MAKE_ME_SERIALIZABLE_1(result);
    };

    struct read_t {
        typedef boost::variant<get_query_t, rget_query_t, distribution_get_query_t> query_t;

        hash_region_t<key_range_t> get_region() const THROWS_NOTHING;
        read_t shard(const hash_region_t<key_range_t> &region) const THROWS_NOTHING;
        read_response_t unshard(const std::vector<read_response_t>& responses, temporary_cache_t *cache) const THROWS_NOTHING;
        read_response_t multistore_unshard(const std::vector<read_response_t>& responses, temporary_cache_t *cache) const THROWS_NOTHING;

        read_t() { }
        read_t(const read_t& r) : query(r.query), effective_time(r.effective_time) { }
        read_t(const query_t& q, exptime_t et) : query(q), effective_time(et) { }

        query_t query;
        exptime_t effective_time;

        RDB_MAKE_ME_SERIALIZABLE_2(query, effective_time);
    };

    struct write_response_t {
        typedef boost::variant<get_result_t, set_result_t, delete_result_t, incr_decr_result_t, append_prepend_result_t> result_t;

        write_response_t() { }
        write_response_t(const write_response_t& w) : result(w.result) { }
        explicit write_response_t(const result_t& rv) : result(rv) { }

        result_t result;
        RDB_MAKE_ME_SERIALIZABLE_1(result);
    };

    struct write_t {
        typedef boost::variant<get_cas_mutation_t, sarc_mutation_t, delete_mutation_t, incr_decr_mutation_t, append_prepend_mutation_t> query_t;
        hash_region_t<key_range_t> get_region() const THROWS_NOTHING;
        write_t shard(const hash_region_t<key_range_t> &region) const THROWS_NOTHING;
        write_response_t unshard(const std::vector<write_response_t>& responses, temporary_cache_t *cache) const THROWS_NOTHING;
        write_response_t multistore_unshard(const std::vector<write_response_t>& responses, temporary_cache_t *cache) const THROWS_NOTHING;

        write_t() { }
        write_t(const write_t& w) : mutation(w.mutation), proposed_cas(w.proposed_cas), effective_time(w.effective_time) { }
        write_t(const query_t& m, cas_t pc, exptime_t et) : mutation(m), proposed_cas(pc), effective_time(et) { }

        query_t mutation;
        cas_t proposed_cas;
        exptime_t effective_time;   /* so operations are deterministic even with expiration */

        RDB_MAKE_ME_SERIALIZABLE_3(mutation, proposed_cas, effective_time);
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
            backfill_atom_t backfill_atom;

            key_value_pair_t() { }
            explicit key_value_pair_t(const backfill_atom_t& backfill_atom_) : backfill_atom(backfill_atom_) { }

            RDB_MAKE_ME_SERIALIZABLE_1(backfill_atom);
        };

        backfill_chunk_t() { }
        explicit backfill_chunk_t(boost::variant<delete_range_t, delete_key_t, key_value_pair_t> val_) : val(val_) { }

        region_t get_region() const THROWS_NOTHING;
        backfill_chunk_t shard(const region_t &r) const THROWS_NOTHING;




        boost::variant<delete_range_t, delete_key_t, key_value_pair_t> val;

        static backfill_chunk_t delete_range(const hash_region_t<key_range_t>& range) {
            return backfill_chunk_t(delete_range_t(range));
        }
        static backfill_chunk_t delete_key(const store_key_t& key, const repli_timestamp_t& recency) {
            return backfill_chunk_t(delete_key_t(key, recency));
        }
        static backfill_chunk_t set_key(const backfill_atom_t& key) {
            return backfill_chunk_t(key_value_pair_t(key));
        }

        RDB_MAKE_ME_SERIALIZABLE_1(val);
    };

    typedef traversal_progress_combiner_t backfill_progress_t;

    static region_t cpu_sharding_subspace(int subregion_number, int num_cpu_shards);

    class store_t : public store_view_t<memcached_protocol_t> {
        boost::scoped_ptr<standard_serializer_t> serializer;
        mirrored_cache_config_t cache_dynamic_config;
        boost::scoped_ptr<cache_t> cache;
        boost::scoped_ptr<btree_slice_t> btree;
        order_source_t order_source;

        fifo_enforcer_source_t token_source;
        fifo_enforcer_sink_t token_sink;

    public:
        // TODO: This was originally private.  Do we still want it to be private?
        typedef region_map_t<memcached_protocol_t, binary_blob_t> metainfo_t;

        store_t(io_backender_t *io_backender, const std::string& filename, bool create, perfmon_collection_t *collection);
        ~store_t();

        void new_read_token(boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> &token_out);
        void new_write_token(boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> &token_out);

        metainfo_t get_metainfo(order_token_t order_token,
                                boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> &token,
                                signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

        void set_metainfo(const metainfo_t &new_metainfo,
                          order_token_t order_token,
                          boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> &token,
                          signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

        memcached_protocol_t::read_response_t read(
                DEBUG_ONLY(const metainfo_checker_t<memcached_protocol_t>& metainfo_checker, )
                const memcached_protocol_t::read_t &read,
                order_token_t order_token,
                boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> &token,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t);

        memcached_protocol_t::write_response_t write(
                DEBUG_ONLY(const metainfo_checker_t<memcached_protocol_t>& metainfo_checker, )
                const metainfo_t& new_metainfo,
                const memcached_protocol_t::write_t &write,
                transition_timestamp_t timestamp,
                order_token_t order_token,
                boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> &token,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t);

        bool send_backfill(
                const region_map_t<memcached_protocol_t, state_timestamp_t> &start_point,
                const boost::function<bool(const metainfo_t&)> &should_backfill,
                const boost::function<void(memcached_protocol_t::backfill_chunk_t)> &chunk_fun,
                backfill_progress_t *progress_out,
                boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> &token,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t);

        void receive_backfill(
                const memcached_protocol_t::backfill_chunk_t &chunk,
                boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> &token,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t);

        void reset_data(
                const hash_region_t<key_range_t> &subregion,
                const metainfo_t &new_metainfo,
                boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> &token,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t);
    private:
        region_map_t<memcached_protocol_t, binary_blob_t> get_metainfo_internal(transaction_t* txn, buf_lock_t* sb_buf) const THROWS_NOTHING;

        void acquire_superblock_for_read(
                access_t access,
                bool snapshot,
                boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> &token,
                boost::scoped_ptr<transaction_t> &txn_out,
                boost::scoped_ptr<real_superblock_t> &sb_out,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t);
        void acquire_superblock_for_backfill(
                boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> &token,
                boost::scoped_ptr<transaction_t> &txn_out,
                boost::scoped_ptr<real_superblock_t> &sb_out,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t);
        void acquire_superblock_for_write(
                access_t access,
                int expected_change_count,
                boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> &token,
                boost::scoped_ptr<transaction_t> &txn_out,
                boost::scoped_ptr<real_superblock_t> &sb_out,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t);
        void check_and_update_metainfo(
            DEBUG_ONLY(const metainfo_checker_t<memcached_protocol_t>& expected_checker, )
            const metainfo_t &new_metainfo,
            transaction_t *txn,
            real_superblock_t *superbloc) const
            THROWS_NOTHING;
        metainfo_t check_metainfo(
            DEBUG_ONLY(const metainfo_checker_t<memcached_protocol_t>& metainfo_checker, )
            transaction_t *txn,
            real_superblock_t *superbloc) const
            THROWS_NOTHING;

        void update_metainfo(const metainfo_t &old_metainfo, const metainfo_t &new_metainfo, transaction_t *txn, real_superblock_t *superbloc) const THROWS_NOTHING;

        perfmon_collection_t perfmon_collection;
    };
};

void debug_print(append_only_printf_buffer_t *buf, const memcached_protocol_t::write_t& write);
void debug_print(append_only_printf_buffer_t *buf, const memcached_protocol_t::backfill_chunk_t& chunk);

#endif /* MEMCACHED_PROTOCOL_HPP_ */
