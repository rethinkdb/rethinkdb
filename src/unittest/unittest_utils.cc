#include "unittest/unittest_utils.hpp"

#include <stdlib.h>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/arch.hpp"
#include "utils.hpp"            // randint

namespace unittest {

temp_file_t::temp_file_t(const char *tmpl) {
    size_t len = strlen(tmpl);
    filename = new char[len+1];
    memcpy(filename, tmpl, len+1);
    int fd = mkstemp(filename);
    guarantee_err(fd != -1, "Couldn't create a temporary file");
    close(fd);
}

temp_file_t::~temp_file_t() {
    unlink(filename);
    delete [] filename;
}

void let_stuff_happen() {
    nap(100);
}

int randport() {
    return 10000 + randint(20000);
}

void run_in_thread_pool(const boost::function<void()>& fun, int num_workers) {
    ::run_in_thread_pool(fun, num_workers);
}


}  // namespace unittest
