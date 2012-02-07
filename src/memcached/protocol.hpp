#ifndef __MEMCACHED_PROTOCOL_HPP__
#define __MEMCACHED_PROTOCOL_HPP__

#include <vector>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/variant.hpp>

#include "btree/slice.hpp"
#include "btree/operations.hpp"
#include "btree/backfill.hpp"
#include "buffer_cache/types.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "memcached/queries.hpp"
#include "protocol_api.hpp"
#include "rpc/serialize_macros.hpp"
#include "timestamps.hpp"
#include "containers/iterators.hpp"

namespace boost {
namespace serialization {

template<class Archive> void save(Archive &ar, const boost::intrusive_ptr<data_buffer_t> &buf, UNUSED const unsigned int version) {
    int64_t sz = buf->size();
    ar & sz;
    ar.save_binary(buf.get()->buf(), buf.get()->size());
}

template<class Archive> void load(Archive &ar, boost::intrusive_ptr<data_buffer_t> &value, UNUSED const unsigned int version) {
    int64_t size;
    ar >> size;
    value = data_buffer_t::create(size);
    ar.load_binary(value->buf(), size);
}

template<class Archive> void save(Archive &ar, rget_result_t &iter, UNUSED const unsigned int version) {
    while (boost::optional<key_with_data_buffer_t> pair = iter->next()) {
        const key_with_data_buffer_t& kv = pair.get();

        const std::string& key = kv.key;
        const boost::intrusive_ptr<data_buffer_t>& data = kv.value_provider;
        ar << true << key << data;
    }
    ar << false;
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

template<class Archive> void load(Archive &ar, rget_result_t &iter, UNUSED const unsigned int version) {
    iter = rget_result_t(new vector_backed_one_way_iterator_t<key_with_data_buffer_t>());
    bool next;
    while (ar >> next, next) {
        std::string key;
        boost::intrusive_ptr<data_buffer_t> data;
        ar >> key >> data;
    }
}

}}

BOOST_SERIALIZATION_SPLIT_FREE(boost::intrusive_ptr<data_buffer_t>);
BOOST_SERIALIZATION_SPLIT_FREE(rget_result_t);

RDB_MAKE_SERIALIZABLE_1(get_query_t, key);
RDB_MAKE_SERIALIZABLE_4(rget_query_t, left_mode, left_key, right_mode, right_key);
RDB_MAKE_SERIALIZABLE_4(get_result_t, is_not_allowed, value, flags, cas);
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
        read_t(const read_t& r) : query(r.query) { }
        explicit read_t(const query_t& q) : query(q) { }

        query_t query;

        RDB_MAKE_ME_SERIALIZABLE_1(query);
    };

    struct write_response_t {
        typedef mutation_result_t::result_variant_t result_t;

        write_response_t() { }
        write_response_t(const write_response_t& w) : result(w.result) { }
        explicit write_response_t(const result_t& rv) : result(rv) { }

        result_t result;
        RDB_MAKE_ME_SERIALIZABLE_1(result);
    };

    struct write_t {
        typedef mutation_t::mutation_variant_t query_t;
        key_range_t get_region() const THROWS_NOTHING;
        write_t shard(key_range_t region) const THROWS_NOTHING;
        write_response_t unshard(std::vector<write_response_t> responses, temporary_cache_t *cache) const THROWS_NOTHING;

        write_t() { }
        write_t(const write_t& w) : mutation(w.mutation), proposed_cas(w.proposed_cas) { }
        write_t(const mutation_t& m, cas_t pc) : mutation(m.mutation), proposed_cas(pc) { }
        write_t(const query_t& m, cas_t pc) : mutation(m), proposed_cas(pc) { }

        query_t mutation;
        cas_t proposed_cas;

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
        explicit backfill_chunk_t(boost::variant<delete_range_t,delete_key_t,key_value_pair_t> val_) : val(val_) { }
        boost::variant<delete_range_t,delete_key_t,key_value_pair_t> val;

        static backfill_chunk_t delete_range(const key_range_t& range) {
            return backfill_chunk_t(delete_range_t(range));
        }
        static backfill_chunk_t delete_key(const store_key_t& key, const repli_timestamp_t& recency) {
            return backfill_chunk_t(delete_key_t(key, recency));
        }
        static backfill_chunk_t set_key(const backfill_atom_t& key) {
            return backfill_chunk_t(key_value_pair_t(key));
        }
    };
};

class memcached_store_view_t : public store_view_t<memcached_protocol_t> {
    btree_slice_t *btree;
    sequence_group_t *seq_group;
    order_source_t order_source;
public:
    // This code relies on the fact that store_view_t::write_transaction_t is also a read_transaction_t
    class txn_t : public store_view_t<memcached_protocol_t>::write_transaction_t {
        btree_slice_t *btree;
        order_token_t token;
        boost::scoped_ptr<transaction_t> txn;
        got_superblock_t superblock;
    public:
        txn_t(btree_slice_t *btree_, sequence_group_t *seq_group, order_source_t &order_source, bool read_txn);

        region_map_t<memcached_protocol_t,binary_blob_t> get_metadata(signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);
        void set_metadata(const region_map_t<memcached_protocol_t, binary_blob_t> &new_metadata) THROWS_NOTHING;
        memcached_protocol_t::read_response_t read(const memcached_protocol_t::read_t &, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);
        memcached_protocol_t::write_response_t write(const memcached_protocol_t::write_t &write, transition_timestamp_t timestamp) THROWS_NOTHING;
        void send_backfill(const region_map_t<memcached_protocol_t,state_timestamp_t> &start_point, const boost::function<void(memcached_protocol_t::backfill_chunk_t)> &chunk_fun, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);
        void receive_backfill(const memcached_protocol_t::backfill_chunk_t &chunk, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);
    private:
        region_map_t<memcached_protocol_t,binary_blob_t> get_metadata_internal(transaction_t* txn, buf_lock_t& sb_buf) THROWS_NOTHING;
        buf_t * get_superblock_buf();
    };

    typedef txn_t read_transaction_t;
    typedef txn_t write_transaction_t;

    memcached_store_view_t(const key_range_t& key_range_, btree_slice_t * btree_, sequence_group_t *seq_group_) : store_view_t<memcached_protocol_t>(key_range_), btree(btree_), seq_group(seq_group_) { }

    boost::shared_ptr<store_view_t<memcached_protocol_t>::read_transaction_t> begin_read_transaction(UNUSED signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        return boost::shared_ptr<store_view_t<memcached_protocol_t>::read_transaction_t>(new txn_t(btree, seq_group, order_source, true));
    }
    boost::shared_ptr<store_view_t<memcached_protocol_t>::write_transaction_t> begin_write_transaction(UNUSED signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        return boost::shared_ptr<store_view_t<memcached_protocol_t>::write_transaction_t>(new txn_t(btree, seq_group, order_source, false));
    }
};

/* `dummy_memcached_store_view_t` is a `store_view_t<memcached_protocol_t>` that
forwards its operations to a `btree_slice_t`. Currently it's mostly just
stubs. */
class dummy_memcached_store_view_t : public store_view_t<memcached_protocol_t> {
public:
    dummy_memcached_store_view_t(key_range_t, btree_slice_t *);

    boost::shared_ptr<store_view_t<memcached_protocol_t>::read_transaction_t> begin_read_transaction(signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);
    boost::shared_ptr<store_view_t<memcached_protocol_t>::write_transaction_t> begin_write_transaction(signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

private:
    btree_slice_t *btree;
};

#endif /* __MEMCACHED_PROTOCOL_HPP__ */
