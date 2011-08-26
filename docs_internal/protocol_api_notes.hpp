/* NOTE: In this document, for a type to "act like a data type" means that it
is default-constructible, copy-constructible, copy-assignable, and destructible.
If it allocates heap memory, it must manage it on its own. */

namespace clustering {

/* `namespace_interface_t` is the main entry point that protocol parsers use to
perform reads and writes on a namespace. `protocol_t` is a config struct that
holds all the types specific to that protocol. */
template<class protocol_t>
class namespace_interface_t {

public:
    /* This document doesn't deal with the details of how to find namespaces.
    Tim's plan is that each protocol will keep its own directory of namespaces
    organized in a protocol-specific way. Riak could have a map from strings
    to namespaces, along with a way to synchronize the creation of namespaces.
    Memcached could have a set of (namespace, port) pairs. Redis could...
    do whatever makes sense for Redis. */
    namespace_interface_t(
        mailbox_cluster_t *cluster,
        metadata_view_t<namespace_metadata_t<protocol_t> > *namespace_metadata
        );
    ~namespace_interface_t();

    /* Performs the given read query on the namespace. [May block] */
    protocol_t::read_response_t read(protocol_t::read_t query);

    /* Performs the given write query on the namespace. [May block] */
    protocol_t::write_response_t write(protocol_t::write_t query);

    /* Reads and writes configuration information for the namespace, such as the
    replication factor. */
    ... configure(...);

private:
    ...
};

}   /* namespace clustering */

/* `?` indicates that something is protocol-specific. Each protocol will define
a `?_protocol_t` for itself and all the members therein. That `?_protocol_t`
will be passed as a template parameter to the clustering code so it knows how
to work with that protocol. */

namespace ? {

class ?_protocol_t {

public:
    /* `region_t` is like a set (in the mathematical sense) of keys, although
    not all sets can be expressed as `region_t`s. Protocols will probably be
    implemented as a range of keys, although it could be something weirder, like
    the set of all keys whose hashes fall into a certain range.

    In this document, we talk about regions using set notation. 
    `union(regions)` refers to the union of multiple regions. And so on. */

    class region_t {

    public:
        /* Returns true if this `region_t` is a superset of `x`. */
        bool contains(region_t x);

        /* Returns true if this `region_t` overlaps `x`. */
        bool overlaps(region_t x);

        /* Returns the `region_t` containing all keys both in this `region_t`
        and `x`. */
        region_t intersection(region_t x);

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
        temporary_cache_t();
        ~temporary_cache_t();

    private:
        ?
    };

    /* `read_t`, `read_response_t`, `write_t`, `write_response_t`, and
    `backfill_chunk_t` must all be serializable. */

    class read_t {

    public:
        /* Indicates which keys the read depends on. */
        region_t get_region();

        /* Breaks the read into several sub-reads for individual regions.
        [Precondition] union(regions) == read.get_region()
        [Precondition] forall x,y in regions: !x.overlaps(y)
        [Postcondition] read.shard(regions).size() == regions.size()
        [Postcondition] read.shard(regions)[i].get_region() IsSubsetOf regions[i]
        */
        std::vector<read_t> shard(std::vector<region_t> regions);

        /* Recombines the responses to a group of reads created by
        `read_t::shard()`.
        [Precondition] responses[i] == store->read(read.shard(regions)[i], ...)
        */
        read_response_t unshard(std::vector<read_response_t> responses, temporary_cache_t *cache);

        /* Other requirements: `read_t` must be serializable. `read_t` must act
        like a data type. */

    private:
        ?
    };

    class read_response_t {

        /* Other requirements: `read_response_t` must be serializable.
        `read_response_t` must act like a data type. */

    private:
        ?
    };

    class write_t {

    public:
        /* Indicates which keys the write depends on or will modify. */
        region_t get_region();

        /* Breaks the write into several sub-writes for individual regions.
        Preconditions and postconditions are the same as for `read_t::shard()`.
        */
        std::vector<write_t> shard(std::vector<region_t> regions);

        /* Recombines the responses to a group of reads created by
        `write_t::shard()`.
        [Precondition] responses[i] == store->write(write.shard(regions)[i], ...)
        */
        write_response_t unshard(std::vector<write_response_t> responses, temporary_cache_t *cache);

        /* Other requirements: `write_t` must be serializable. `write_t` must
        act like a data type. */

    private:
        ?
    };

    class write_response_t {

        /* Other requirements: `write_response_t` must be serializable.
        `write_response_t` must act like a data type. */

    private:
        ?
    };

    /* `store_t` is the object that performs actual operations on stored data.
    It is responsible for constructing its own cache and btree.

    Although operations may be run on the `store_t` concurrently, it should
    behave as though every operation's effect was instantaneous. For example, if
    the store receives a write and then immediately receives a read before the
    write is done, the read should see the effects of the write. */

    class store_t {

    public:
        /* Returns the same region that was passed to the constructor. */
        region_t get_region();

