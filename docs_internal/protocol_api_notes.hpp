// Copyright 2010-2012 RethinkDB, all rights reserved.
/* NOTE: In this document, for a type to "act like a data type" means that it
is default-constructible, copy-constructible, copy-assignable, and destructible.
If it allocates heap memory, it must manage it on its own. */

/* `?` indicates that something is protocol-specific. Each protocol will define
a `?_protocol_t` for itself and all the members therein. That `?_protocol_t`
will be passed as a template parameter to the clustering code so it knows how
to work with that protocol. */

namespace ? {

class ?_protocol_t {

public:
    /* `region_t` is like a set (in the mathematical sense) of keys, although
    not all sets can be expressed as `region_t`s. Protocols will probably
    implement `region_t` as a range of keys, although it could be something
    weirder, like the set of all keys whose hashes fall into a certain range. */

    class region_t {

    public:
        /* Returns an empty `region_t` that every other `region_t` is a superset
        of. */
        static region_t empty() THROWS_NOTHING;

        /* Returns a `region_t` that is a superset of every other `region_t`. */
        static region_t universe() THROWS_NOTHING;

        /* Most of the `region_t` operations are defined as standalone
        functions. See below. */

        /* Other requirements: `region_t` must be serializable. The "==" and
        "!=" operators must work on `region_t`. `region_t` must act like a data
        type. */

    private:
        ?
    };

    /* Every call to `unshard()` will get a `temporary_cache_t *`. The
    `temporary_cache_t` will be constructed by the routing logic. A
    `temporary_cache_t` may be used for only one call to `unshard()`, or it may
    be reused repeatedly with different `unshard()` calls, and `unshard()`
    should behave the same in either case. The `temporary_cache_t` may be
    constructed on any thread and used on any thread or threads at the same
    time.

    The intended use case is to construct interpreter contexts for protocols
    that include embedded Javascript or Lua. In theory a new interpreter could
    be created for each call, but that would be expensive; the
    `temporary_cache_t` can be used to hold a cached interpreter to improve
    performance. In practice, the `temporary_cache_t` will probably be used many
    times, so this will be a significant performance improvement. */

    class temporary_cache_t {

    public:
        temporary_cache_t() THROWS_NOTHING;
        ~temporary_cache_t() THROWS_NOTHING;

    private:
        ?
    };

    /* `read_t`, `read_response_t`, `write_t`, `write_response_t`, and
    `backfill_chunk_t` must all be serializable. */

    class read_t {

    public:
        /* Indicates which keys the read depends on. */
        region_t get_region() const THROWS_NOTHING;

        /* Extracts the part of the read that affects the given region.
        TODO: This should be called `mask()` instead.
        [Precondition] read.get_region().contains(region)
        [Postcondition] region.contains(read.shard(region).get_region())
        */
        read_t shard(region_t region) const THROWS_NOTHING;

        /* Recombines the responses to a group of reads created by
        `read_t::shard()`, that were sharded in the key-range direction. */
        read_response_t unshard(const std::vector<read_response_t>& responses, temporary_cache_t *cache) const THROWS_NOTHING;

        /* Recombines the responses to a group of reads, that were
           sharded in the hash-range direction. */
        read_response_t multistore_unshard(const std::vector<read_response_t>& responses, temporary_cache_t *cache) const THROWS_NOTHING;

        /* Other requirements: `read_t` must be serializable. `read_t` must act
        like a data type. */

    private:
        ?
    };

    class read_response_t {

        /* `read_response_t` must be serializable. `read_response_t` must act
        like a data type. */

    private:
        ?
    };

    class write_t {

    public:
        /* Indicates which keys the write depends on or will modify. */
        region_t get_region() const THROWS_NOTHING;

        /* Extracts the part of the write that affects the given region.
        TODO: This should be called `mask()` instead.
        [Precondition] write.get_region().contains(region)
        [Postcondition] region.contains(write.shard(region).get_region())
        */
        write_t shard(region_t region) const THROWS_NOTHING;

        /* Recombines the responses to a group of reads created by
        `write_t::shard()`. */
        write_response_t unshard(const std::vector<write_response_t>& responses, temporary_cache_t *cache) const THROWS_NOTHING;

        /* Recombines the responses to a group of writes, that were
           sharded in the hash-range direction. */
        write_response_t multistore_unshard(const std::vector<write_response_t>& responses, temporary_cache_t *cache) const THROWS_NOTHING;

        /* Other requirements: `write_t` must be serializable. `write_t` must
        act like a data type. */

    private:
        ?
    };

    class write_response_t {

        /* `write_response_t` must be serializable. `write_response_t` must act
        like a data type. */

    private:
        ?
    };

    class backfill_chunk_t {

        /* `backfill_chunk_t` must be serializable. `backfill_chunk_t` must act
        like a data type. */

        region_t get_region() const THROWS_NOTHING;

        // [Precondition] see write_t
        // [Postcondition] see write_t
        backfill_chunk_t shard(region_t region) const THROWS_NOTHING;

    private:
        ?
    };
};

/* Returns true if `potential_superset` is a superset of `potential_subset`. */
bool region_is_superset(const ?_protocol_t::region_t &potential_superset, const ?_protocol_t::region_t &potential_subset) THROWS_NOTHING;

/* Returns the intersection of `r1` and `r2`. */
?_protocol_t::region_t region_intersection(const ?_protocol_t::region_t &r1, const ?_protocol_t::region_t &r2) THROWS_NOTHING;

/* If one of the regions in `vec` overlaps another one of the regions in `vec`,
throws `bad_join_exc_t`. If the union of the regions in `vec` cannot be
expressed as a region, throws `bad_region_exc_t`. (If both are true, it can
throw either exception.) Otherwise, returns the union of the regions in `vec`.
*/
?_protocol_t::region_t region_join(const std::vector<?_protocol_t::region_t> &vec) THROWS_ONLY(bad_join_exc_t, bad_region_exc_t);

/* There are some other `region_*` functions in `protocol_api.hpp` that you can
overload for better performance. By default, they are implemented in terms of
the primitive `region_*` functions defined here. */

}   /* namespace ? */

// Also, a proper store_view_t<protocol_t> needs to be implemented. See protocol_api.hpp.
