// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_MAIN_WATCHABLE_FIELDS_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_WATCHABLE_FIELDS_HPP_

#include <map>

#include "concurrency/watchable.hpp"
#include "containers/incremental_lenses.hpp"
#include "rpc/connectivity/peer_id.hpp"

template<class inner_t, class outer_t>
class field_copier_t {
public:
    field_copier_t(inner_t outer_t::*_field, const clone_ptr_t<watchable_t<inner_t> > &_inner, watchable_variable_t<outer_t> *_outer) :
        field(_field), inner(_inner), outer(_outer),
        subscription(std::bind(&field_copier_t<inner_t, outer_t>::on_change, this)) {
        typename watchable_t<inner_t>::freeze_t freeze(inner);
        subscription.reset(inner, &freeze);
        on_change();
    }

private:
    void on_change() {
        outer->apply_atomic_op([this](outer_t *value) {
            value->*field = inner->get();
            return true;
        });
    }

    inner_t outer_t::*field;
    clone_ptr_t<watchable_t<inner_t> > inner;
    watchable_variable_t<outer_t> *outer;
    typename watchable_t<inner_t>::subscription_t subscription;

    DISABLE_COPYING(field_copier_t);
};

template<class inner_t, class outer_t>
class field_getter_t {
public:
    explicit field_getter_t(inner_t outer_t::*f) : field(f) { }

    std::map<peer_id_t, inner_t> operator()(const std::map<peer_id_t, outer_t> &outer) const {
        std::map<peer_id_t, inner_t> inner;
        for (typename std::map<peer_id_t, outer_t>::const_iterator it = outer.begin(); it != outer.end(); it++) {
            inner[it->first] = it->second.*field;
        }
        return inner;
    }

    /* Support `boost::result_of`'s protocol for indicating functor return types
    so that we work with `watchable_t::subview()`. */
    template<class T>
    class result {
    public:
        typedef std::map<peer_id_t, inner_t> type;
    };

private:
    inner_t outer_t::*const field;
};

// `incremental_field_getter_t` does the same as field_getter_t, except that
// it is implemented as an incremental lens and operates on a 
// `change_tracking_map_t` instead of an `std::map`.
// `inner_field_getter_t` provides a mapping function, which is applied by
// `incremental_field_getter_t` to every value of a given map. It simply
// extracts a given field from the provided value.
template<class inner_t, class outer_t>
class inner_field_getter_t {
public:
    explicit inner_field_getter_t(inner_t outer_t::*f) : field(f) { }

    inner_t operator()(const outer_t &outer) const {
        return outer.*field;
    }

    /* Support `boost::result_of`'s protocol for indicating functor return types
    so that we work with `watchable_t::subview()`. */
    template<class T>
    class result {
    public:
        typedef inner_t type;
    };

private:
    inner_t outer_t::*const field;
};
template<class inner_t, class outer_t>
class incremental_field_getter_t
    : public incremental_map_lens_t<peer_id_t, outer_t, inner_field_getter_t<inner_t, outer_t> > {
public:
    explicit incremental_field_getter_t(inner_t outer_t::*f) :
        incremental_map_lens_t<peer_id_t, outer_t, inner_field_getter_t<inner_t, outer_t> >(
            inner_field_getter_t<inner_t, outer_t>(f)) {
    }
};

#endif /* CLUSTERING_ADMINISTRATION_MAIN_WATCHABLE_FIELDS_HPP_ */
