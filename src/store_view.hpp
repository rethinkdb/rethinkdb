#ifndef STORE_VIEW_HPP_
#define STORE_VIEW_HPP_

/// @file store_view.hpp
/// @brief Abstract interface for key-value store views
///
/// Defines the store_view_t abstract class that represents a region of a key-value store.
/// Handles read/write operations, metadata, and backfill operations for a specific region.
///
/// @defgroup StoreViews Data Store Views
/// Store view abstractions for data access and management
/// @{

#include "btree/types.hpp"
#include "protocol_api.hpp"
#include "region/region_map.hpp"

class backfill_item_t;
class backfill_pre_item_t;

#ifndef NDEBUG
/// @brief Debug-only metadata validation helper
/// Verifies that metadata for a region has an expected value or pattern.
/// @note Only available in debug builds (NDEBUG not defined)
class metainfo_checker_t {
public:
    /// @brief Constructs a metadata checker with region and callback
    /// @param r The region to check metadata for
    /// @param cb Callback function invoked with (region, metainfo) pairs
    metainfo_checker_t(
            const region_t &r,
            const std::function<void(const region_t &, const binary_blob_t &)> &cb) :
        region(r), callback(cb) { }
    
    region_t region;                                                ///< Region to validate
    std::function<void(const region_t &, const binary_blob_t &)> callback;  ///< Validation callback
};

#endif  // NDEBUG

/// @brief Token representing a read lock on the store
/// Holds the lock acquired when getting in line for superblock read access.
struct read_token_t {
    /// @brief The read lock token from the FIFO enforcer
    object_buffer_t<fifo_enforcer_sink_t::exit_read_t> main_read_token;
};

/// @brief Token representing a write lock on the store
/// Holds the lock acquired when getting in line for superblock write access.
struct write_token_t {
    /// @brief The write lock token from the FIFO enforcer
    object_buffer_t<fifo_enforcer_sink_t::exit_write_t> main_write_token;
};

/// @brief Abstract interface for a region of a key-value store
///
/// `store_view_t` is an abstract class that represents a region of a key-value store
/// for some protocol. It covers some `region_t`, which is returned by `get_region()`.
///
/// In addition to the actual data, `store_view_t` is responsible for keeping track of
/// metadata which is keyed by region. The metadata is currently implemented as opaque
/// binary blob (`binary_blob_t`).
///
/// @example
/// @code
/// // Typical usage pattern:
/// store_view_t *view = get_store_view();
///
/// // Acquire tokens for operations
/// read_token_t read_token;
/// view->new_read_token(&read_token);
///
/// // Perform a read
/// read_response_t response;
/// view->read(nullptr, read_spec, &response, &read_token, &interruptor);
/// @endcode
class store_view_t : public home_thread_mixin_t {
public:
    /// @brief Virtual destructor
    virtual ~store_view_t() {
        home_thread_mixin_t::assert_thread();
    }

    /// @brief Returns the region managed by this store view
    /// Safe to call from any thread
    /// @return The region_t covered by this view
    region_t get_region() {
        return region;
    }

    /// @brief Notifies the view of a resharding operation
    /// Called when the store is resharded to a new region boundary.
    /// @param shard_region The new shard region after resharding
    virtual void note_reshard(const region_t &shard_region) = 0;

    /// @brief Allocates a new read token for lock acquisition
    /// @param token_out Pointer to receive the allocated read token
    virtual void new_read_token(read_token_t *token_out) = 0;

    /// @brief Allocates a new write token for lock acquisition
    /// @param token_out Pointer to receive the allocated write token
    virtual void new_write_token(write_token_t *token_out) = 0;

    /// @brief Retrieves metadata for a region
    /// Gets the metadata (opaque binary blob) for the specified region.
    /// @param order_token Order token for operation ordering
    /// @param token Read token for lock management
    /// @param region The region to get metadata for
    /// @param interruptor Signal to interrupt the operation
    /// @return A region_map_t mapping regions to their metadata blobs
    /// @throws interrupted_exc_t if the interruptor fires
    virtual region_map_t<binary_blob_t> get_metainfo(
            order_token_t order_token,
            read_token_t *token,
            const region_t &region,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) = 0;

