// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONTAINERS_INCREMENTAL_LENSES_HPP_
#define CONTAINERS_INCREMENTAL_LENSES_HPP_

#include <map>
#include <set>

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/utility/result_of.hpp>

// TODO: Document the classes
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

    const std::map<key_type, inner_type> &get_inner() const { return inner; }
    const std::set<key_type> &get_changed_keys() const { return changed_keys; }
    unsigned int get_current_version() const { return current_version; }

    // TODO (daniel): Remove this, unless we really need it.
    static std::map<key_type, inner_type> inner_extractor(
        const change_tracking_map_t<key_type, inner_type> &ctm) {

        return ctm.get_inner();
    }

private:
    std::map<key_type, inner_type> inner;
    std::set<key_type> changed_keys;
    unsigned int current_version;
};

template <class key_type, class inner_type, class callable_type>
class incremental_map_lens_t {
public:
    typedef typename boost::result_of<callable_type(inner_type)>::type inner_result_type;
    typedef change_tracking_map_t<key_type, inner_result_type> result_type;

    explicit incremental_map_lens_t(const callable_type &_inner_lens) :
        inner_lens(_inner_lens) {
    }

    void operator()(const change_tracking_map_t<key_type, inner_type> &input,
                    result_type *current_out) {
        guarantee(current_out != NULL);
        // Begin a new version and verify that we are in sync
        current_out->begin_version();
        /* Right now we expect that we are called on the output exactly once after
         * each change to the input and that we do not miss any version.
         * If this should change in the future, we could instead of crashing here
         * just recompute the whole output to get back in sync. That would
         * destroy our performance guarantees though, and should be a rare process.
         */
        guarantee(current_out->get_current_version() == input.get_current_version(),
            "incremental_map_lens_t got out of sync with input. "
            "Our current version: %u, Input version: %u",
            current_out->get_current_version(),
            input.get_current_version());
        // Update changed peers only
        for (auto it = input.get_changed_keys().begin(); it != input.get_changed_keys().end(); ++it) {
            auto input_value_it = input.get_inner().find(*it);
            if (input_value_it == input.get_inner().end()) {
                // The peer was deleted
                current_out->delete_value(*it);
            } else {
                // The peer was added or changed, (re-)compute it.
                current_out->set_value(*it, inner_lens(input_value_it->second));
            }
        }
    }

private:
    callable_type inner_lens;
};

#endif /* CONTAINERS_INCREMENTAL_LENSES_HPP_ */
