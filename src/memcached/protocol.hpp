#ifndef __MEMCACHED_MEMCACHED_PROTOCOL_HPP__
#define __MEMCACHED_MEMCACHED_PROTOCOL_HPP__

#include "namespace_interface.hpp"
#include "memcached/queries.hpp"
#include <boost/variant.hpp>
#include "serializer/serializer.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "btree/slice.hpp"

/* `key_range_t` represents a contiguous set of keys. */

struct key_range_t {

    enum bound_t {
        open,
        closed,
        none
    };

    key_range_t();   /* creates a range containing all keys */
    key_range_t(bound_t, store_key_t, bound_t, store_key_t);

    bool contains(key_range_t range);
    bool overlaps(key_range_t range);
    key_range_t intersection(key_range_t range);

    /* If `!right_unbounded`, the range contains all keys which are greater than
    or equal to `left` and less than `right`. If `right_unbounded`, it contains
    all keys greater than or equal to `left`. `right_unbounded` is the only way
    to represent ranges that contain the largest possible key. */
    store_key_t left, right;
    bool right_unbounded;
};

/* `memcached_protocol_t` is a container struct. It's never actually
instantiated; it just exists to pass around all the memcached-related types and
functions that the query-routing logic needs to know about. */

struct memcached_protocol_t {

    typedef key_range_t region_t;

    struct read_t {

        region_t get_region();
        std::vector<read_t> shard(std::vector<region_t> regions);
        std::vector<read_t> parallelize(int optimal_factor);

        read_t() { }
        read_t(const read_t& r) : query(r.query) { }
        template<class T> read_t(const T& q) : query(q) { }

        boost::variant<get_query_t, rget_query_t> query;
    };

    struct read_response_t {
        static read_response_t unshard(std::vector<read_response_t> responses);
        static read_response_t unparallelize(std::vector<read_response_t> responses);

        read_response_t() { }
        read_response_t(const read_response_t& r) : result(r.result) { }
        template<class T> read_response_t(const T& r) : result(r) { }

        boost::variant<get_result_t, rget_result_t> result;
    };

    struct write_t {
        region_t get_region();
        std::vector<write_t> shard(std::vector<region_t> regions);

        write_t() { }
        write_t(const write_t& w) : mutation(w.mutation), proposed_cas(w.proposed_cas) { }
        template<class T> write_t(const T& m, cas_t pc) : mutation(m), proposed_cas(pc) { }

        boost::variant<
            get_cas_mutation_t,
            sarc_mutation_t,
            delete_mutation_t,
            incr_decr_mutation_t,
            append_prepend_mutation_t
            > mutation;
        cas_t proposed_cas;
    };

    struct write_response_t {
        static write_response_t unshard(std::vector<write_response_t> responses);

        write_response_t() { }
        write_response_t(const write_response_t& w) : result(w.result) { }
        template<class T> write_response_t(const T& r) : result(r) { }

        boost::variant<
            get_result_t,   // for `gets` operations
            set_result_t,
            delete_result_t,
            incr_decr_result_t,
            append_prepend_result_t
            > result;
    };

    /* Backfilling not implemented

    struct backfill_chunk_t { ... }; */

    struct store_t {

        static void create(serializer_t *ser, region_t region);
        store_t(serializer_t *ser, region_t region);
        region_t get_region();

        read_response_t read(read_t read, order_token_t tok);
        write_response_t write(write_t write, repli_timestamp_t timestamp, order_token_t tok);

        /* Backfilling not implemented

        repli_timestamp_t get_timestamp();
        void send_backfill(
            region_t region,
            repli_timestamp_t timestamp,
            boost::function<void(backfill_chunk_t)> sender
            );
        void recv_backfill_chunk(backfill_chunk_t chunk);
        void recv_backfill_end(repli_timestamp_t timestamp); */

    private:
        mirrored_cache_config_t cache_config;
        cache_t cache;
        btree_slice_t btree;
        region_t region;
    };
};

#endif /* __MEMCACHED_MEMCACHED_PROTOCOL_HPP__ */
