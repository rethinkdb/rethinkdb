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
    // Unlink both possible locations of the file.
    const int res1 = ::unlink(name().temporary_path().c_str());
    EXPECT_TRUE(res1 == 0 || errno == ENOENT);
    const int res2 = ::unlink(name().permanent_path().c_str());
    EXPECT_TRUE(res2 == 0 || errno == ENOENT);
}

serializer_filepath_t temp_file_t::name() const {
    return serializer_filepath_t(base_path_t("/tmp"), filename);
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