        /* A store can be either coherent or incoherent. Roughly, "incoherent"
        means you're in the middle of a backfill. The coherence of a store must
        be persisted to disk.
        [Precondition] !store.is_backfilling() */
        bool is_coherent();

        /* Returns the store's current timestamp.
        [Precondition] !store.is_backfilling() */
        repli_timestamp_t get_timestamp();

        /* Performs a read operation on the store. May not modify the store's
        state in any way. If `interruptor` is pulsed, then `read()` must either
        return or throw `interrupted_exc_t` within a constant amount of time.
        [Precondition] read.get_region() IsSubsetOf store.get_region()
        [Precondition] store.is_coherent()
        [Precondition] !store.is_backfilling()
        [May block]
        */
        read_response_t read(read_t read, order_token_t otok, signal_t *interruptor);

        /* Performs a write operation on the store. The effect on the stored
        state must be deterministic; if I have two `store_t`s in the same state
        and I call `write()` on both with the same parameters, then they must
        both transition to the same state. If `interruptor` is pulsed, then
        `write()` must either return or throw `interrupted_exc_t` within a
        constant amount of time. If `interrupted_exc_t` is thrown, the write may
        or may not have been completed, but it must not be left in an
        intermediate state.
        [Precondition] write.get_region() IsSubsetOf store.get_region()
        [Precondition] store.is_coherent()
        [Precondition] !store.is_backfilling()
        [Precondition] timestamp >= store.get_timestamp()
        [Postcondition] store.get_timestamp() == timestamp
        [May block]
        */
        write_response_t write(write_t write, repli_timestamp_t timestamp, order_token_t otok, signal_t *interruptor);

        /* Returns `true` if the store is in the middle of a backfill. */
        bool is_backfilling();

        class backfill_request_t {

        public:
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
            ?
        };

        class backfill_chunk_t {

            /* Other requirements: `backfill_chunk_t` must be serializable.
            `backfill_chunk_t` must act like a data type. */

        private:
            ?
        };

        class backfill_end_t {

            /* Other requirements: `backfill_end_t` must be serializable.
            `backfill_end_t` must act like a data type. */

        private:
            ?
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
        is pulsed before the backfill is over, then `backfiller()` must either
        return or throw `interrupted_exc_t` within a constant amount of time. If
        it throws `interrupted_exc_t`, the backfill may be left incomplete.
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
        ?
    };

    /* Stores will be created and destroyed via some yet-unspecified mechanism.
    I don't know exactly what it will be yet. */

    /* NOTE: I don't know what form the `rebalance()` function will take in the
    finished product. I'm including it here because I know something like it
    will eventually be necessary. But it probably won't be a stand-alone
    function, and its type signature might be different.

    TODO: `rebalance()` should probably also be interruptible.

    Creates a set of stores which contain the same data as `recyclees`, but
    whose regions are `goals`. Each store in `recyclees` must either be
    destroyed or re-used as part of the return value.
    [Precondition] The regions of the stores in `recyclees` must not overlap.
    [Precondition] The regions in `goals` must not overlap.
    [Precondition] The union of the regions in `recyclees` must be the same
        as the union of the regions in `goals`.
    [Postcondition] store_manager.rebalance(recyclees, goals)[i].get_region() == goals[i]
    [Postcondition] store_manager.rebalance(recyclees, goals).size() == goals.size()
    [May block] */
    static std::vector<store_t *> rebalance(
        std::vector<store_t *> recyclees,
        std::vector<region_t> goals);
};

}   /* namespace ? */

/* Notes on ordering guarantees:

1.  All the replicas of each individual key will see writes in the same order.

    Example: Suppose K = "x". You send (append "a" to K) and (append "b" to K)
    concurrently from different nodes. Either every copy of K will become "xab",
    or every copy of K will become "xba", but the different copies of K will
    never disagree.

2.  Queries from the same origin will be performed in same order they are sent.

    Example: Suppose K = "a". You send (set K to "b") and (read K) from the same
    thread on the same node, in that order. The read will return "b".

3.  Arbitrary atomic single-key operations can be performed, as long as the
    `protocol_t::store_t` supports them.

4.  There are no other atomicity or ordering guarantees.

    Example: Suppose K1 = "x" and K2 = "x". You send (append "a" to every key)
    and (append "b" to every key) concurrently. Every copy of K1 will agree with
    every other copy of K1, and every copy of K2 will agree with every other
    copy of K2, but K1 and K2 may disagree.

    Example: Suppose K = "a". You send (set K to "b"). As soon as it's sent, you
    send (set K to "c") from a different node. K may end up being either "b" or
    "c".

    Example: Suppose K1 = "a" and K2 = "a". You send (set K1 to "b") and (set K2
    to "b") from the same node, in that order. Then you send (read K1 and K2)
    from a different node. The read may return (K1 = "a", K2 = "b").

5.  There is no simple way to perform an atomic multikey transaction. You might
    be able to fake it by using a key as a "lock".
*/
