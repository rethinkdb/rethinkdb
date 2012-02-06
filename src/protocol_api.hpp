#ifndef __PROTOCOL_API_HPP__
#define __PROTOCOL_API_HPP__

#include "errors.hpp"
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>   /* for `std::pair` serialization */
#include <functional>
#include <utility>
#include <vector>

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

struct bad_region_exc_t : public std::exception {
    const char *what() const throw () {
        return "The set you're trying to compute cannot be expressed as a "
            "`region_t`.";
    }
};

struct bad_join_exc_t : public std::exception {
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

/* Regions contained in region_map_t must never intersect. */
template<class protocol_t, class value_t>
class region_map_t {
public:
    typedef typename std::vector<std::pair<typename protocol_t::region_t, value_t> >::const_iterator const_iterator;

    region_map_t() THROWS_NOTHING { }
    region_map_t(typename protocol_t::region_t r, value_t v) THROWS_NOTHING {
        regions_and_values.push_back(std::make_pair(r, v));
    }

    explicit region_map_t(const std::vector<std::pair<typename protocol_t::region_t, value_t> > &x) THROWS_NOTHING :
        regions_and_values(x)
    {
        /* Make sure that the vector we were given is valid */
        DEBUG_ONLY(get_domain());
    }

    typename protocol_t::region_t get_domain() const THROWS_NOTHING {
        std::vector<typename protocol_t::region_t> regions;
        for (int i = 0; i < (int)regions_and_values.size(); i++) {
            regions.push_back(regions_and_values[i].first);
        }
        return region_join(regions);
    }

    std::vector<std::pair<typename protocol_t::region_t, value_t> > get_as_pairs() const {
        return regions_and_values;
    }

    const_iterator begin() const {
        return regions_and_values.begin();
    }

    const_iterator end() const {
        return regions_and_values.end();
    }

    region_map_t mask(typename protocol_t::region_t region) const {
        std::vector<std::pair<typename protocol_t::region_t, value_t> > masked_pairs;
        for (int i = 0; i < (int)regions_and_values.size(); i++) {
            typename protocol_t::region_t ixn = region_intersection(regions_and_values[i].first, region);
            if (!region_is_empty(ixn)) {
                masked_pairs.push_back(std::make_pair(ixn, regions_and_values[i].second));
            }
        }
        return region_map_t(masked_pairs);
    }

    // Important: 'update' assumes that new_values regions do not intersect
    region_map_t update(const region_map_t& new_values) {
        std::vector<typename protocol_t::region_t> overlay_regions;
        for (const_iterator i = new_values.begin(); i != new_values.end(); ++i) {
            overlay_regions.push_back((*i).first);
        }

        std::vector<std::pair<typename protocol_t::region_t, value_t> > updated_pairs;
        for (const_iterator i = begin(); i != end(); ++i) {
            typename protocol_t::region_t old = (*i).first;
            std::vector<typename protocol_t::region_t> old_subregions = region_subtract_many(old, overlay_regions);

            // Insert the unchanged parts of the old region into updated_pairs with the old value
            for (typename std::vector<typename protocol_t::region_t>::const_iterator j = old_subregions.begin(); j != old_subregions.end(); ++j) {
                updated_pairs.push_back(std::make_pair(*j, (*i).second));
            }
        }
        std::copy(new_values.begin(), new_values.end(), std::back_inserter(updated_pairs));

        return region_map_t(updated_pairs);
    }

private:
    std::vector<std::pair<typename protocol_t::region_t, value_t> > regions_and_values;
};

template<class protocol_t, class old_t, class new_t, class callable_t>
region_map_t<protocol_t, new_t> region_map_transform(const region_map_t<protocol_t, old_t> &original, const callable_t &callable) {
    std::vector<std::pair<typename protocol_t::region_t, old_t> > original_pairs = original.get_as_pairs();
    std::vector<std::pair<typename protocol_t::region_t, new_t> > new_pairs;
    for (int i = 0; i < (int)original_pairs.size(); i++) {
        new_pairs.push_back(std::make_pair(
                original_pairs[i].first,
                callable(original_pairs[i].second)
                ));
    }
    return region_map_t<protocol_t, new_t>(new_pairs);
}

/* `store_view_t` is an abstract class that represents a region of a key-value
store for some protocol. It's templatized on the protocol (`protocol_t`). It
covers some `protocol_t::region_t`, which is returned by `get_region()`.

In addition to the actual data, `store_view_t` is responsible for keeping track
of metadata which is keyed by region. The metadata is currently implemented as
opaque binary blob (`binary_blob_t`).

Here are the possible call sequences for read/write transactions:
 - begin_read_transaction get_metainfo* [read|send_backfill] destructor
 - begin_write_transaction (get_metainfo|set_metainfo)* [read|write|send_backfill|recv_backfill] destructor
*/

template<class protocol_t>
class store_view_t {
public:
    virtual ~store_view_t() { }

