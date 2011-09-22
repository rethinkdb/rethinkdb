#ifndef __PROTOCOL_API_HPP__
#define __PROTOCOL_API_HPP__

#include "concurrency/fifo_checker.hpp"
#include "concurrency/signal.hpp"
#include "timestamps.hpp"

/* This file describes the relationship between the protocol-specific logic for
each protocol and the protocol-agnostic logic that routes queries for all the
protocols. Each protocol defines a `protocol_t` struct that acts as a
"container" to hold all the types for that protocol. The protocol-agnostic logic
will be templatized on a `protocol_t`. `protocol_t` itself is never
instantiated. For a description of what `protocol_t` must be like, see
`rethinkdb/docs_internal/protocol_api_notes.hpp`. */

/* `namespace_interface_t` is the interface that the protocol-agnostic database
logic for query routing exposes to the protocol-specific query parser. */

template<class protocol_t>
struct namespace_interface_t {

    virtual typename protocol_t::read_response_t read(typename protocol_t::read_t, order_token_t tok, signal_t *interruptor) = 0;
    virtual typename protocol_t::write_response_t write(typename protocol_t::write_t, order_token_t tok, signal_t *interruptor) = 0;

protected:
    virtual ~namespace_interface_t() { }
};

/* Exceptions thrown by functions operating on `protocol_t::region_t` */

class bad_region_exc_t : public std::exception {
public:
    const char *what() const throw () {
        return "The set you're trying to compute cannot be expressed as a "
            "`region_t`.";
    }
};

class bad_join_exc_t : public std::exception {
public:
    const char *what() const throw () {
        return "You need to give a non-overlapping set of regions.";
    }
};

/* Some `protocol_t::region_t` functions can be implemented in terms of other
functions. Here are default implementations for those functions. */

template<class region_t>
bool region_is_empty(const region_t &r) {
    return region_is_superset(region_t::empty(), r);
}

template<class region_t>
bool region_overlaps(const region_t &r1, const region_t &r2) {
    return !region_is_empty(region_intersection(r1, r2));
}

/* `ready_store_view_t` is an abstract class that represents an up-to-date and
coherent store for some protocol. (Stores that need a backfill are represented
as `outdated_store_view_t`.) The protocol-specific logic should implement a subclass
of `ready_store_view_t<protocol_t>` and override `do_read()`, `do_write()`, and
`do_send_backfill()`. Then it can pass a
`boost::shared_ptr<ready_store_view_t<protocol_t> >` to some protocol-agnostic
query-routing logic which will invoke `read()`, `write()`, and
`send_backfill()`.*/

template<class protocol_t>
class ready_store_view_t {

public:
    virtual ~ready_store_view_t() { }

    typename protocol_t::region_t get_region() {
        return region;
    }

    state_timestamp_t get_timestamp() {
        return timestamp;
    }

    typename protocol_t::read_response_t read(typename protocol_t::read_t r, state_timestamp_t t, order_token_t otok, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        {
            mutex_acquisition_t acq(&sanity_mutex);
            rassert(region_is_superset(region, r.get_region()));
            rassert(timestamp == t);
            otok = order_checkpoint.check_through(otok);
        }
        return do_read(r, t, otok, interruptor);
    }

    typename protocol_t::write_response_t write(typename protocol_t::write_t w, transition_timestamp_t t, order_token_t otok, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        {
            mutex_acquisition_t acq(&sanity_mutex);
            rassert(region_is_superset(region, w.get_region()));
            rassert(timestamp == t.timestamp_before());
            timestamp = t.timestamp_after();
            otok = order_checkpoint.check_through(otok);
        }
        return do_write(w, t, otok, interruptor);
    }

