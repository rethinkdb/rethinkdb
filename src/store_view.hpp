#ifndef STORE_VIEW_HPP_
#define STORE_VIEW_HPP_

#include "protocol_api.hpp"
#include "region/region_map.hpp"

class backfill_item_t;
class backfill_pre_item_t;

#ifndef NDEBUG
// Checks that the metainfo has a certain value, or certain kind of value.
class metainfo_checker_callback_t {
public:
    virtual void check_metainfo(const region_map_t<binary_blob_t>& metainfo,
                                const region_t& domain) const = 0;
protected:
    metainfo_checker_callback_t() { }
    virtual ~metainfo_checker_callback_t() { }
private:
    DISABLE_COPYING(metainfo_checker_callback_t);
};

struct trivial_metainfo_checker_callback_t : public metainfo_checker_callback_t {

    trivial_metainfo_checker_callback_t() { }
    void check_metainfo(UNUSED const region_map_t<binary_blob_t>& metainfo, UNUSED const region_t& region) const {
        /* do nothing */
    }

private:
    DISABLE_COPYING(trivial_metainfo_checker_callback_t);
};

struct equality_metainfo_checker_callback_t : public metainfo_checker_callback_t {
    explicit equality_metainfo_checker_callback_t(const binary_blob_t& expected_value)
        : value_(expected_value) { }

    void check_metainfo(const region_map_t<binary_blob_t>& metainfo, const region_t& region) const {
        region_map_t<binary_blob_t> masked = metainfo.mask(region);

        for (region_map_t<binary_blob_t>::const_iterator it = masked.begin(); it != masked.end(); ++it) {
            rassert(it->second == value_);
        }
    }

private:
    const binary_blob_t value_;

    DISABLE_COPYING(equality_metainfo_checker_callback_t);
};

class metainfo_checker_t {
public:
    metainfo_checker_t(const metainfo_checker_callback_t *callback,
                       const region_t& region) : callback_(callback), region_(region) { }

    void check_metainfo(const region_map_t<binary_blob_t>& metainfo) const {
        callback_->check_metainfo(metainfo, region_);
    }
    const region_t& get_domain() const { return region_; }
    const metainfo_checker_t mask(const region_t& region) const {
        return metainfo_checker_t(callback_, region_intersection(region, region_));
    }

private:
    const metainfo_checker_callback_t *const callback_;
    const region_t region_;

    // This _is_ copyable because of mask, but all copies' lifetimes
    // are limited by that of callback_.
};

#endif  // NDEBUG

/* {read,write}_token_t hold the lock held when getting in line for the
   superblock. */
struct read_token_t {
    object_buffer_t<fifo_enforcer_sink_t::exit_read_t> main_read_token;
};

struct write_token_t {
    object_buffer_t<fifo_enforcer_sink_t::exit_write_t> main_write_token;
};

/* `store_view_t` is an abstract class that represents a region of a key-value store
for some protocol.  It covers some `region_t`, which is returned by `get_region()`.

In addition to the actual data, `store_view_t` is responsible for keeping track of
metadata which is keyed by region. The metadata is currently implemented as opaque
binary blob (`binary_blob_t`).
*/

class store_view_t : public home_thread_mixin_t {
public:
    virtual ~store_view_t() {
        home_thread_mixin_t::assert_thread();
    }

    region_t get_region() {
        /* Safe to call on any thread */
        return region;
    }

    virtual void note_reshard() = 0;

    virtual void new_read_token(read_token_t *token_out) = 0;
    virtual void new_write_token(write_token_t *token_out) = 0;

    /* Gets the metainfo. The return value will cover the same region as `get_region()`.
    */
    virtual void do_get_metainfo(order_token_t order_token,
                                 read_token_t *token,
                                 signal_t *interruptor,
                                 region_map_t<binary_blob_t> *out)
        THROWS_ONLY(interrupted_exc_t) = 0;

    /* Replaces the metainfo over the view's entire range with the given metainfo. */
    virtual void set_metainfo(const region_map_t<binary_blob_t> &new_metainfo,
                              order_token_t order_token,
                              write_token_t *token,
                              signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;

    /* Performs a read. The read's region must be a subset of the store's region. */
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
    It passes the items to `callback`, aborting if `on_pre_item()` returns `false`. */
    class backfill_pre_item_consumer_t {
    public:
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
    class backfill_item_consumer_t {
    public:
        virtual continue_bool_t on_item(
            const region_map_t<binary_blob_t> &metainfo,
            backfill_item_t &&item) THROWS_NOTHING = 0;
        virtual continue_bool_t on_empty_range(
            const region_map_t<binary_blob_t> &metainfo,
            const key_range_t::right_bound_t &threshold) THROWS_NOTHING = 0;
    protected:
        virtual ~backfill_item_consumer_t() { }
    };
    class backfill_pre_item_producer_t {
    public:
        virtual continue_bool_t next_pre_item(
            backfill_pre_item_t const **next_out,
            key_range_t::right_bound_t *edge_out) THROWS_NOTHING = 0;

        /* The pointer given to `next_pre_item()` should remain valid until
        `release_pre_item()` is called. Every call to `next_pre_item()` that returns
        sets `*next_out` to a non-null value will be followed by a call to
        `next_pre_item()`. */
        virtual void release_pre_item() THROWS_NOTHING = 0;
    protected:
        virtual ~backfill_pre_item_producer_t() { }
    };
    virtual continue_bool_t send_backfill(
            const region_map_t<state_timestamp_t> &start_point,
            backfill_pre_item_producer_t *pre_item_producer,
            backfill_item_consumer_t *item_consumer,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) = 0;

    /* Applies backfill item(s) sent by `send_backfill()`. The `new_metainfo` is applied
    atomically as the backfill items are applied. `receive_backfill()` continues until
    `next_item()` returns `ABORT` or it calls `on_commit()` with the right-hand side of
    the region. */
    class backfill_item_producer_t {
    public:
        virtual continue_bool_t next_item(
            bool *is_item_out,
            backfill_item_t *item_out,
            key_range_t::right_bound_t *edge_out) THROWS_NOTHING = 0;

        /* Returns the metainfo corresponding to the item stream. The returned pointer
        may be invalidated if the calling coroutine yields, calls `next_item()`, or calls
        `on_commit()`. The metainfo is only guaranteed to be complete up to the
        right-hand side of the last item (or edge) returned by `next_item()`. */
        virtual const region_map_t<binary_blob_t> *get_metainfo() THROWS_NOTHING = 0;

        /* By the time `receive_backfill()` returns, the store must have called
        `on_commit()` with a threshold greater than or equal to the last item returned
        from `next_item()`. */
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
