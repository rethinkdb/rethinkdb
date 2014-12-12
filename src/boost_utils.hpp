// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BOOST_UTILS_HPP_
#define BOOST_UTILS_HPP_

#include "errors.hpp"
#include <boost/optional.hpp>

#include "containers/printf_buffer.hpp"

template<class T, class U>
T opt_or(const boost::optional<T> t, U u) {
    return t ? std::move(*t) : T(std::move(u));
}

template<class T>
bool opt_lt(const boost::optional<T> &a, const boost::optional<T> &b) {
    return (a && b) ? (*a < *b) : (!a && b);
}

template <class T>
void debug_print(printf_buffer_t *buf, const boost::optional<T> &value) {
    if (value) {
        buf->appendf("opt{");
        debug_print(buf, *value);
        buf->appendf("}");
    } else {
        buf->appendf("none");
    }
}

#endif  // BOOST_UTILS_HPP_