    typename protocol_t::region_t get_region() {
        return region;
    }

    struct read_transaction_t {
        virtual ~read_transaction_t() { }

        /* Gets the metadata.
        [Precondition] `this` holds the superblock
        [Postcondition] return_value.get_domain() == view->get_region()
        [May block] */
        virtual region_map_t<protocol_t, binary_blob_t> get_metadata(
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t) = 0;

        /* Performs a read.
        [Precondition] `this` holds the superblock
        [Postcondition] `this` does not hold the superblock
        [May block] */
        virtual typename protocol_t::read_response_t read(
                const typename protocol_t::read_t &,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t) = 0;

        /* Expresses the changes that have happened since `start_point` as a
        series of `backfill_chunk_t` objects.
        [Precondition] start_point.get_domain() <= view->get_region()
        [Precondition] `this` holds the superblock
        [Postcondition] `this` does not hold the superblock
        [May block] */
        virtual void send_backfill(
                const region_map_t<protocol_t, state_timestamp_t> &start_point,
                const boost::function<void(typename protocol_t::backfill_chunk_t)> &chunk_fun,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t) = 0;
    };

    /* Begins a read transaction. If there are any outstanding write
    transactions that still hold the superblock, blocks until they release the
    superblock.
    [May block] */
    virtual boost::shared_ptr<read_transaction_t> begin_read_transaction(
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) = 0;

    struct write_transaction_t : public read_transaction_t {
        virtual ~write_transaction_t() { }

        /* Replaces the metadata over the view's entire range with the given
        metadata.
        [Precondition] `this` holds the superblock
        [Precondition] new_metadata.get_domain() == view->get_region()
        [Postcondition] this->get_metadata() == new_metadata
        [May block] */
        virtual void set_metadata(
                const region_map_t<protocol_t, binary_blob_t> &new_metadata)
                THROWS_NOTHING = 0;

        /* Performs a write.
        [Precondition] `this` holds the superblock
        [Precondition] region_is_superset(view->get_region(), write.get_region())
        [Postcondition] `this` does not hold the superblock
        [May block] */
        virtual typename protocol_t::write_response_t write(
                const typename protocol_t::write_t &write,
                transition_timestamp_t timestamp)
                THROWS_NOTHING = 0;

        /* Applies a backfill data chunk sent by `send_backfill()`. If
        `interrupted_exc_t` is thrown, the state of the database is undefined
        except that doing a second backfill must put it into a valid state.
        [Precondition] `this` holds the superblock
        [Postcondition] `this` does not hold the superblock
        [May block] */
        virtual void receive_backfill(
                const typename protocol_t::backfill_chunk_t &chunk,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t) = 0;
    };

    /* Begins a write transaction. If there are any outstanding read or write
    transactions that still hold the superblock, blocks until they release the
    superblock.
    [May block] */
    virtual boost::shared_ptr<write_transaction_t> begin_write_transaction(
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) = 0;

protected:
    explicit store_view_t(typename protocol_t::region_t r) : region(r) { }

private:
    typename protocol_t::region_t region;
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
