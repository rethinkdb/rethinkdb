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

/* `store_view_t` is an abstract class that represents a key-value store or a
piece thereof. The protocol-specific logic should implement a subclass of
`store_view_t` that overrides `do_read()`, `do_write()`, `do_send_backfill()`,
and `do_receive_backfill()` to perform operations on some underlying B-tree or
other storage structure. */

template<class protocol_t>
class store_view_t {

public:
    virtual ~store_view_t() { }

    typename protocol_t::region_t get_region() {
        return region;
    }

    typename protocol_t::read_response_t read(typename protocol_t::read_t r, state_timestamp_t t, order_token_t otok, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        rassert(region_is_superset(region, r.get_region()));
        otok = order_checkpoint.check_through(otok);
        return do_read(r, t, otok, interruptor);
    }

    typename protocol_t::write_response_t write(typename protocol_t::write_t w, transition_timestamp_t t, order_token_t otok) THROWS_NOTHING {
        rassert(region_is_superset(region, w.get_region()));
        otok = order_checkpoint.check_through(otok);
        return do_write(w, t, otok);
    }

    void send_backfill(
            std::vector<std::pair<typename protocol_t::region_t, state_timestamp_t> > start_point,
            boost::function<void(typename protocol_t::backfill_chunk_t)> chunk_fun,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
        for (int i = 0; i < (int)start_point.size(); i++) {
            rassert(region_contains(region, start_point[i].first));
            for (int j = i + 1; j < (int)start_point.size(); j++) {
                rassert(!region_overlaps(start_point[i].first, start_point[j].first));
            }
        }
        do_send_backfill(start_point, chunk_fun, interruptor);
    }

    void receive_backfill(
            typename protocol_t::backfill_chunk_t chunk,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
        do_recv_backfill(chunk, interruptor);
    }

protected:
    store_view_t(typename protocol_t::region_t r) : region(r) { }

    virtual typename protocol_t::read_response_t do_read(
        const typename protocol_t::read_t &r,
        state_timestamp_t t,
        order_token_t otok,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) = 0;

    virtual typename protocol_t::write_response_t do_write(
        const typename protocol_t::write_t &w,
        transition_timestamp_t t,
        order_token_t otok)
        THROWS_NOTHING = 0;

    virtual void do_send_backfill(
        std::vector<std::pair<typename protocol_t::region_t, state_timestamp_t> > start_point,
        boost::function<void(typename protocol_t::backfill_chunk_t)> chunk_fun,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) = 0;

    virtual void do_receive_backfill(
        typename protocol_t::backfill_chunk_t chunk,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) = 0;

private:
    typename protocol_t::region_t region;
    order_checkpoint_t order_checkpoint;
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
