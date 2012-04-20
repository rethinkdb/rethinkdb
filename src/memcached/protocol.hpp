#ifndef MEMCACHED_PROTOCOL_HPP_
#define MEMCACHED_PROTOCOL_HPP_

#include <vector>

#include "errors.hpp"
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>

#include "btree/slice.hpp"
#include "btree/backfill.hpp"
#include "btree/parallel_traversal.hpp"  // TODO: sigh
#include "buffer_cache/mirrored/config.hpp"
#include "buffer_cache/types.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/iterators.hpp"
#include "memcached/queries.hpp"
#include "memcached/region.hpp"
#include "protocol_api.hpp"
#include "rpc/serialize_macros.hpp"
#include "serializer/log/log_serializer.hpp"
#include "timestamps.hpp"

inline write_message_t &operator<<(write_message_t &msg, const boost::intrusive_ptr<data_buffer_t> &buf) {
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

inline int deserialize(read_stream_t *s, boost::intrusive_ptr<data_buffer_t> *buf) {
    bool exists;
    int res = deserialize(s, &exists);
    if (res) { return res; }
    if (exists) {
        int64_t size;
        int res = deserialize(s, &size);
        if (res) { return res; }
        if (size < 0) { return -3; }
        *buf = data_buffer_t::create(size);
        int64_t num_read = force_read(s, (*buf)->buf(), size);

        if (num_read == -1) { return -1; }
        if (num_read < size) { return -2; }
        rassert(num_read == size);
    }
    return 0;
}

template<typename T>
class vector_backed_one_way_iterator_t : public one_way_iterator_t<T> {
    typename std::vector<T> data;
    size_t pos;
public:
    vector_backed_one_way_iterator_t() : pos(0) { }
    void push_back(T &v) { data.push_back(v); }
    typename boost::optional<T> next() {
        if (pos < data.size()) {
            return boost::optional<T>(data[pos++]);
        } else {
            return boost::optional<T>();
        }
    }
    void prefetch() { }
};


inline write_message_t &operator<<(write_message_t &msg, const rget_result_t &iter) {
    while (boost::optional<key_with_data_buffer_t> pair = iter->next()) {
        const key_with_data_buffer_t &kv = pair.get();

        const std::string &key = kv.key;
        const boost::intrusive_ptr<data_buffer_t> &data = kv.value_provider;
        bool next = true;
        msg << next;
        msg << key;
        msg << data;
    }
    bool next = false;
    msg << next;
    return msg;
}

inline int deserialize(read_stream_t *s, rget_result_t *iter) {
    *iter = rget_result_t(new vector_backed_one_way_iterator_t<key_with_data_buffer_t>());
    bool next;
    for (;;) {
        int res = deserialize(s, &next);
        if (res) { return res; }
        if (!next) {
            return 0;
        }

        // TODO: See the load function above.  I'm guessing this code is never used.
        std::string key;
        res = deserialize(s, &key);
        if (res) { return res; }
        boost::intrusive_ptr<data_buffer_t> data;
        res = deserialize(s, &data);
        if (res) { return res; }

        // You'll note that we haven't put the values in the vector-backed iterator.  Neither does the load function above...
    }

}


RDB_MAKE_SERIALIZABLE_1(get_query_t, key);
RDB_MAKE_SERIALIZABLE_4(rget_query_t, left_mode, left_key, right_mode, right_key);
RDB_MAKE_SERIALIZABLE_3(get_result_t, value, flags, cas);
RDB_MAKE_SERIALIZABLE_3(key_with_data_buffer_t, key, mcflags, value_provider);
RDB_MAKE_SERIALIZABLE_1(get_cas_mutation_t, key);
RDB_MAKE_SERIALIZABLE_7(sarc_mutation_t, key, data, flags, exptime, add_policy, replace_policy, old_cas);
RDB_MAKE_SERIALIZABLE_2(delete_mutation_t, key, dont_put_in_delete_queue);
RDB_MAKE_SERIALIZABLE_3(incr_decr_mutation_t, kind, key, amount);
RDB_MAKE_SERIALIZABLE_2(incr_decr_result_t, res, new_value);
RDB_MAKE_SERIALIZABLE_3(append_prepend_mutation_t, kind, key, data);
RDB_MAKE_SERIALIZABLE_6(backfill_atom_t, key, value, flags, exptime, recency, cas_or_zero);

/* `memcached_protocol_t` is a container struct. It's never actually
instantiated; it just exists to pass around all the memcached-related types and
functions that the query-routing logic needs to know about. */

class memcached_protocol_t {
public:
    typedef key_range_t region_t;

    struct temporary_cache_t { };

    struct read_response_t {
        typedef boost::variant<get_result_t, rget_result_t> result_t;

        read_response_t() { }
        read_response_t(const read_response_t& r) : result(r.result) { }
        explicit read_response_t(const result_t& r) : result(r) { }

        result_t result;
        RDB_MAKE_ME_SERIALIZABLE_1(result);
    };

    struct read_t {
        typedef boost::variant<get_query_t, rget_query_t> query_t;

        key_range_t get_region() const THROWS_NOTHING;
        read_t shard(const key_range_t &region) const THROWS_NOTHING;
        read_response_t unshard(std::vector<read_response_t> responses, temporary_cache_t *cache) const THROWS_NOTHING;

        read_t() { }
        read_t(const read_t& r) : query(r.query), effective_time(r.effective_time) { }
        read_t(const query_t& q, exptime_t et) : query(q), effective_time(et) { }

        query_t query;
        exptime_t effective_time;

        RDB_MAKE_ME_SERIALIZABLE_1(query);
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
        key_range_t get_region() const THROWS_NOTHING;
        write_t shard(key_range_t region) const THROWS_NOTHING;
        write_response_t unshard(std::vector<write_response_t> responses, temporary_cache_t *cache) const THROWS_NOTHING;

        write_t() { }
        write_t(const write_t& w) : mutation(w.mutation), proposed_cas(w.proposed_cas), effective_time(w.effective_time) { }
        write_t(const query_t& m, cas_t pc, exptime_t et) : mutation(m), proposed_cas(pc), effective_time(et) { }

        query_t mutation;
        cas_t proposed_cas;
        exptime_t effective_time;   /* so operations are deterministic even with expiration */

        RDB_MAKE_ME_SERIALIZABLE_2(mutation, proposed_cas);
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
            key_range_t range;

            delete_range_t() { }
            explicit delete_range_t(const key_range_t& _range) : range(_range) { }

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
        boost::variant<delete_range_t, delete_key_t, key_value_pair_t> val;

        static backfill_chunk_t delete_range(const key_range_t& range) {
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

    class store_t : public store_view_t<memcached_protocol_t> {
        typedef region_map_t<memcached_protocol_t, binary_blob_t> metainfo_t;

        boost::scoped_ptr<standard_serializer_t> serializer;
        mirrored_cache_config_t cache_dynamic_config;
        boost::scoped_ptr<cache_t> cache;
        boost::scoped_ptr<btree_slice_t> btree;
        order_source_t order_source;

        fifo_enforcer_source_t token_source;
        fifo_enforcer_sink_t token_sink;

    public:
        store_t(const std::string& filename, bool create);
        ~store_t();

        void new_read_token(boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> &token_out);
        void new_write_token(boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> &token_out);

        metainfo_t get_metainfo(
                boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> &token,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t);

        void set_metainfo(
                const metainfo_t &new_metainfo,
                boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> &token,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t);

        memcached_protocol_t::read_response_t read(
                DEBUG_ONLY(const metainfo_t& expected_metainfo, )
                const memcached_protocol_t::read_t &read,
                boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> &token,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t);

        memcached_protocol_t::write_response_t write(
                DEBUG_ONLY(const metainfo_t& expected_metainfo, )
                const metainfo_t& new_metainfo,
                const memcached_protocol_t::write_t &write,
                transition_timestamp_t timestamp,
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
                memcached_protocol_t::region_t subregion,
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
                got_superblock_t &sb_out,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t);
        void acquire_superblock_for_backfill(
                boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> &token,
                boost::scoped_ptr<transaction_t> &txn_out,
                got_superblock_t &sb_out,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t);
        void acquire_superblock_for_write(
                access_t access,
                int expected_change_count,
                boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> &token,
                boost::scoped_ptr<transaction_t> &txn_out,
                got_superblock_t &sb_out,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t);
        void check_and_update_metainfo(
            DEBUG_ONLY(const metainfo_t& expected_metainfo, )
            const metainfo_t &new_metainfo,
            transaction_t *txn,
            got_superblock_t &superbloc) const
            THROWS_NOTHING;
        metainfo_t check_metainfo(
            DEBUG_ONLY(const metainfo_t& expected_metainfo, )
            transaction_t *txn,
            got_superblock_t &superbloc) const
            THROWS_NOTHING;
        void update_metainfo(const metainfo_t &old_metainfo, const metainfo_t &new_metainfo, transaction_t *txn, got_superblock_t &superbloc) const THROWS_NOTHING;
};
};

#endif /* MEMCACHED_PROTOCOL_HPP_ */
