// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_DATUM_UTILS_HPP_
#define RDB_PROTOCOL_DATUM_UTILS_HPP_

#include "containers/archive/versioned.hpp"
#include "rdb_protocol/datum.hpp"

class optional_datum_less_t {
public:
    explicit optional_datum_less_t(reql_version_t reql_version)
        : reql_version_(reql_version) { }

    bool operator()(const ql::datum_t &a,
                    const ql::datum_t &b) const {
        if (a.has()) {
            return b.has() && a.compare_lt(reql_version_, b);
        } else {
            return b.has();
        }
    }

    reql_version_t reql_version() const { return reql_version_; }
private:
    reql_version_t reql_version_;
};

/* Sorts datums according to an undefined deterministic ordering. */
class latest_version_optional_datum_less_t :
    public optional_datum_less_t {
public:
    latest_version_optional_datum_less_t() :
        optional_datum_less_t(reql_version_t::LATEST) { }
};

#endif /* RDB_PROTOCOL_DATUM_UTILS_HPP_ */