    state_timestamp_t send_backfill(
            std::vector<std::pair<typename protocol_t::region_t, state_timestamp_t> > start_point,
            boost::function<void(typename protocol_t::backfill_chunk_t)> chunk_fun,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {

        state_timestamp_t end_point;
        {
            mutex_acquisition_t acq(&sanity_mutex);
            end_point = timestamp;

            /* Sanity checking */
            for (int i = 0; i < (int)start_point.size(); i++) {
                rassert(region_contains(region, start_point[i].first));
                rassert(start_point[i].second <= timestamp);
                for (int j = i + 1; j < (int)start_point.size(); j++) {
                    rassert(!region_overlaps(start_point[i].first, start_point[j].first));
                }
            }
        }
        do_send_backfill(start_point, chunk_fun, interruptor);
        return end_point;
    }

protected:
    ready_store_view_t(typename protocol_t::region_t r, state_timestamp_t t) : region(r), timestamp(t) { }

    /* `do_read()` performs the read expressed by `r`. If `interruptor` is
    pulsed, then `do_read()` should throw `interrupted_exc_t`. `t` and `otok`
    are just for sanity checking.
    [Precondition] region_is_superset(store.region, r.get_region())
    [Precondition] t == store.timestamp
    [May block] */
    virtual typename protocol_t::read_response_t do_read(const typename protocol_t::read_t &r, state_timestamp_t t, order_token_t otok, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;

    /* `do_write()` performs the write expressed by `w`. `interruptor`, `t`, and
    `otok` are as in `do_read()`.
    [Precondition] region_is_superset(store.region, w.get_region())
    [Precondition] t.timestamp_after() == store.timestamp   (because timestamp is updated right before `do_write()` is called)
    [May block] */
    virtual typename protocol_t::write_response_t do_write(const typename protocol_t::write_t &w, transition_timestamp_t t, order_token_t otok, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;

    virtual state_timestamp_t do_send_backfill(
        std::vector<std::pair<typename protocol_t::region_t, state_timestamp_t> > start_point,
        boost::function<void(typename protocol_t::backfill_chunk_t)> chunk_fun,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) = 0;

private:
    mutex_t sanity_mutex;

    typename protocol_t::region_t region;
    state_timestamp_t timestamp;

    order_checkpoint_t order_checkpoint;
};

/* `outdated_store_view_t` is an abstract class representing a store that needs to
receive a backfill. The protocol-specific logic should implement a subclass of
`outdated_store_view_t` and override `do_apply_backfill_chunk()` and `do_done()`.
Then it will pass a `boost::shared_ptr<outdated_store_view_t<protocol_t> >` along
with information about where the backfill needs to start to the
protocol-agnostic logic that will perform the backfill. The protocol-agnostic
logic will call `send_backfill()` on a `ready_store_view_t<protocol_t>` somewhere,
then forward the resulting `protocol_t::backfill_chunk_t` objects to the
`outdated_store_view_t`'s `receive_backfill_chunk()` method. When it's done, it will
call `done()` with the timestamp returned by the `ready_store_view_t`. `done()` will
produce a `boost::shared_ptr<ready_store_view_t<protocol_t> >` that contains the
backfilled data. */

template<class protocol_t>
class outdated_store_view_t {

public:
    virtual ~outdated_store_view_t() { }

    typename protocol_t::region_t get_region() {
        return region;
    }

    void receive_backfill_chunk(typename protocol_t::backfill_chunk_t chunk, signal_t *interruptor) {
        rassert(unfinished);
        if (interruptor) throw interrupted_exc_t();
        do_receive_backfill_chunk(chunk, interruptor);
    }

    boost::shared_ptr<ready_store_view_t<protocol_t> > done(state_timestamp_t ts) {
        rassert(unfinished);
        unfinished = false;
        boost::shared_ptr<ready_store_view_t<protocol_t> > this_as_ready_store = do_done(ts);
        rassert(this_as_ready_store->get_timestamp() == ts);
        rassert(this_as_ready_store->get_region() == region);
        return this_as_ready_store;
    }

protected:
    outdated_store_view_t(typename protocol_t::region_t r) :
        region(r), unfinished(true) { }

    virtual void do_receive_backfill_chunk(typename protocol_t::backfill_chunk_t, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;

    virtual boost::shared_ptr<ready_store_view_t<protocol_t> > do_done(state_timestamp_t ts) THROWS_NOTHING = 0;

private:
    typename protocol_t::region_t region;
    bool unfinished;
};

/* The query-routing logic provides the following ordering guarantees:

1.  All the replicas of each individual key will see writes in the same order.

    Example: Suppose K = "x". You send (append "a" to K) and (append "b" to K)
    concurrently from different nodes. Either every copy of K will become "xab",
    or every copy of K will become "xba", but the different copies of K will
    never disagree.

2.  Queries from the same origin will be performed in same order they are sent.

    Example: Suppose K = "a". You send (set K to "b") and (read K) from the same
    thread on the same node, in that order. The read will return "b".

3.  Arbitrary atomic single-key operations can be performed, as long as they can
    be expressed as `protocol_t::write_t` objects.

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

#endif /* __PROTOCOL_API_HPP__ */
