// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_WATCHABLE_MAP_HPP_
#define CONCURRENCY_WATCHABLE_MAP_HPP_

#include <functional>
#include <map>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "concurrency/signal.hpp"
#include "concurrency/pubsub.hpp"
#include "containers/map_sentries.hpp"
#include "utils.hpp"

/* `watchable_map_t` is like `watchable_t` except specialized for holding a map in which
the keys usually update independently. If the map contains N keys, it takes O(log(N))
time to propagate an update for that key, as opposed to a `watchable_t<std::map>`, which
would take O(N) time.

Note that `watchable_map_t` doesn't make any guarantees about update ordering. The values
visible in the `watchable_map_t` are guaranteed to quickly converge to the correct value,
but they could do so in any order. In addition, if a value is changed and then quickly
changed back, it's possible that no notification will be sent. (The current
implementation will always deliver notifications in this case, but there's no guarantee.)
*/

template<class key_t, class value_t>
class watchable_map_t : public home_thread_mixin_t {
public:
    /* `all_subs_t` registers for notifications whenever any key in the map is inserted,
    deleted, or changed. */
    class all_subs_t {
    public:
        /* Starts watching for changes to the given map. When a change occurs to a key,
        `cb` will be called with the key and a pointer to the new value, or `nullptr` if
        the key was deleted. If `initial_call` is `true`, then `cb` will be called once
        for each key-value pair in the map at the time the `all_subs_t` is created. */
        all_subs_t(
            watchable_map_t<key_t, value_t> *map,
            const std::function<void(const key_t &, const value_t *)> &cb,
            bool initial_call = true);

    private:
        typename publisher_t<std::function<void(const key_t &, const value_t *)> >
            ::subscription_t subscription;
        DISABLE_COPYING(all_subs_t);
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
        DISABLE_COPYING(key_subs_t);
    };

    /* Returns all of the key-value pairs in the map. This needs to copy all of the keys
    and values in the map, so it's likely to be slow. */
    virtual std::map<key_t, value_t> get_all() = 0;

    /* Looks up a single key-value pair in the map. Returns the value if found, or an
    empty `boost::optional` if not. This needs to copy the value, so it's likely to be
    slow. */
    virtual boost::optional<value_t> get_key(const key_t &key) = 0;

    /* `read_all()` and `read_key()` are like `get_all()` and `get_key()`, except that
    they avoid copying. */
    virtual void read_all(
        /* This function will be called once for each key-value pair in the map. The
        `const value_t *` will always be non-NULL; it's a pointer for consistency with
        other functions in this class. */
        const std::function<void(const key_t &, const value_t *)> &) = 0;
    virtual void read_key(
        const key_t &key,
        const std::function<void(const value_t *)> &) = 0;

    /* `run_key_until_satisfied()` repeatedly calls `fun()` on a `const value_t *`
    representing the current value of `key` until `fun` returns `true` or `interruptor`
    is pulsed. It's efficient because it only retries `fun` when the value changes. */
    template<class callable_t>
    void run_key_until_satisfied(
        const key_t &key,
        const callable_t &fun,
        signal_t *interruptor);

    /* `run_all_until_satisfied()` repeatedly calls `fun()` with `this` until `fun`
    returns `true` or `interruptor` is pulsed. It's efficient because it only retries
    `fun` when a key has changed. */
    template<class callable_t>
    void run_all_until_satisfied(
        const callable_t &fun,
        signal_t *interruptor);

protected:
    watchable_map_t() { }
    virtual ~watchable_map_t() { }

    /* Subclasses should call `notify_change()` any time that they alter values. */
    void notify_change(
        const key_t &key,
        const value_t *new_value,
        rwi_lock_assertion_t::write_acq_t *acq);

    void rethread(threadnum_t new_thread) {
        home_thread_mixin_t::real_home_thread = new_thread;
        all_subs_publisher.rethread(new_thread);
    }

private:
    virtual rwi_lock_assertion_t *get_rwi_lock() = 0;

    publisher_controller_t<std::function<void(const key_t &, const value_t *)> >
        all_subs_publisher;
    std::multimap<key_t, std::function<void(const value_t *)> > key_subs_map;

    DISABLE_COPYING(watchable_map_t);
};

template<class key_t, class value_t>
class watchable_map_var_t : public watchable_map_t<key_t, value_t> {
public:
    /* `entry_t` creates a map entry in its constructor and removes it in the destructor.
    It is an alternative to `set_key()` and `delete_key()`. It crashes if an entry with
    that key already exists. Don't call `delete_key()` on the entry that this creates,
    and don't call `set_all()` while this exists. */
    class entry_t {
    public:
        entry_t() : parent(nullptr) { }
        entry_t(watchable_map_var_t *parent, const key_t &key, const value_t &value);
        entry_t(const entry_t &) = delete;
        entry_t(entry_t &&);
        ~entry_t();
        entry_t &operator=(const entry_t &) = delete;
        entry_t &operator=(entry_t &&);

        key_t get_key();
        void change(const std::function<bool(value_t *value)> &callback);

    private:
        watchable_map_var_t *parent;
        typename std::map<key_t, value_t>::iterator iterator;
    };

    watchable_map_var_t() { }
    explicit watchable_map_var_t(std::map<key_t, value_t> &&source);

    std::map<key_t, value_t> get_all();
    boost::optional<value_t> get_key(const key_t &key);
    void read_all(const std::function<void(const key_t &, const value_t *)> &);
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

    /* `change_key()` atomically modifies the value of `key`. It calls the callback
    exactly once, with two parameters `exists` and `value`. If the key is present,
    `*exists` will be `true` and `*value` will be its value; if the key is absent, then
    `*exists` will be `false` and `*value` will be a valid buffer but with undefined
    contents. The callback can modify `*exists` and/or `*value`. It must return `true` if
    it makes any changes. these changes will be reflected in the `watchable_map_t`. The
    callback must not block or call any other methods of the `watchable_map_var_t`. */
    void change_key(
        const key_t &key,
        const std::function<bool(bool *exists, value_t *value)> &callback);

    void rethread(threadnum_t new_thread) {
        watchable_map_t<key_t, value_t>::rethread(new_thread);
        rwi_lock.rethread(new_thread);
    }

private:
    rwi_lock_assertion_t *get_rwi_lock() {
        return &rwi_lock;
    }

    std::map<key_t, value_t> map;
    rwi_lock_assertion_t rwi_lock;
};

#include "concurrency/watchable_map.tcc"

#endif   /* CONCURRENCY_WATCHABLE_MAP_HPP_ */

