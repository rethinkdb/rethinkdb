// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef PERFMON_FILTER_HPP_
#define PERFMON_FILTER_HPP_

#include <set>
#include <string>
#include <vector>

#include "errors.hpp"

#include "re2/re2.h"
#include "rdb_protocol/datum.hpp"

template <class> class scoped_ptr_t;

class perfmon_filter_t {
public:
    explicit perfmon_filter_t(const std::set<std::vector<std::string> > &paths);
    ql::datum_t filter(const ql::datum_t &stats) const;
private:
    ql::datum_t subfilter(const ql::datum_t &stats,
                          size_t depth, std::vector<bool> active) const;
    std::vector<std::vector<scoped_ptr_t<RE2> > > regexps; //regexps[PATH][DEPTH]
    DISABLE_COPYING(perfmon_filter_t);
};

#endif  // PERFMON_FILTER_HPP_
