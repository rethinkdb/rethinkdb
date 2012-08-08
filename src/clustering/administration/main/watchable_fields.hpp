#ifndef CLUSTERING_ADMINISTRATION_MAIN_WATCHABLE_FIELDS_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_WATCHABLE_FIELDS_HPP_

#include <map>

#include "concurrency/watchable.hpp"
#include "rpc/connectivity/connectivity.hpp"

template<class inner_t, class outer_t>
class field_copier_t {
public:
    field_copier_t(inner_t outer_t::*_field, const clone_ptr_t<watchable_t<inner_t> > &_inner, watchable_variable_t<outer_t> *_outer) :
        field(_field), inner(_inner), outer(_outer),
        subscription(boost::bind(&field_copier_t<inner_t, outer_t>::on_change, this)) {
        typename watchable_t<inner_t>::freeze_t freeze(inner);
        subscription.reset(inner, &freeze);
        on_change();
    }

private:
    void on_change() {
        outer_t value = outer->get_watchable()->get();
        value.*field = inner->get();
        outer->set_value(value);
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

#endif /* CLUSTERING_ADMINISTRATION_MAIN_WATCHABLE_FIELDS_HPP_ */
