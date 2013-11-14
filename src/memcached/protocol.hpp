// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef MEMCACHED_PROTOCOL_HPP_
#define MEMCACHED_PROTOCOL_HPP_

#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/variant.hpp>

#include "backfill_progress.hpp"
#include "btree/btree_store.hpp"
#include "buffer_cache/types.hpp"
#include "containers/archive/stl_types.hpp"
#include "memcached/memcached_btree/backfill.hpp"
#include "memcached/queries.hpp"
#include "memcached/region.hpp"
#include "hash_region.hpp"
#include "protocol_api.hpp"
#include "rpc/serialize_macros.hpp"
#include "timestamps.hpp"
#include "perfmon/types.hpp"
#include "repli_timestamp.hpp"

class io_backender_t;
class real_superblock_t;
class traversal_progress_combiner_t;

write_message_t &operator<<(write_message_t &msg, const counted_t<data_buffer_t> &buf);
archive_result_t deserialize(read_stream_t *s, counted_t<data_buffer_t> *buf);
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
    static const std::string protocol_name;
    typedef hash_region_t<key_range_t> region_t;

    struct context_t { };

    struct read_response_t {
        typedef boost::variant<get_result_t, rget_result_t, distribution_result_t> result_t;

        read_response_t() { }
        read_response_t(const read_response_t &r) : result(r.result) { }
        explicit read_response_t(const result_t &r) : result(r) { }

        result_t result;
    };

    struct read_t {
        typedef boost::variant<get_query_t,
                               rget_query_t,
                               distribution_get_query_t> query_t;

        region_t get_region() const THROWS_NOTHING;
        // Returns true if the read had any applicability to the region, and a
        // non-empty read was written to read_out.
        bool shard(const region_t &regions,
                   read_t *read_out) const THROWS_NOTHING;
        void unshard(const read_response_t *responses,
                     size_t count,
                     read_response_t *response,
                     context_t *ctx,
                     signal_t *) const THROWS_NOTHING;

        read_t() { }
        read_t(const read_t& r) : query(r.query), effective_time(r.effective_time) { }
        read_t(const query_t& q, exptime_t et) : query(q), effective_time(et) { }

        bool use_snapshot() const { return false; }

        query_t query;
        exptime_t effective_time;
    };

    struct write_response_t {
        typedef boost::variant<get_result_t, set_result_t, delete_result_t, incr_decr_result_t, append_prepend_result_t> result_t;

        write_response_t() { }
        write_response_t(const write_response_t& w) : result(w.result) { }
        explicit write_response_t(const result_t& rv) : result(rv) { }

        result_t result;
    };

    struct write_t {
        typedef boost::variant<get_cas_mutation_t, sarc_mutation_t, delete_mutation_t, incr_decr_mutation_t, append_prepend_mutation_t> query_t;

        durability_requirement_t durability() const { return DURABILITY_REQUIREMENT_DEFAULT; }

        region_t get_region() const THROWS_NOTHING;

        // Returns true if the write had any applicability to the region, and a non-empty
        // write was written to write_out.
        bool shard(const region_t &region,
                   write_t *write_out) const THROWS_NOTHING;
        void unshard(const write_response_t *responses, size_t count, write_response_t *response, context_t *ctx, signal_t *) const THROWS_NOTHING;

        write_t() { }
        write_t(const write_t& w) : mutation(w.mutation), proposed_cas(w.proposed_cas), effective_time(w.effective_time) { }
        write_t(const query_t& m, cas_t pc, exptime_t et) : mutation(m), proposed_cas(pc), effective_time(et) { }

        query_t mutation;
        cas_t proposed_cas;
        exptime_t effective_time;   /* so operations are deterministic even with expiration */
    };

    struct backfill_chunk_t {
        struct delete_key_t {
            store_key_t key;
            repli_timestamp_t recency;

            delete_key_t() { }
            delete_key_t(const store_key_t& _key, const repli_timestamp_t& _recency) : key(_key), recency(_recency) { }
        };
        struct delete_range_t {
            region_t range;

            delete_range_t() { }
            explicit delete_range_t(const region_t& _range) : range(_range) { }
        };
        struct key_value_pair_t {
            backfill_atom_t backfill_atom;

            key_value_pair_t() { }
            explicit key_value_pair_t(const backfill_atom_t& _backfill_atom) : backfill_atom(_backfill_atom) { }
        };

        backfill_chunk_t() { }
        explicit backfill_chunk_t(boost::variant<delete_range_t, delete_key_t, key_value_pair_t> _val) : val(_val) { }

        /* This is called by `btree_store_t`; it's not part of the ICL protocol
        API. */
        repli_timestamp_t get_btree_repli_timestamp() const THROWS_NOTHING;

        boost::variant<delete_range_t, delete_key_t, key_value_pair_t> val;

        static backfill_chunk_t delete_range(const region_t &range) {
            return backfill_chunk_t(delete_range_t(range));
        }
        static backfill_chunk_t delete_key(const store_key_t& key, const repli_timestamp_t& recency) {
            return backfill_chunk_t(delete_key_t(key, recency));
        }
        static backfill_chunk_t set_key(const backfill_atom_t& key) {
            return backfill_chunk_t(key_value_pair_t(key));
        }
    };

    typedef traversal_progress_combiner_t backfill_progress_t;

    static region_t cpu_sharding_subspace(int subregion_number, int num_cpu_shards);

    class store_t : public btree_store_t<memcached_protocol_t> {
    public:
        store_t(serializer_t *serializer,
                const std::string &perfmon_name,
                int64_t cache_quota,
                bool create,
                perfmon_collection_t *collection,
                context_t *,
                io_backender_t *io,
                const base_path_t &base_path);
        virtual ~store_t();

    private:
        void protocol_read(const read_t &read,
                           read_response_t *response,
                           btree_slice_t *btree,
                           transaction_t *txn,
                           superblock_t *superblock,
                           read_token_pair_t *token,
                           signal_t *interruptor);

        void protocol_write(const write_t &write,
                            write_response_t *response,
                            transition_timestamp_t timestamp,
                            btree_slice_t *btree,
                            transaction_t *txn,
                            scoped_ptr_t<superblock_t> *superblock,
                            write_token_pair_t *token,
                            signal_t *interruptor);

        void protocol_send_backfill(const region_map_t<memcached_protocol_t, state_timestamp_t> &start_point,
                                    chunk_fun_callback_t<memcached_protocol_t> *chunk_fun_cb,
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
    };

};

RDB_DECLARE_SERIALIZABLE(memcached_protocol_t::read_response_t);
RDB_DECLARE_SERIALIZABLE(memcached_protocol_t::read_t);
RDB_DECLARE_SERIALIZABLE(memcached_protocol_t::write_response_t);
RDB_DECLARE_SERIALIZABLE(memcached_protocol_t::write_t);
RDB_DECLARE_SERIALIZABLE(memcached_protocol_t::backfill_chunk_t::delete_key_t);
RDB_DECLARE_SERIALIZABLE(memcached_protocol_t::backfill_chunk_t::delete_range_t);
RDB_DECLARE_SERIALIZABLE(memcached_protocol_t::backfill_chunk_t::key_value_pair_t);
RDB_DECLARE_SERIALIZABLE(memcached_protocol_t::backfill_chunk_t);


void debug_print(printf_buffer_t *buf, const memcached_protocol_t::write_t& write);
void debug_print(printf_buffer_t *buf, const memcached_protocol_t::backfill_chunk_t& chunk);

#endif /* MEMCACHED_PROTOCOL_HPP_ */
