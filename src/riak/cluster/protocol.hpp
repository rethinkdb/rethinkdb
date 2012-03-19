#ifndef RIAK_PROTOCOL_HPP_
#define RIAK_PROTOCOL_HPP_

#include "riak/cluster/read.hpp"
#include "riak/cluster/region.hpp"
#include "riak/cluster/write.hpp"
#include "utils.hpp"

class order_token_t;
class signal_t;
namespace boost { template <typename Signature> class function; }
namespace riak { class riak_interface_t; }

namespace riak {


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

class protocol_t {
    typedef riak::region_t region_t;
};

} //namespace riak 

#endif 
