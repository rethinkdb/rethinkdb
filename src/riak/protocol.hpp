#ifndef __RIAK_PROTOCOL_HPP__
#define __RIAK_PROTOCOL_HPP__

#include <set>
#include <string>
#include <vector>
#include <boost/variant.hpp>
#include "utils.hpp"
#include <boost/bind.hpp>
#include "riak/structures.hpp"
#include "javascript/javascript.hpp"
#include "riak/riak_interface.hpp"

namespace riak {
/* a region defines a subset (non-strict) of a keyspace where the keyspace is just the set of strings */

class region_t {
private:
    unsigned hash(std::string) { crash("Not implemented"); }
public:
    typedef std::set<std::string> finite_t;
    typedef std::pair<unsigned, unsigned> hash_range_t; //keys which hash to something in the range, [first, second)

    //universe_t and null_t are never implemented anywhere because they
    //have no implementation null sets and universe sets are all the same 
    //class universe_t {};
    //class null_t {};
public:
    static bool hash_range_contains_key(hash_range_t, std::string) { crash("Not implemented"); }
    static bool hash_range_contains_hash(hash_range_t, unsigned) { crash("Not implemented"); }

    /* regions can define the subset in one of 3 ways:
     * - as an finite set of keys
     * - as a range of hash values
     */
    typedef boost::variant<finite_t, hash_range_t> key_spec_t;
    key_spec_t key_spec;

public:
    region_t(key_spec_t);

public:
    typedef finite_t::const_iterator key_it_t;

    //does x contain y
    
    struct contains_functor : public boost::static_visitor<bool> {
        bool operator()(const finite_t &, const finite_t &) const;
        bool operator()(const finite_t &, const hash_range_t &) const;
        bool operator()(const hash_range_t &, const finite_t &) const;
        bool operator()(const hash_range_t &, const hash_range_t &) const;
    };

    struct overlaps_functor : public boost::static_visitor<bool> {
        bool operator()(const finite_t &, const finite_t &) const;
        bool operator()(const finite_t &, const hash_range_t &) const;
        bool operator()(const hash_range_t &, const finite_t &) const;
        bool operator()(const hash_range_t &, const hash_range_t &) const;
    };

    struct intersection_functor : public boost::static_visitor<key_spec_t> {
        key_spec_t operator()(const finite_t &, const finite_t &) const;
        key_spec_t operator()(const finite_t &, const hash_range_t &) const;
        key_spec_t operator()(const hash_range_t &, const finite_t &) const;
        key_spec_t operator()(const hash_range_t &, const hash_range_t &) const;
    };

public:
    bool contains(region_t &);
    bool overlaps(region_t &);
    region_t intersection(region_t &);

public:
    static region_t universe();
    static region_t null();
};

class temporary_cache_t;

class read_t;
//class read_response_t;


class point_read_t;
class point_read_response_t;

class bucket_read_t;
class bucket_read_response_t;

class mapred_read_t;
class mapred_read_response_t;


/* a point read represents reading a single key from the database */
class point_read_t {
private:
    friend class read_enactor_vistor_t;
    std::string key;
    boost::optional<std::pair<int, int> > range;
public:
    //point_read_t() { crash("Not implemented"); }
    point_read_t(std::string);
    point_read_t(std::string, std::pair<int, int>);
public:
    region_t get_region() const;
    std::vector<read_t> shard(std::vector<region_t>);
};

class point_read_response_t {
public:
    point_read_response_t(object_t _result) : result(_result) { }
private:
    object_t result;
};

/* bucket read represents reading all of the data from a bucket */
class bucket_read_t {
private:
    region_t region_limit;
public:
    bucket_read_t() : region_limit(region_t::universe()) { crash("Not implemented"); }
    bucket_read_t(region_t _region_limit) : region_limit(_region_limit) { crash("Not implemented"); }
public:
    region_t get_region() const;
    std::vector<read_t> shard(std::vector<region_t>);
    bucket_read_response_t combine(const bucket_read_response_t &, const bucket_read_response_t &) const;
};

class bucket_read_response_t {
public:
    bucket_read_response_t() { }

