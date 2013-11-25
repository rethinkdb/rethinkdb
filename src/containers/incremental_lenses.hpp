// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONTAINERS_INCREMENTAL_LENSES_HPP_
#define CONTAINERS_INCREMENTAL_LENSES_HPP_

#include <map>
#include <set>

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/utility/result_of.hpp>

/* `change_tracking_map_t` is like an `std::map`. However it keeps track
 * of which keys have been modified.
 * You can get access to the map's data through `get_inner()`.
 * Before changing any of the values, you must call `begin_version()`.
 * This resets the set of changed keys. It is therefore important that whatever
 * processes the changes to the change_tracking_map is called once per call to
 * begin_version, because it could otherwise miss some changes.
 */
template <class key_type, class inner_type>
class change_tracking_map_t {
public:
    change_tracking_map_t() : current_version(0) { }

    // Write operations
    void begin_version() {
        changed_keys.clear();
        ++current_version;
    }
    void set_value(const key_type &key, const inner_type &value) {
        rassert (current_version > 0, "You must call begin_version() before "
            "performing any changes.");
        changed_keys.insert(key);
        inner[key] = value;
    }
    // Same as above but with move semantics
    void set_value(const key_type &key, inner_type &&value) {
        rassert (current_version > 0, "You must call begin_version() before "
            "performing any changes.");
        changed_keys.insert(key);
        inner[key] = std::move(value);
    }
    void delete_value(const key_type &key) {
        rassert (current_version > 0, "You must call begin_version() before "
            "performing any changes.");
        changed_keys.insert(key);
        inner.erase(key);
    }
    void clear() {
        rassert (current_version > 0, "You must call begin_version() before "
            "performing any changes.");
        for (auto it = inner.begin(); it != inner.end(); ++it) {
            changed_keys.insert(it->first);
        }
        inner.clear();
    }

    // Getters
    const std::map<key_type, inner_type> &get_inner() const { return inner; }
    const std::set<key_type> &get_changed_keys() const { return changed_keys; }
    unsigned int get_current_version() const { return current_version; }

    // Operators
    bool operator==(const change_tracking_map_t &o) const {
        return o.current_version == current_version
            && o.changed_keys == changed_keys
            && o.inner == inner;
    }
    bool operator!=(const change_tracking_map_t &o) const {
        return !(o == *this);
    }

private:
    std::map<key_type, inner_type> inner;
    std::set<key_type> changed_keys;
    unsigned int current_version;
};


/* Given an `inner_lens`, the `incremental_map_lens_t` maintains a mapped
 * version of a change_tracking_map, where each value v_out of the output is
 * inner_lens(v_in). It tries to reduce computational costs by only applying
 * inner_lens to values in the input map that are marked as changed.
 *
 * `incremental_map_lens_t` is designed for being used in the context of a
 * `subview_watchable_t`.
 */
template <class key_type, class inner_type, class callable_type>
class incremental_map_lens_t {
public:
    typedef typename boost::result_of<callable_type(inner_type)>::type inner_result_type;
    typedef change_tracking_map_t<key_type, inner_result_type> result_type;

    explicit incremental_map_lens_t(const callable_type &_inner_lens) :
        inner_lens(_inner_lens) {
    }

    bool operator()(const change_tracking_map_t<key_type, inner_type> &input,
                    result_type *current_out) {
        guarantee(current_out != NULL);

        bool anything_changed = false;

        const bool do_init = current_out->get_current_version() == 0;
        // Begin a new version
        current_out->begin_version();

        if (do_init) {
            // Compute all values
            for (auto it = input.get_inner().begin(); it != input.get_inner().end(); ++it) {
                current_out->set_value(it->first, inner_lens(it->second));
            }
            anything_changed = true;
        } else {
            // Update changed values only
            for (auto it = input.get_changed_keys().begin(); it != input.get_changed_keys().end(); ++it) {
                auto input_value_it = input.get_inner().find(*it);
                auto existing_it = current_out->get_inner().find(*it);
                if (input_value_it == input.get_inner().end()) {
                    // The value was deleted
                    if (existing_it != current_out->get_inner().end()) {
                        current_out->delete_value(*it);
                        anything_changed = true;
                    }
                } else {
                    // This is to determine if the value has changed or not
                    inner_result_type old_value;
                    bool has_old_value = false;p
                    if (existing_it == current_out->get_inner().end()) {
                        // New value
                        anything_changed = true;
                    } else if (!anything_changed) {
                        // Changed value. We must check for changes later. Keep a copy
                        // of the old value around.
                        // We can use move here because we are going to overwrite it anyways
                        old_value = std::move(existing_it->second);
                        has_old_value = true;
                    }

                    // The value was added or changed, (re-)compute it.
                    current_out->set_value(*it, inner_lens(input_value_it->second));

                    if (has_old_value) {
                        if (!(old_value == current_out->get_inner().find(*it)->second)) {
                            anything_changed = true;
                        }
                    }
                }
            }
        }
        return anything_changed;
    }

private:
    callable_type inner_lens;
};

#endif /* CONTAINERS_INCREMENTAL_LENSES_HPP_ */
