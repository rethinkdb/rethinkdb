// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_WATCHABLE_MAP_HPP_
#define CONCURRENCY_WATCHABLE_MAP_HPP_

/* `watchable_map_t` is like `watchable_t` except specialized for holding a map in which
the keys usually update independently. If the map contains N keys, it takes O(log(N))
time to propagate an update for that key, as opposed to a `watchable_t<std::map>`, which
would take O(N) time.

`watchable_map_t` also makes some design decisions differently than `watchable_t`. In
particular, `watchable_map_t` doesn't support subviews. (They were a bad idea in the
first place.) */

template<class key_t, class value_t>
class watchable_map_t : public home_thread_mixin_t {
public:
    /* `all_subs_t` registers for notifications whenever any key in the map is inserted,
    deleted, or changed. */
    class all_subs_t {
    public:
        /* Starts watching for changes to the given map. When a change occurs to a key,
        `cb` will be called with the key and a pointer to the new value, or `nullptr` if
        the key was deleted. If `initial_cb` is provided, the `all_subs_t` constructor
        will call it with the value of the map at the time that the subscription starts.
        This can be used to read the current value and then subscribe to new changes in a
        way that is safe from race conditions. */
        all_subs_t(
            watchable_map_t<key_t, value_t> *map,
            const std::function<void(const key_t &key, const value_t *maybe_value)> &cb,
            const std::function<void(const std::map<key_t, value_t> &)> &initial_cb);

    private:
        typename publisher_t<std::function<void(const key_t &, const value_t *)> >
            ::subscription_t subscription;
        DISABLE_COPYING(watchable_map_all_subs_t);
    };

    /* `key_subs_t` registers for notifications whenever a specific key in the map
    is inserted, deleted, or changed. */
    class key_subs_t {
    public:
        /* Starts watching for changes to the given key in the given map. When `key` is
        inserted, changed, or deleted in `map`, `cb` will be called with a pointer to the
        new value, or `nullptr` if `key` was deleted. If `initial_call` is `true`, then
        the constructor will call `cb` once with the initial value of `key`. */
        key_subs_t(
            watchable_map_t<key_t, value_t> *map,
            const key_t &key,
            const std::function<void(const value_t *maybe_value)> &cb,
            bool initial_call = true);

    private:
        multimap_insertion_sentry_t<key_t, std::function<void(const value_t *)> > sentry;
        DISABLE_COPYING(watchable_map_key_subs_t);
    };

    /* Returns all of the key-value pairs in the map. */
    virtual std::map<key_t, value_t> get_all() = 0;

    /* Looks up a single key-value pair in the map. Returns the value if found, or an
    empty `boost::optional` if not. */
    virtual boost::optional<value_t> get_key(const key_t &key) = 0;

    /* `read_all()` and `read_key()` are like `get_all()` and `get_key()`, except that
    they avoid copying. */
    virtual void read_all(
        const std::function<void(const std::map<key_t, value_t> &)> &) = 0;
    virtual void read_key(
        const key_t &key,
        const std::function<void(const value_t *)> &) = 0;

protected:
    /* Subclasses should call `notify_change()` any time that they alter values. */
    void notify_change(
        const key_t &key,
        const value_t *new_value,
        rwi_lock_assertion_t::write_acq_t *write_acq);

private:
    publisher_controller_t<std::function<void(const key_t &, const value_t *)> >
        all_subs_publisher;
    std::multimap<key_t, std::function<void(const value_t *)> > key_subs_map;

    DISABLE_COPYING(watchable_map_t);
};

template<class key_t, class value_t>
class watchable_map_var_t : public watchable_map_t<key_t, value_t> {
public:
    std::map<key_t, value_t> get_all();
    boost::optional<value_t> get_key(const key_t &key);
    void read_all(const std::function<void(const std::map<key_t, value_t> &)> &);
    void read_key(const key_t &key, const std::function<void(const value_t *)> &);

    /* `set_all()` removes all of the key-value pairs from the map and replaces them with
    values from `new_value`. */
    void set_all(const std::map<key_t, value_t> &new_value);

    /* `set_key()` sets the value of `key` to `new_value`. If `key` was not in the
    map before, it will be inserted. As an optimization, `set_key()` compares the
    previous value of `key` to `new_value`; if they compare equal, it will not notify
    subscriptions. If `value` does not support equality comparisons, use
    `set_key_no_equals()`. */
    void set_key(const key_t &key, const value_t &new_value);
    void set_key_no_equals(const key_t &key, const value_t &new_value);

    /* `delete_key()` removes `key` from the map if it was present before. */
    void delete_key(const key_t &key);

private:
    std::map<key_t, value_t> map;
    rwi_lock_assertion_t rwi_lock;
};

#include "concurrency/watchable_map.tcc"

#endif   /* CONCURRENCY_WATCHABLE_MAP_HPP_ */

