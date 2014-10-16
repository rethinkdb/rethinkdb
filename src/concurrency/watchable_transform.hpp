// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_WATCHABLE_TRANSFORM_HPP_
#define CONCURRENCY_WATCHABLE_TRANSFORM_HPP_

#include "concurrency/auto_drainer.hpp"
#include "concurrency/watchable.hpp"
#include "concurrency/watchable_map.hpp"

/* This file contains ways of converting between `watchable_t`s and `watchable_map_t`s of
different types. These sorts of conversions may be slow, so view these types with
suspicion. */

/* `watchable_map_transform_t` presents a `watchable_map_t` whose content is some
function of the content of another `watchable_map_t`. */
template<class key1_t, class value1_t, class key2_t, class value2_t>
class watchable_map_transform_t : public watchable_map_t<key2_t, value2_t> {
public:
    explicit watchable_map_transform_t(watchable_map_t<key1_t, value1_t> *inner);

    /* `key_1_to_2` converts a key in the input watchable into the corresponding key for
    the output watchable. It can return `false` to indicate that the key should be
    excluded from the output watchable. It must be deterministic. */
    virtual bool key_1_to_2(
        const key1_t &key1,
        key2_t *key2_out) = 0;

    /* `value_1_to_2` converts a value in the input watchable into the corresponding
    value for the output watchable. The output value must be a sub-field of the input
    value; i.e. `*value2_out` should point to memory owned by `*value1`. It must be
    deterministic. */
    virtual void value_1_to_2(
        const value1_t *value1,
        const value2_t **value2_out) = 0;

    /* `key_2_to_1` is the reverse of `key_1_to_2`. It can return `false` if there is no
    key that `key_1_to_2` would map to the given key. */
    virtual bool key_2_to_1(
        const key2_t &key2,
        key1_t *key1_out) = 0;

    /* `get_all()` must copy every key and value, so beware of performance issues.
    `get_key()` will also copy the value. `read_all()` and `read_key()` don't make any
    copies of values, although they still may copy keys. */
    std::map<key2_t, value2_t> get_all();
    boost::optional<value2_t> get_key(const key2_t &key);
    void read_all(const std::function<void(const key2_t &, const value2_t *)> &);
    void read_key(const key2_t &key, const std::function<void(const value2_t *)> &);

private:
    /* This method exists to work around a bug in GCC. See GCC issue 61148. */
    void do_notify_change(const key2_t &key, const value2_t *value,
            rwi_lock_assertion_t::write_acq_t *acq) {
        watchable_map_t<key2_t, value2_t>::notify_change(key, value, acq);
    }

    rwi_lock_assertion_t *get_rwi_lock();
    watchable_map_t<key1_t, value1_t> *inner;
    typename watchable_map_t<key1_t, value1_t>::all_subs_t all_subs;
    rwi_lock_assertion_t rwi_lock;
};

/* `get_watchable_for_key()` returns a `watchable_t` that represents the value of a
specific key of a `watchable_map_t`. It must copy the value every time it changes, so
beware of performance issues. */
template<class key_t, class value_t>
clone_ptr_t<watchable_t<boost::optional<value_t> > > get_watchable_for_key(
    watchable_map_t<key_t, value_t> *,
    const key_t &key);

/* `watchable_map_entry_copier_t` is the opposite of `get_watchable_for_key()`. It
creates an entry in the given `watchable_map_var_t` for the given key using values from
the given `watchable_t`, and updates the entry every time the `watchable_t` changes. It
must copy the value every time it changes, so beware of performance issues. */
template<class key_t, class value_t>
class watchable_map_entry_copier_t {
public:
    watchable_map_entry_copier_t(
        watchable_map_var_t<key_t, value_t> *map,
        const key_t &key,
        clone_ptr_t<watchable_t<value_t> > value,
        /* If `remove_when_done` is `true`, the `watchable_map_entry_copier_t` will
        remove the entry in its destructor.
        TODO: Remove this parameter; it should always be `true`. Right now it exists to
        support one use case in `reactor_driver.cc`, and that use case should be changed
        to not use this behavior after we implement Raft. */
        bool remove_when_done = true);
    ~watchable_map_entry_copier_t();

private:
    watchable_map_var_t<key_t, value_t> *map;
    key_t key;
    clone_ptr_t<watchable_t<value_t> > value;
    bool remove_when_done;
    typename watchable_t<value_t>::subscription_t subs;
};

/* `watchable_buffer_t` merges changes to the input watchable if they occur in quick
succession, so that it generates fewer change events. This can be important for
performance. */
template<class value_t>
class watchable_buffer_t {
public:
    watchable_buffer_t(clone_ptr_t<watchable_t<value_t> > input, int64_t delay);
    clone_ptr_t<watchable_t<value_t> > get_output() {
        return output.get_watchable();
    }
private:
    void notify();
    clone_ptr_t<watchable_t<value_t> > input;
    int64_t delay;
    bool coro_running;
    watchable_variable_t<value_t> output;
    auto_drainer_t drainer;
    typename watchable_t<value_t>::subscription_t subs;
};

#include "concurrency/watchable_transform.tcc"

#endif /* CONCURRENCY_WATCHABLE_TRANSFORM_HPP_ */