    bucket_read_response_t(object_iterator_t obj_it) {
        while (boost::optional<object_t> cur = obj_it.next()) {
            keys.push_back(cur->key);
        }
    }
    bucket_read_response_t(const bucket_read_response_t &x, const bucket_read_response_t &y) {
        keys.insert(keys.end(), x.keys.begin(), x.keys.end());
        keys.insert(keys.end(), y.keys.begin(), y.keys.end());
    }
private:
    std::vector<std::string> keys;
};

/* mapred reads read a subset of the data */

class mapred_read_t {
private:
    region_t region;
public:
    mapred_read_t(region_t _region) : region(_region) {}
public:
    region_t get_region() const;
    std::vector<read_t> shard(std::vector<region_t> regions);
};

typedef JS::ctx_group_t unshard_ctx_t;

class mapred_read_response_t {
public:
    static mapred_read_response_t unshard(std::vector<mapred_read_response_t> response, unshard_ctx_t *);
    mapred_read_response_t(std::string _json_result)
        : json_result(_json_result)
    { }
    /* mapred_read_response_t(const mapred_read_response_t &, const mapred_read_respone_t &) {
        crash("Not implemented");
    } */
private:
    std::string json_result;
};

typedef boost::variant<point_read_t, bucket_read_t, mapred_read_t> read_variant_t;
typedef boost::variant<point_read_response_t, bucket_read_response_t, mapred_read_response_t> read_response_variant_t;

typedef read_response_variant_t read_response_t;

class read_t {
public:
    read_variant_t internal;
public:
    read_t(point_read_t r) : internal(r) {}
    read_t(bucket_read_t r) : internal(r) {}
    read_t(mapred_read_t r) : internal(r) {}
public:
    region_t get_region() const;
    std::vector<read_t> shard(std::vector<region_t>);
    read_response_t unshard(std::vector<read_response_t>);
};


//XXX this can just be a type deffing of the variant now
/* class read_response_t {
public:
    read_response_variant_t internal;
public:
    read_response_t(point_read_response_t _internal) : internal(_internal) { crash("Not implemented"); }
    read_response_t(bucket_read_response_t _internal) : internal(_internal) { crash("Not implemented"); }
    read_response_t(mapred_read_response_t _internal) : internal(_internal) { crash("Not implemented"); }
}; */

/* this class is used to actually enact a read_t */
class read_enactor_vistor_t : public boost::static_visitor<read_response_t> {
    read_response_t operator()(point_read_t, riak_interface_t *);
    read_response_t operator()(bucket_read_t, riak_interface_t *);
    read_response_t operator()(mapred_read_t, riak_interface_t *);
};

class write_t;

class set_write_t {
private:
    object_t object;
    boost::optional<etag_cond_spec_t> etag_cond_spec;
    boost::optional<time_cond_spec_t> time_cond_spec;

public:
    region_t get_region();
    std::vector<write_t> shard(std::vector<region_t> regions);
};

class set_write_response_t {
    //if a set is done without a key then we need to pick the key ourselves.
    //and tell them what we picked.
    boost::optional<std::string> key_if_created;

    //sometimes a set will ask that the object be sent back with the response. the object
    boost::optional<object_t> object;
};

class delete_write_t {
private:
    std::string key;
public:
    region_t get_region();
    std::vector<write_t> shard(std::vector<region_t> regions);
};

class delete_write_response_t {
    /* whether or not we actually found the write */
    bool found;
};

typedef boost::variant<set_write_t, delete_write_t> write_variant_t;
typedef boost::variant<set_write_response_t, delete_write_response_t> write_response_variant_t;

typedef write_response_variant_t write_response_t;

class write_t {
private:
    write_variant_t internal;
public:
    write_t(set_write_t _internal) : internal(_internal) { }
    write_t(delete_write_t  _internal) : internal(_internal) { }
public:
    region_t get_region();
    std::vector<write_t> shard(std::vector<region_t>);
    write_response_t unshard(std::vector<write_response_t>, temporary_cache_t);
};

/* Riak store */

class store_t {
private:
    region_t region;
    riak_interface_t *interface;
    bool backfilling;
public:
    store_t(region_t, riak_interface_t *);
    /* Returns the same region that was passed to the constructor. */
    region_t get_region();

    bool is_coherent();

    /* Returns the store's current timestamp.
       [Precondition] !store.is_backfilling() */
    repli_timestamp_t get_timestamp();

    /* Performs a read operation on the store. May not modify the store's
       state in any way.
       [Precondition] read.get_region() <= store.get_region()
       [Precondition] store.is_coherent()
       [Precondition] !store.is_backfilling()
       [May block]
       */
    read_response_t read(read_t read, order_token_t otok);

