// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef PERFMON_FILTER_HPP_
#define PERFMON_FILTER_HPP_

#include <set>
#include <string>
#include <vector>

#include "errors.hpp"

class perfmon_result_t;
template <class> class scoped_ptr_t;
class scoped_regex_t;

class perfmon_filter_t {
public:
    explicit perfmon_filter_t(const std::set<std::string> &paths);
    ~perfmon_filter_t();
    // This takes a const scoped_ptr_t because subfilter needs one to sanely work.
    void filter(const scoped_ptr_t<perfmon_result_t> *target) const;
private:
    void subfilter(scoped_ptr_t<perfmon_result_t> *target,
                   size_t depth, std::vector<bool> active) const;
    std::vector<std::vector<scoped_regex_t *> > regexps; //regexps[PATH][DEPTH]
    DISABLE_COPYING(perfmon_filter_t);
};


#endif  // PERFMON_FILTER_HPP_
