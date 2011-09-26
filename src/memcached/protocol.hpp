#ifndef __MEMCACHED_PROTOCOL_HPP__
#define __MEMCACHED_PROTOCOL_HPP__

#include <vector>

#include "errors.hpp"
#include <boost/variant.hpp>

#include "btree/slice.hpp"
#include "buffer_cache/types.hpp"
#include "memcached/queries.hpp"
#include "protocol_api.hpp"
#include "timestamps.hpp"

/* `key_range_t` represents a contiguous set of keys. */

class key_range_t {

public:
    enum bound_t {
        open,
        closed,
        none
    };

    key_range_t();   /* creates a range containing no keys */
    key_range_t(bound_t, store_key_t, bound_t, store_key_t);

    static key_range_t empty() THROWS_NOTHING {
        return key_range_t();
    }

    /* If `right.unbounded`, then the range contains all keys greater than or
    equal to `left`. If `right.bounded`, then the range contains all keys
    greater than or equal to `left` and less than `right.key`. */
    struct right_bound_t {
        right_bound_t() : unbounded(true) { }
        right_bound_t(store_key_t k) : unbounded(false), key(k) { }
        bool unbounded;
        store_key_t key;
    };
    store_key_t left;
    right_bound_t right;
};

bool region_is_superset(const key_range_t &potential_superset, const key_range_t &potential_subset) THROWS_NOTHING;
key_range_t region_intersection(const key_range_t &r1, const key_range_t &r2) THROWS_NOTHING;
key_range_t region_join(const std::vector<key_range_t> &vec) THROWS_ONLY(bad_join_exc_t, bad_region_exc_t);
bool region_overlaps(const key_range_t &r1, const key_range_t &r2) THROWS_NOTHING;

bool operator==(key_range_t, key_range_t) THROWS_NOTHING;
bool operator!=(key_range_t, key_range_t) THROWS_NOTHING;

/* `memcached_protocol_t` is a container struct. It's never actually
instantiated; it just exists to pass around all the memcached-related types and
functions that the query-routing logic needs to know about. */

class memcached_protocol_t {

public:
    typedef key_range_t region_t;

    class temporary_cache_t {
    };

    class read_response_t {

    public:
        read_response_t() { }
        read_response_t(const read_response_t& r) : result(r.result) { }
        template<class T> read_response_t(const T& r) : result(r) { }

        boost::variant<get_result_t, rget_result_t> result;
    };

    class read_t {

    public:
        key_range_t get_region() const THROWS_NOTHING;
        read_t shard(const key_range_t &region) const THROWS_NOTHING;
        read_response_t unshard(std::vector<read_response_t> responses, temporary_cache_t *cache) const THROWS_NOTHING;

        read_t() { }
        read_t(const read_t& r) : query(r.query) { }
        template<class T> read_t(const T& q) : query(q) { }

        boost::variant<get_query_t, rget_query_t> query;
    };

    class write_response_t {

    public:
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

    class write_t {

    public:
        key_range_t get_region() const THROWS_NOTHING;
        write_t shard(key_range_t region) const THROWS_NOTHING;
        write_response_t unshard(std::vector<write_response_t> responses, temporary_cache_t *cache) const THROWS_NOTHING;

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

    class backfill_chunk_t {

        /* stub */
    };
};

/* `dummy_memcached_store_view_t` is a `store_view_t<memcached_protocol_t>` that
forwards its operations to a `btree_slice_t`. Its initial timestamp is
hardcoded at zero, and it doesn't persist the timestamp anywhere in the btree.
*/

class dummy_memcached_store_view_t : public store_view_t<memcached_protocol_t> {

public:
    dummy_memcached_store_view_t(key_range_t, btree_slice_t *);

protected:
    /* `store_view_t` interface: */

    memcached_protocol_t::read_response_t do_read(const memcached_protocol_t::read_t &r, state_timestamp_t t, order_token_t otok, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);
    memcached_protocol_t::write_response_t do_write(const memcached_protocol_t::write_t &w, transition_timestamp_t t, order_token_t otok) THROWS_NOTHING;

    void do_send_backfill(
        UNUSED std::vector<std::pair<key_range_t, state_timestamp_t> > start_point,
        UNUSED boost::function<void(memcached_protocol_t::backfill_chunk_t)> chunk_sender,
        UNUSED signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t)
    {
        crash("stub");
    }

    void do_receive_backfill(
        UNUSED memcached_protocol_t::backfill_chunk_t chunk,
        UNUSED signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t)
    {
        crash("stub");
    }

private:
    btree_slice_t *btree;
};

#endif /* __MEMCACHED_PROTOCOL_HPP__ */