    /* Performs a write operation on the store. The effect on the stored
       state must be deterministic; if I have two `store_t`s in the same state
       and I call `write()` on both with the same parameters, then they must
       both transition to the same state.
       [Precondition] write.get_region() <= store.get_region()
       [Precondition] store.is_coherent()
       [Precondition] !store.is_backfilling()
       [Precondition] timestamp >= store.get_timestamp()
       [Postcondition] store.get_timestamp() == timestamp
       [May block]
       */
    write_response_t write(write_t write, repli_timestamp_t timestamp, order_token_t otok);

    /* Returns `true` if the store is in the middle of a backfill. */
    bool is_backfilling();

    struct backfill_request_t {

        /* You don't have to actually implement `get_region()` and
           `get_timestamp()`; they're only here to make it easier to describe
           preconditions and postconditions. */

        /* Returns the same value as the backfillee's `get_region()` method.
        */
        region_t get_region();

        /* Returns the same value as the backfillee's `get_timestamp()`
           method. */
        repli_timestamp_t get_timestamp();

        /* Other requirements: `backfill_request_t` must be serializable.
           `backfill_request_t` must act like a data type. */

        private:
    };

    struct backfill_chunk_t {

        /* Other requirements: `backfill_chunk_t` must be serializable.
           `backfill_chunk_t` must act like a data type. */

    private:
    };

    struct backfill_end_t {

        /* Other requirements: `backfill_end_t` must be serializable.
           `backfill_end_t` must act like a data type. */

    };

    /* Prepares the store for a backfill. Returns a `backfill_request_t`
       which expresses what information the store needs backfilled.
       [Precondition] !store.is_backfilling()
       [Postcondition] store.is_backfilling()
       [Postcondition] store.backfillee_begin().get_region() == store.get_region()
       [Postcondition] store.get_timestamp() == store.backfillee_begin().get_timestamp()
       [May block] */
    backfill_request_t backfillee_begin();

    /* Delivers a chunk of a running backfill.
       [Precondition] store.is_backfilling()
       [May block] */
    void backfillee_chunk(backfill_chunk_t);

    /* Notifies that the backfill is over.
       [Precondition] store.is_backfilling()
       [Postcondition] !store.is_backfilling()
       [Postcondition] store.is_coherent()
       [May block] */
    void backfillee_end(backfill_end_t);

    /* Notifies that the backfill won't be finished because something went
       wrong.
       [Precondition] store.is_backfilling()
       [Postcondition] !store.is_backfilling()
       [Postcondition] !store.is_coherent()
       [May block] */
    void backfillee_cancel();

    /* Sends a backfill to another store. `request` should be the return
       value of the backfillee's `backfillee_begin()` method. `backfiller()`
       should call `chunk_fun` with `backfill_chunk_t`s to be passed to the
       backfillee's `backfillee_chunk()` method. `backfiller()` should block
       until the backfill is done, and then return a `backfill_end_t` to be
       passed to the backfillee's `backfillee_end()` method. If `interruptor`
       is pulsed before the backfill is over, then `backfiller()` should stop
       the backfill and throw an `interrupted_exc_t` as soon as possible.
       [Precondition] request.get_region() == store.get_region()
       [Precondition] request.get_timestamp() <= store.get_timestamp()
       [Precondition] !store.is_backfilling()
       [Precondition] store.is_coherent()
       [May block] */
    backfill_end_t backfiller(
            backfill_request_t request,
            boost::function<void(backfill_chunk_t)> chunk_fun,
            signal_t *interruptor);

    /* Here's an example of how to use the backfill API. `backfill()` will
       copy data from `backfiller` to `backfillee` unless `interruptor` is
       pulsed, in which case it will throw `interrupted_exc_t`.

       void backfill(store_t *backfillee, store_t *backfiller, signal_t *interruptor) {
       backfill_request_t req = backfillee->backfillee_begin();
       backfill_end_t end;
       try {
       end = backfiller->backfiller(
       req,
       boost::bind(&store_t::backfillee_chunk, backfillee),
       interruptor);
       } catch (interrupted_exc_t) {
       backfillee->backfillee_cancel();
       throw;
       }
       backfillee->backfillee_end(end);
       }
       */

    private:
};

} //namespace riak 

#endif 
