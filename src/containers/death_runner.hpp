// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONTAINERS_DEATH_RUNNER_HPP_
#define CONTAINERS_DEATH_RUNNER_HPP_

#include "errors.hpp"
#include <boost/function.hpp>

/* `death_runner_t` runs an arbitrary function in its destructor */
class death_runner_t {
public:
    death_runner_t() { }
    explicit death_runner_t(const boost::function<void()> &f) : fun(f) { }
    ~death_runner_t() {
        if (!fun.empty()) fun();
    }
    boost::function<void()> fun;
};


#endif  // CONTAINERS_DEATH_RUNNER_HPP_
