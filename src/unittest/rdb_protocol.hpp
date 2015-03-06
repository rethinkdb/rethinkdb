// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef UNITTEST_RDB_PROTOCOL_HPP_
#define UNITTEST_RDB_PROTOCOL_HPP_

#include <set>
#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/function.hpp>

#include "containers/scoped.hpp"

class namespace_interface_t;
class order_source_t;
class store_t;

namespace unittest {

/* A few functions that might be of use for other unit tests as well. */

void run_with_namespace_interface(
        boost::function<void(
            namespace_interface_t *,
            order_source_t *,
            const std::vector<scoped_ptr_t<store_t> > *)> fun,
        bool oversharding = false,
        int num_restarts = 1);

void wait_for_sindex(
    const std::vector<scoped_ptr_t<store_t> > *stores,
    const std::string &id);

} /* namespace unittest */

#endif  // UNITTEST_RDB_PROTOCOL_HPP_
