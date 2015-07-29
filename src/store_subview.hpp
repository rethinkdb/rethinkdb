#ifndef STORE_SUBVIEW_HPP_
#define STORE_SUBVIEW_HPP_

#include "store_view.hpp"

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

class store_subview_t final : public store_view_t {
public:
    /* Note that `store_subview_t` can be created and deleted on any thread, but its
    "home thread" will be set as the home thread of the underlying store. */

    store_subview_t(store_view_t *_store_view, region_t region)
        : store_view_t(region), store_view(_store_view) {
        home_thread_mixin_t::real_home_thread = store_view->home_thread();
        rassert(region_is_superset(_store_view->get_region(), region));
    }

    ~store_subview_t() {
        home_thread_mixin_t::real_home_thread = get_thread_id();
    }
    void note_reshard() {
        store_view->note_reshard();
    }

    using store_view_t::get_region;

    void new_read_token(read_token_t *token_out) {
        home_thread_mixin_t::assert_thread();
        store_view->new_read_token(token_out);
    }

    void new_write_token(write_token_t *token_out) {
        home_thread_mixin_t::assert_thread();
        store_view->new_write_token(token_out);
    }

    region_map_t<binary_blob_t> get_metainfo(
            order_token_t order_token,
            read_token_t *token,
            const region_t &region,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        rassert(region_is_superset(get_region(), region));
        return store_view->get_metainfo(order_token, token, region, interruptor);
    }

    void set_metainfo(const region_map_t<binary_blob_t> &new_metainfo,
                      order_token_t order_token,
                      write_token_t *token,
                      write_durability_t durability,
                      signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        rassert(region_is_superset(get_region(), new_metainfo.get_domain()));
        store_view->set_metainfo(
            new_metainfo, order_token, token, durability, interruptor);
    }

    void read(
            DEBUG_ONLY(const metainfo_checker_t& metainfo_checker, )
            const read_t &read,
            read_response_t *response,
            read_token_t *token,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        rassert(region_is_superset(get_region(), metainfo_checker.region));

        store_view->read(DEBUG_ONLY(metainfo_checker, ) read, response, token,
            interruptor);
    }

    void write(
            DEBUG_ONLY(const metainfo_checker_t& metainfo_checker, )
            const region_map_t<binary_blob_t>& new_metainfo,
            const write_t &write,
            write_response_t *response,
            write_durability_t durability,
            state_timestamp_t timestamp,
            order_token_t order_token,
            write_token_t *token,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        rassert(region_is_superset(get_region(), metainfo_checker.region));
        rassert(region_is_superset(get_region(), new_metainfo.get_domain()));

        store_view->write(DEBUG_ONLY(metainfo_checker, ) new_metainfo, write, response,
            durability, timestamp, order_token, token, interruptor);
    }

    continue_bool_t send_backfill_pre(
            const region_map_t<state_timestamp_t> &start_point,
            backfill_pre_item_consumer_t *pre_item_consumer,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        rassert(region_is_superset(get_region(), start_point.get_domain()));
        return store_view->send_backfill_pre(
            start_point, pre_item_consumer, interruptor);
    }

    continue_bool_t send_backfill(
            const region_map_t<state_timestamp_t> &start_point,
            size_t mem_usage_limit,
            backfill_pre_item_producer_t *pre_item_producer,
            backfill_item_consumer_t *item_consumer,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        rassert(region_is_superset(get_region(), start_point.get_domain()));
        return store_view->send_backfill(
            start_point,
            mem_usage_limit,
            pre_item_producer,
            item_consumer,
            interruptor);
    }

    continue_bool_t receive_backfill(
            const region_t &region,
            backfill_item_producer_t *item_producer,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        rassert(region_is_superset(get_region(), region));
        return store_view->receive_backfill(region, item_producer, interruptor);
    }

    void wait_until_ok_to_receive_backfill(signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
        store_view->wait_until_ok_to_receive_backfill(interruptor);
    }

    void reset_data(
            const binary_blob_t &zero_version,
            const region_t &subregion,
            write_durability_t durability,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        rassert(region_is_superset(get_region(), subregion));
        store_view->reset_data(zero_version, subregion, durability, interruptor);
    }

private:
    store_view_t *store_view;

    DISABLE_COPYING(store_subview_t);
};


#endif  // STORE_SUBVIEW_HPP_
