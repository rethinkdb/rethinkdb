// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_MATH_UTILS_HPP_
#define RDB_PROTOCOL_MATH_UTILS_HPP_

#include <limits>

template <class T>
double safe_to_double(T val) {
    static const int64_t upper_limit = 1LL << 53;
    static const int64_t lower_limit = -(upper_limit);

    if (static_cast<int64_t>(val) > upper_limit || static_cast<int64_t>(val) < lower_limit) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    double res = static_cast<double>(val);
    rassert(val == static_cast<T>(res));
    return res;
}

#endif // RDB_PROTOCOL_MATH_UTILS_HPP_
