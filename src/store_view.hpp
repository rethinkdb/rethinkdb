#ifndef STORE_VIEW_HPP_
#define STORE_VIEW_HPP_

#include "debug.hpp"

/* `store_view_t` is an abstract class that represents a region of a key-value store
for some protocol.  It covers some `region_t`, which is returned by `get_region()`.

In addition to the actual data, `store_view_t` is responsible for keeping track of
metadata which is keyed by region. The metadata is currently implemented as opaque
binary blob (`binary_blob_t`).
*/

class store_view_t : public home_thread_mixin_t {
public:
    virtual ~store_view_t() {
        assert_thread();
    }

    region_t get_region() {
        return region;
    }

    virtual void note_reshard() = 0;

    virtual void new_read_token(read_token_t *token_out) = 0;
    virtual void new_write_token(write_token_t *token_out) = 0;

    /* Gets the metainfo.
    [Postcondition] return_value.get_domain() == view->get_region()
    [May block] */
    virtual void do_get_metainfo(order_token_t order_token,
                                 read_token_t *token,
                                 signal_t *interruptor,
                                 region_map_t<binary_blob_t> *out)
        THROWS_ONLY(interrupted_exc_t) = 0;

    /* Replaces the metainfo over the view's entire range with the given metainfo.
    [Precondition] region_is_superset(view->get_region(), new_metainfo.get_domain())
    [Postcondition] this->get_metainfo() == new_metainfo
    [May block] */
    virtual void set_metainfo(const region_map_t<binary_blob_t> &new_metainfo,
                              order_token_t order_token,
                              write_token_t *token,
                              signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;

    /* Performs a read.
    [Precondition] region_is_superset(view->get_region(), expected_metainfo.get_domain())
    [Precondition] region_is_superset(expected_metainfo.get_domain(), read.get_region())
    [May block] */
    virtual void read(
            DEBUG_ONLY(const metainfo_checker_t& metainfo_expecter, )
            const read_t &read,
            read_response_t *response,
            order_token_t order_token,
            read_token_t *token,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) = 0;


    /* Performs a write.
    [Precondition] region_is_superset(view->get_region(), expected_metainfo.get_domain())
    [Precondition] new_metainfo.get_domain() == expected_metainfo.get_domain()
    [Precondition] region_is_superset(expected_metainfo.get_domain(), write.get_region())
    [May block] */
    virtual void write(
            DEBUG_ONLY(const metainfo_checker_t& metainfo_expecter, )
            const region_map_t<binary_blob_t> &new_metainfo,
            const write_t &write,
            write_response_t *response,
            write_durability_t durability,
            state_timestamp_t timestamp,
            order_token_t order_token,
            write_token_t *token,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) = 0;

    /* Expresses the changes that have happened since `start_point` as a
    series of `backfill_chunk_t` objects.
    [Precondition] start_point.get_domain() <= view->get_region()
    [Side-effect] `should_backfill` must be called exactly once
    [Return value] Value equal to the value returned by should_backfill
    [May block]
    */
    virtual bool send_backfill(
            const region_map_t<state_timestamp_t> &start_point,
            send_backfill_callback_t *send_backfill_cb,
            traversal_progress_combiner_t *progress,
            read_token_t *token,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) = 0;


    /* Applies a backfill data chunk sent by `send_backfill()`. If
    `interrupted_exc_t` is thrown, the state of the database is undefined
    except that doing a second backfill must put it into a valid state.
    [May block]
    */
    virtual void receive_backfill(
            const backfill_chunk_t &chunk,
            write_token_t *token,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) = 0;

    /* Throttles an individual backfill chunk. Preserves ordering.
    [May block] */
    virtual void throttle_backfill_chunk(signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) = 0;

    /* Deletes every key in the region, and sets the metainfo for that region to
    `zero_version`.
    [Precondition] region_is_superset(region, subregion)
    [May block]
     */
    virtual void reset_data(
            const binary_blob_t &zero_version,
            const region_t &subregion,
            write_durability_t durability,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) = 0;

protected:
    explicit store_view_t(region_t r) : region(r) { }

private:
    const region_t region;

    DISABLE_COPYING(store_view_t);
};


#endif  // STORE_VIEW_HPP_
