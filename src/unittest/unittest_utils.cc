// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/unittest_utils.hpp"

#include <stdlib.h>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/timing.hpp"
#include "arch/runtime/starter.hpp"
#include "utils.hpp"

namespace unittest {

temp_file_t::temp_file_t() {
    char tmpl[] = "/tmp/rdb_unittest.XXXXXX";
    const int fd = mkstemp(tmpl);
    guarantee_err(fd != -1, "Couldn't create a temporary file");
    close(fd);

    filename = tmpl + 5;
}

temp_file_t::~temp_file_t() {
    unlink(("/tmp" + filename).c_str());
}

void let_stuff_happen() {
#ifdef VALGRIND
    nap(2000);
#else
    nap(100);
#endif
}

std::set<ip_address_t> get_unittest_addresses() {
    return ip_address_t::get_local_addresses(std::set<ip_address_t>(), false);
}

int randport() {
    return 10000 + randint(20000);
}

void run_in_thread_pool(const boost::function<void()>& fun, int num_workers) {
    ::run_in_thread_pool(fun, num_workers);
}

}  // namespace unittest
