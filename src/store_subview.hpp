#ifndef STORE_SUBVIEW_HPP_
#define STORE_SUBVIEW_HPP_

/// @file store_subview.hpp
/// @brief Partial view abstraction for a store region
///
/// Provides store_subview_t, which creates a view over a subset region of an existing store.
/// Documents the consistency and ordering guarantees provided by the query routing logic.
///
/// @defgroup StoreSubviews Store Subviews and Region Partitioning
/// Partial store views with documented consistency guarantees
/// @{

#include "store_view.hpp"

/// @brief Consistency and ordering guarantees provided by RethinkDB query routing
///
/// The query-routing logic provides the following ordering guarantees:
///
/// **1. Single-key write ordering:** All replicas of each individual key will see
/// writes in the same order.
///
/// Example: Two writes to key "x": (append "a") and (append "b") sent concurrently.
/// Every copy of "x" will become either "xab" or "xba", but never "xab" on one
/// replica and "xba" on another.
///
/// **2. Origin ordering:** Queries from the same origin are performed in the order
/// they are sent.
///
/// Example: From the same thread: (set K to "b") then (read K) returns "b".
///
/// **3. Arbitrary single-key operations:** Any atomic operation on a single key
/// can be performed, as long as it can be expressed as a write_t object.
///
/// **4. No multi-key atomicity:** Other keys may be in different states.
///
/// Example: Set K1 to "b" and K2 to "b" concurrently. K1 and K2 may disagree.
///
/// Example: Set K1 to "b" from node A, then set K2 to "b" from node B. A subsequent
/// read from node C may see (K1="a", K2="b").
///
/// **5. No simple atomic multi-key transactions:** You might be able to fake it
/// by using a key as a "lock".
///
/// @see store_subview_t for creating views over subregions
class store_subview_t final : public store_view_t {
public:
    /// @brief Creates a subview over a region of an existing store
    ///
    /// Note that `store_subview_t` can be created and deleted on any thread,
    /// but its "home thread" will be set as the home thread of the underlying store.
    ///
    /// @param _store_view The underlying store view to create a subview of
    /// @param _region The region to restrict this view to (must be a subset of _store_view's region)
    /// @example
    /// @code
    /// store_view_t* full_store = get_full_store_view();
    /// region_t shard_region = compute_shard_region();
    /// store_subview_t shard_view(full_store, shard_region);
    /// // Now shard_view only sees data in shard_region
    /// @endcode
    store_subview_t(store_view_t *_store_view, region_t _region)
        : store_view_t(_region), store_view(_store_view) {
        home_thread_mixin_t::real_home_thread = store_view->home_thread();
        rassert(region_is_superset(_store_view->get_region(), _region));
    }

    /// @brief Destructor handles thread affinity cleanup
    ~store_subview_t() {
        home_thread_mixin_t::real_home_thread = get_thread_id();
    }

    /// @brief Handles store resharding events
    /// @param shard_region The new shard region (must equal this view's region)
    void note_reshard(const region_t &shard_region) {
        guarantee(get_region() == shard_region);
        store_view->note_reshard(shard_region);
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
            const region_t &_region,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        rassert(region_is_superset(get_region(), _region));
        return store_view->get_metainfo(order_token, token, _region, interruptor);
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
            const read_t &_read,
            read_response_t *response,
            read_token_t *token,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        rassert(region_is_superset(get_region(), metainfo_checker.region));

        store_view->read(DEBUG_ONLY(metainfo_checker, ) _read, response, token,
            interruptor);
    }

    void write(
            DEBUG_ONLY(const metainfo_checker_t& metainfo_checker, )
            const region_map_t<binary_blob_t>& new_metainfo,
            const write_t &_write,
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

        store_view->write(DEBUG_ONLY(metainfo_checker, ) new_metainfo, _write, response,
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
            backfill_pre_item_producer_t *pre_item_producer,
            backfill_item_consumer_t *item_consumer,
            backfill_item_memory_tracker_t *memory_tracker,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        rassert(region_is_superset(get_region(), start_point.get_domain()));
        return store_view->send_backfill(
            start_point,
            pre_item_producer,
            item_consumer,
            memory_tracker,
            interruptor);
    }

    continue_bool_t receive_backfill(
            const region_t &_region,
            backfill_item_producer_t *item_producer,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        rassert(region_is_superset(get_region(), _region));
        return store_view->receive_backfill(_region, item_producer, interruptor);
    }

    void wait_until_ok_to_receive_backfill(signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) {
        store_view->wait_until_ok_to_receive_backfill(interruptor);
    }

    bool check_ok_to_receive_backfill() THROWS_NOTHING {
        return store_view->check_ok_to_receive_backfill();
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