    /// @brief Sets metadata for a region
    /// Replaces the metadata in the regions covered by new_metainfo.
    /// @param new_metainfo The new metadata mapping to install
    /// @param order_token Order token for operation ordering
    /// @param token Write token for lock management
    /// @param durability The durability requirement for this write
    /// @param interruptor Signal to interrupt the operation
    /// @throws interrupted_exc_t if the interruptor fires
    virtual void set_metainfo(
            const region_map_t<binary_blob_t> &new_metainfo,
            order_token_t order_token,
            write_token_t *token,
            write_durability_t durability,
            signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;

    /// @brief Performs a read operation
    /// The read's region must be a subset of the store's region.
    /// @param metainfo_expecter Debug-only metadata validation (debug builds only)
    /// @param read The read operation specification
    /// @param response Output parameter to receive the read result
    /// @param token Read token for lock management
    /// @param interruptor Signal to interrupt the operation
    /// @throws interrupted_exc_t if the interruptor fires
    virtual void read(
            DEBUG_ONLY(const metainfo_checker_t& metainfo_expecter, )
            const read_t &read,
            read_response_t *response,
            read_token_t *token,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) = 0;

    /* Performs a write. `new_metainfo`'s region must be a subset of the store's region,
    and the write's region must be a subset of `new_metainfo`'s region. */
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

    /* `send_backfill_pre()` expresses the keys that have changed since `start_point` as
    a series of `backfill_pre_item_t` objects, ignoring the values of the changed keys.
    It passes the items to `callback`. The pre-items will not overlap, and the calls to
    `on_pre_item()` are guaranteed to go in lexicographical order from left to right.
    `on_empty_range()` indicates that there won't be any pre-items between the end of the
    last pre-item and the threshold passed to `on_empty_range()`.

    If the callback returns `ABORT`, then the callback will not be called again and
    `send_backfill_pre()` will return `ABORT`. If the callback never returns `ABORT`,
    then `send_backfill_pre()` will backfill the entire range, ending with a call to
    `on_pre_item()` or `on_empty_rang()` that ends exactly on the right-hand edge of
    `start_point`. Then it will return `CONTINUE`.

    This callback is pretty much exactly the same as the corresponding type in
    `backfill.hpp`. The only reason they aren't merged is because I didn't want to
    include `backfill.hpp` from here, and because the rest of the interface is
    different. */
    class backfill_pre_item_consumer_t {
    public:
        /* It's OK for `on_pre_item()` and `on_empty_range()` to block, but they
        shouldn't block for very long, because the caller may hold B-tree locks during
        the call. */
        virtual continue_bool_t on_pre_item(
            backfill_pre_item_t &&item) THROWS_NOTHING = 0;
        virtual continue_bool_t on_empty_range(
            const key_range_t::right_bound_t &threshold) THROWS_NOTHING = 0;
    protected:
        virtual ~backfill_pre_item_consumer_t() { }
    };
    virtual continue_bool_t send_backfill_pre(
            const region_map_t<state_timestamp_t> &start_point,
            backfill_pre_item_consumer_t *pre_item_consumer,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) = 0;

    /* `send_backfill()` consumes a sequence of `backfill_pre_item_t`s and it produces a
    sequence of `backfill_item_t`s. The `backfill_item_t`s will include values for
    everything that is listed in the `backfill_pre_item_t`s and also for everything that
    changed since `start_point`. It passes the items and their associated metainfo to
    `callback`. */

    /* The semantics of `on_item()` and `on_empty_range()` are the same as for
    `store_view_t::send_backfill_pre()`. The only difference is that these functions
    don't return a `continue_bool_t`. This is because enforcing a memory limit is
    handled separately through a `backfill_item_memory_tracker_t`. The metainfo
    blob that is passed to `on_item()` and `on_empty_range()` is guaranteed to
    cover at least the region from the right-hand edge of the previous item to the
    right-hand edge of the current item; it may or may not cover a larger area as
    well. */
    class backfill_item_consumer_t {
    public:
        /* It's OK for `on_item()` and `on_empty_range()` to block, but they shouldn't
        block for very long, because the caller may hold B-tree locks while calling them.
        */
        virtual void on_item(
            const region_map_t<binary_blob_t> &metainfo,
            backfill_item_t &&item) THROWS_NOTHING = 0;
        virtual void on_empty_range(
            const region_map_t<binary_blob_t> &metainfo,
            const key_range_t::right_bound_t &threshold) THROWS_NOTHING = 0;
    protected:
        virtual ~backfill_item_consumer_t() { }
    };

    /* `send_backfill()` receives pre-items via `backfill_pre_item_producer_t`. The
    semantics are the same as in `btree_backfill_pre_item_producer_t` except for the
    addition of the `rewind()` method. `send_backfill()` will never try to rewind to a
    point to the left of the rightmost `on_item()` or `on_empty_range()` call, so it's
    safe for the `backfill_pre_item_producer_t` to discard the pre-items in response to
    calls to `on_item()` or `on_empty_range()` on the `backfill_item_consumer_t`. */
    class backfill_pre_item_producer_t {
    public:
        virtual continue_bool_t consume_range(
            key_range_t::right_bound_t *cursor_inout,
            const key_range_t::right_bound_t &limit,
            const std::function<void(const backfill_pre_item_t &)> &callback) = 0;
        virtual bool try_consume_empty_range(
            const key_range_t &range) = 0;
        virtual void rewind(const key_range_t::right_bound_t &point) = 0;
    protected:
        virtual ~backfill_pre_item_producer_t() { }
    };

    virtual continue_bool_t send_backfill(
            const region_map_t<state_timestamp_t> &start_point,
            backfill_pre_item_producer_t *pre_item_producer,
            backfill_item_consumer_t *item_consumer,
            backfill_item_memory_tracker_t *memory_tracker,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) = 0;

    /* `receive_backfill()` applies backfill item(s) generated by `send_backfill()` to
    the B-tree. It receives the items from `next_item()`, which works just like
    `next_pre_item()` on `backfill_pre_item_producer_t` except that it returns the item
    by value instead of by reference. It periodically calls `on_commit()` with increasing
    values of `threshold` to describe its progress.

    `receive_backfill()` is required to apply every item it receives from `next_item()`,
    even if `next_item()` returns `ABORT` at some point. It's also required to call
    `on_commit()` for item it applies to the B-tree by the time it returns. For example,
    if `next_item()` generates item A and then the next call to `next_item()` returns
    `ABORT`, then `receive_backfill()` would apply item A to the B-tree, then call
    `on_commit()` (*after* the call to `next_item()` that returned `ABORT`), then return
    `ABORT`.

    Durability guarantees: The data is not necessarily durable on disk when `on_commit()`
    is called, but it's definitely durable on disk by the time `receive_backfill()`
    returns. */
    class backfill_item_producer_t {
    public:
        /* These callbacks may block, but they shouldn't block for very long because
        `receive_backfill()` might hold B-tree locks while running the callbacks. */

        /* `next_item()` can generate either an item or an empty range. In the former
        case, it sets `*is_item_out` to `true`, then fills `item_out` and ignores
        `empty_range_out`; in the latter case, it does the opposite. */
        virtual continue_bool_t next_item(
            bool *is_item_out,
            backfill_item_t *item_out,
            key_range_t::right_bound_t *empty_range_out) THROWS_NOTHING = 0;

        /* Returns the metainfo corresponding to the item stream. The returned pointer
        may be invalidated if the calling coroutine yields, calls `next_item()`, or calls
        `on_commit()`. The metainfo is only guaranteed to be complete up to the
        right-hand side of the last item (or empty range) returned by `next_item()`. */
        virtual const region_map_t<binary_blob_t> *get_metainfo() THROWS_NOTHING = 0;

        virtual void on_commit(
            const key_range_t::right_bound_t &threshold) THROWS_NOTHING = 0;
    protected:
        virtual ~backfill_item_producer_t() { }
    };
    virtual continue_bool_t receive_backfill(
            const region_t &region,
            backfill_item_producer_t *item_producer,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) = 0;

    /* Blocks if a secondary index is post-constructing so it's not a good time to
    perform a backfill. */
    virtual void wait_until_ok_to_receive_backfill(signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) = 0;
    /* Like `wait_until_ok_to_receive_backfill`, but doesn't block and instead returns
    `false` if an index is post-constructing. */
    virtual bool check_ok_to_receive_backfill() THROWS_NOTHING = 0;

    /* Deletes every key in the region, and sets the metainfo for that region to
    `zero_version`. */
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
