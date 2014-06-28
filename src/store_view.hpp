#ifndef STORE_VIEW_HPP_
#define STORE_VIEW_HPP_

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

    virtual void new_read_token(object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token_out) = 0;
    virtual void new_write_token(object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token_out) = 0;

    void new_read_token_pair(read_token_pair_t *token_pair_out) {
        new_read_token(&token_pair_out->main_read_token);
    }
    void new_write_token_pair(write_token_pair_t *token_pair_out) {
        new_write_token(&token_pair_out->main_write_token);
    }

    /* Gets the metainfo.
    [Postcondition] return_value.get_domain() == view->get_region()
    [May block] */
    virtual void do_get_metainfo(order_token_t order_token,
                                 object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token,
                                 signal_t *interruptor,
                                 region_map_t<binary_blob_t> *out)
        THROWS_ONLY(interrupted_exc_t) = 0;

    /* Replaces the metainfo over the view's entire range with the given metainfo.
    [Precondition] region_is_superset(view->get_region(), new_metainfo.get_domain())
    [Postcondition] this->get_metainfo() == new_metainfo
    [May block] */
    virtual void set_metainfo(const region_map_t<binary_blob_t> &new_metainfo,
                              order_token_t order_token,
                              object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token,
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
            read_token_pair_t *token,
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
            transition_timestamp_t timestamp,
            order_token_t order_token,
            write_token_pair_t *token,
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
            read_token_pair_t *token_pair,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) = 0;


    /* Applies a backfill data chunk sent by `send_backfill()`. If
    `interrupted_exc_t` is thrown, the state of the database is undefined
    except that doing a second backfill must put it into a valid state.
    [May block]
    */
    virtual void receive_backfill(
            const backfill_chunk_t &chunk,
            write_token_pair_t *token,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) = 0;

    /* Throttles an individual backfill chunk. Preserves ordering.
    [May block] */
    virtual void throttle_backfill_chunk(signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) = 0;

    /* Deletes every key in the region.
    [Precondition] region_is_superset(region, subregion)
    [May block]
     */
    virtual void reset_data(
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
    be expressed as `write_t` objects.

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

#include "debug.hpp"

class store_subview_t : public store_view_t
{
public:
    store_subview_t(store_view_t *_store_view, region_t region)
        : store_view_t(region), store_view(_store_view) {
        rassert(region_is_superset(_store_view->get_region(), region));
    }

    ~store_subview_t() {
        store_view->note_reshard();
    }
    void note_reshard() {
        store_view->note_reshard();
    }

    using store_view_t::get_region;

    void new_read_token(object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token_out) {
        home_thread_mixin_t::assert_thread();
        store_view->new_read_token(token_out);
    }

    void new_write_token(object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token_out) {
        home_thread_mixin_t::assert_thread();
        store_view->new_write_token(token_out);
    }

    void do_get_metainfo(order_token_t order_token,
                         object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token,
                         signal_t *interruptor,
                         region_map_t<binary_blob_t> *out)
            THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        region_map_t<binary_blob_t> tmp;
        store_view->do_get_metainfo(order_token, token, interruptor, &tmp);
        *out = tmp.mask(get_region());
    }

    void set_metainfo(const region_map_t<binary_blob_t> &new_metainfo,
                      order_token_t order_token,
                      object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token,
                      signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        rassert(region_is_superset(get_region(), new_metainfo.get_domain()));
        store_view->set_metainfo(new_metainfo, order_token, token, interruptor);
    }

    void read(
            DEBUG_ONLY(const metainfo_checker_t& metainfo_checker, )
            const read_t &read,
            read_response_t *response,
            order_token_t order_token,
            read_token_pair_t *token_pair,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        rassert(region_is_superset(get_region(), metainfo_checker.get_domain()));

        store_view->read(DEBUG_ONLY(metainfo_checker, ) read, response, order_token, token_pair, interruptor);
    }

    void write(
            DEBUG_ONLY(const metainfo_checker_t& metainfo_checker, )
            const region_map_t<binary_blob_t>& new_metainfo,
            const write_t &write,
            write_response_t *response,
            write_durability_t durability,
            transition_timestamp_t timestamp,
            order_token_t order_token,
            write_token_pair_t *token_pair,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        rassert(region_is_superset(get_region(), metainfo_checker.get_domain()));
        rassert(region_is_superset(get_region(), new_metainfo.get_domain()));

        store_view->write(DEBUG_ONLY(metainfo_checker, ) new_metainfo, write, response, durability, timestamp, order_token, token_pair, interruptor);
    }

    bool send_backfill(
            const region_map_t<state_timestamp_t> &start_point,
            send_backfill_callback_t *send_backfill_cb,
            traversal_progress_combiner_t *p,
            read_token_pair_t *token_pair,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        rassert(region_is_superset(get_region(), start_point.get_domain()));

        return store_view->send_backfill(start_point, send_backfill_cb, p, token_pair, interruptor);
    }

    void receive_backfill(
            const backfill_chunk_t &chunk,
            write_token_pair_t *token_pair,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        store_view->receive_backfill(chunk, token_pair, interruptor);
    }

    void throttle_backfill_chunk(signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        store_view->throttle_backfill_chunk(interruptor);
    }

    void reset_data(
            const region_t &subregion,
            write_durability_t durability,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        rassert(region_is_superset(get_region(), subregion));

        store_view->reset_data(subregion, durability, interruptor);
    }

private:
    store_view_t *store_view;

    DISABLE_COPYING(store_subview_t);
};

#endif  // STORE_VIEW_HPP_
