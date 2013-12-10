// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef BOOST_UTILS_HPP_
#define BOOST_UTILS_HPP_

#include "errors.hpp"
#include <boost/optional.hpp>

#include "containers/printf_buffer.hpp"

template <class T>
void debug_print(printf_buffer_t *buf, const boost::optional<T> &value) {
    if (value) {
        buf->appendf("opt(");
        debug_print(buf, *value);
        buf->appendf("}");
    } else {
        buf->appendf("none");
    }
}


#endif  // BOOST_UTILS_HPP_
