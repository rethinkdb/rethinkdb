// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_DATUM_UTILS_HPP_
#define RDB_PROTOCOL_DATUM_UTILS_HPP_

#include "containers/archive/versioned.hpp"
#include "rdb_protocol/datum.hpp"

class optional_datum_less_t {
public:
    explicit optional_datum_less_t() { }
    bool operator()(const ql::datum_t &a, const ql::datum_t &b) const {
        if (a.has()) {
            return b.has() && a < b;
        } else {
            return b.has();
        }
    }
};

#endif /* RDB_PROTOCOL_DATUM_UTILS_HPP_ */
