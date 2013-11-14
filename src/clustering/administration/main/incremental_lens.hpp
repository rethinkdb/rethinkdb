// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_MAIN_INCREMENTAL_LENS_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_INCREMENTAL_LENS_HPP_

#include <map>
#include <list>

#include "errors.hpp"
#include <boost/bind.hpp>

// TODO: Document the classes
template <class key_type, class inner_type>
class change_tracking_map_t {
public:
    // Write operations
    void set_value(const key_type &key, const inner_type &value) {
        changed_keys.push_back(key);
        inner[key] = value;
    }
    void delete_value(const key_type &key) {
        changed_keys.push_back(key);
        inner.erase(key);
    }
    void clear() {
        for (auto it = inner.begin(); it != inner.end(); ++it) {
            changed_keys.push_back(it->first);
        }
        inner.clear();
    }

    const std::map<key_type, inner_type> &get_inner() const { return inner; }
    const std::list<key_type> &get_changed_peers() const { return changed_keys; }

private:
    std::map<key_type, inner_type> inner;
    std::list<key_type> changed_keys;
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
        guarantee(current != NULL);
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

#endif /* CLUSTERING_ADMINISTRATION_MAIN_INCREMENTAL_LENS_HPP_ */
