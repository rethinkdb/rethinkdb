#ifndef STORE_VIEW_HPP_
#define STORE_VIEW_HPP_

#include "protocol_api.hpp"
#include "region/region_map.hpp"

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
        assert_thread();
    }

    region_t get_region() {
        /* Safe to call on any thread */
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

    /* `send_backfill_pre()` expresses the keys that have changed since `start_point` as
    a series of `backfill_pre_atom_t` objects, ignoring the values of the changed keys.
    It passes the atoms to `callback`, aborting if `callback` returns `false`.
    [Precondition] region_is_superset(view->get_region(), start_point.get_domain())
    [Return value] `true` if finished, `false` if `callback` aborted the backfill
    [May block]
    */
    class send_backfill_pre_callback_t {
    public:
        /* For each range, `on_pre_atom()` will be called zero or more times, and then
        `on_done_range()` will be called. The ranges will be processed in lexicographical
        order and will be contiguous.

        If `on_pre_atom()` returns false, then `on_done_range()` will still be called one
        last time. */
        virtual bool on_pre_atom(backfill_pre_atom_t &&atom) = 0;
        virtual void on_done_range(const key_range_t &range) = 0;
    };
    virtual bool send_backfill_pre(
            const region_map_t<state_timestamp_t> &start_point,
            backfill_pre_callback_t *callback,
            signal_t *interruptor) = 0;

    /* `send_backfill()` expresses the changes that have happened since `start_point` as
    a series of `backfill_atom_t` objects. It also includes new values for any keys
    listed in `pre_atoms`. It passes the atoms to `callback`, aborting if `callback`
    returns `false`. It also includes the value of the metainfo with each group of atoms.
    [Precondition] region_is_superset(view->get_region(), start_point.get_domain())
    [Return value] `true` if finished, `false` if `callback` aborted the backfill
    [May block]
    */
    class send_backfill_callback_t {
    public:
        /* For each range, `on_atom()` will be called zero or more times, and then
        `on_done_range()` will be called; its parameter will be the metainfo for the
        atoms that were just provided for this range. The ranges will be processed in
        lexicographical order and will be contiguous. If `on_atom()` returns `false`,
        then `on_done_range()` will still be called one last time. */
        virtual bool on_atom(backfill_atom_t &&atom) = 0;
        virtual void on_done_range(const region_map_t<binary_blob_t> &metainfo) = 0;

        /* `next_pre_atom()` should set `*pre_atom_out` to the next pre atom which is at
        least partially contained in `range`, or `nullptr` if there are no more pre atoms
        in that range. It should return `false` if the next pre atom is not available
        yet; this has the same effect as returning `false` from `on_atom()`. */
        virtual bool next_pre_atom(
            const key_range_t &range,
            backfill_pre_atom_t const **next_out) = 0;
    };
    virtual bool send_backfill(
            const region_map_t<state_timestamp_t> &start_point,
            const std::deque<backfill_pre_atom_t> &pre_atoms,
            backfill_callback_t *callback,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) = 0;

    /* Applies backfill atom(s) sent by `send_backfill()`. The `new_metainfo` is applied
    atomically as the backfill atoms are applied.
    [Precondition] The store's current state is as described by the `start_point` and
        `backfill_pre_atom_t`s given to `send_backfill()`.
    [Precondition] region_is_superset(view->get_region(), new_metainfo.get_region())
    [Precondition] region_is_superset(new_metainfo.get_region(), chunk[n].get_region())
    [Precondition] The atoms in `chunk` are in lexicographical order.
    [Postcondition] For any key, the atom for that key was applied iff the metainfo was
        changed for that key.
    [Return value] `true` if we reached the end; `false` if `callback()` returned `false`
    [May block]
    */
    virtual void receive_backfill(
            const region_map_t<binary_blob_t> &new_metainfo,
            const std::deque<backfill_atom_t> &chunk,
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
    [May block] */
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
