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
    deterministic. It's also OK to set `*value2_out` to `nullptr`, in which case it will
    not appear in the output map. */
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

/* `watchable_map_value_transform_t` is a specialized form of `watchable_map_transform_t`
that doesn't alter the keys, just the values. This can save some typing. */
template<class key_t, class value1_t, class value2_t>
class watchable_map_value_transform_t :
    public watchable_map_transform_t<key_t, value1_t, key_t, value2_t> {
public:
    watchable_map_value_transform_t(
            watchable_map_t<key_t, value1_t> *_input,
            const std::function<const value2_t *(const value1_t *)> &_fun) :
        watchable_map_transform_t<key_t, value1_t, key_t, value2_t>(_input),
        fun(_fun) { }
private:
    bool key_1_to_2(const key_t &key1, key_t *key2_out) {
        *key2_out = key1;
        return true;
    }
    bool key_2_to_1(const key_t &key2, key_t *key1_out) {
        *key1_out = key2;
        return true;
    }
    void value_1_to_2(const value1_t *value1, const value2_t **value2_out) {
        *value2_out = fun(value1);
    }
    std::function<const value2_t *(const value1_t *)> fun;
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
        clone_ptr_t<watchable_t<value_t> > value);
    ~watchable_map_entry_copier_t();

private:
    watchable_map_var_t<key_t, value_t> *map;
    key_t key;
    clone_ptr_t<watchable_t<value_t> > value;
    typename watchable_t<value_t>::subscription_t subs;
};

/* `watchable_field_copier_t` keeps some field of a `watchable_variable_t` in sync with
the value of some other `watchable_t`. */
template<class inner_t, class outer_t>
class watchable_field_copier_t {
public:
    watchable_field_copier_t(
            inner_t outer_t::*_field,
            const clone_ptr_t<watchable_t<inner_t> > &_inner,
            watchable_variable_t<outer_t> *_outer) :
        field(_field), inner(_inner), outer(_outer),
        subscription(
            [this]() {
                outer->apply_atomic_op([this](outer_t *value) {
                    value->*field = inner->get();
                    return true;
                });
            },
            inner,
            initial_call_t::YES)
        { }

private:
    inner_t outer_t::*field;
    clone_ptr_t<watchable_t<inner_t> > inner;
    watchable_variable_t<outer_t> *outer;
    typename watchable_t<inner_t>::subscription_t subscription;

    DISABLE_COPYING(watchable_field_copier_t);
};

/* `watchable_map_keyed_var_t` is like a `watchable_map_var_t`, except that the key-value
pairs are indexed by a second set of keys, different from the key-value pairs that appear
in the map. */
template<class key1_t, class key2_t, class value_t>
class watchable_map_keyed_var_t {
public:
    watchable_map_t<key2_t, value_t> *get_values() {
        return &values;
    }

    void set_key(const key1_t &key1, const key2_t &key2, const value_t &value) {
        auto key1_iter = entries.find(key1);

        bool key2_exists = false;
        values.read_key(key2, [&](const value_t *v) {
            key2_exists = (v != nullptr);
        });

        if (key1_iter != entries.end()) {
            // A `key1` entry already exists ..
            if (key1_iter->second.get_key() == key2) {
                // .. and has the right `key2`, change its value, and we're done.
                key1_iter->second.change([&](value_t *v) {
                    *v = value;
                    return true;
                });
                return;
            } else {
                // .. but does not have the right key, remove it.
                entries.erase(key1_iter);
            }
        }

        if (key2_exists) {
            // A `key2` entry already exists, find the matching `key1` which we know
            // isn't the one given, and delete it.
            delete_secondary_key(key2);
        }

        // Insert the new `key1`, `key2`, `value` entry.
        auto result = entries.insert(std::make_pair(
            key1, typename watchable_map_var_t<key2_t, value_t>::entry_t(
                &values, key2, value)));
        guarantee(result.second);
    }

    void delete_key(const key1_t &key1) {
        entries.erase(key1);
    }

private:
    void delete_secondary_key(const key2_t &key2) {
        // Iterate through all the `entries` in search of one with a key of `key2`,
        // deleting it will automatically remove it from `values` as well.
        for (auto iter = entries.begin(); iter != entries.end(); ++iter) {
            if (iter->second.get_key() == key2) {
                entries.erase(iter);
                break;
            }
        }
    }

    watchable_map_var_t<key2_t, value_t> values;
    std::map<key1_t, typename watchable_map_var_t<key2_t, value_t>::entry_t> entries;
};

/* `watchable_map_combiner_t` combines several `watchable_map_t`s into one, attaching a
tag to each one. */
template<class tag_t, class key_t, class value_t>
class watchable_map_combiner_t :
    public watchable_map_t<std::pair<tag_t, key_t>, value_t> {
public:
    class source_t {
    public:
        source_t(
            watchable_map_combiner_t *parent,
            const tag_t &tag,
            watchable_map_t<key_t, value_t> *inner);
        ~source_t();
    private:
        void on_change(const key_t &key, const value_t *value);
        watchable_map_combiner_t *parent;
        watchable_map_t<key_t, value_t> *inner;
        map_insertion_sentry_t<tag_t, watchable_map_t<key_t, value_t> *> sentry;
        typename watchable_map_t<key_t, value_t>::all_subs_t subs;
    };

    std::map<std::pair<tag_t, key_t>, value_t> get_all();
    boost::optional<value_t> get_key(const std::pair<tag_t, key_t> &key);
    void read_all(
        const std::function<void(
            const std::pair<tag_t, key_t> &, const value_t *)> &);
    void read_key(
        const std::pair<tag_t, key_t> &key,
        const std::function<void(const value_t *)> &);

private:
    rwi_lock_assertion_t *get_rwi_lock() {
        return &rwi_lock;
    }
    rwi_lock_assertion_t rwi_lock;
    std::map<tag_t, watchable_map_t<key_t, value_t> *> map;
};

#include "concurrency/watchable_transform.tcc"

#endif /* CONCURRENCY_WATCHABLE_TRANSFORM_HPP_ */

