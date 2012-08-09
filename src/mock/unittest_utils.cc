#include "mock/unittest_utils.hpp"

#include <stdlib.h>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/timing.hpp"
#include "arch/runtime/starter.hpp"
#include "utils.hpp"            // randint

namespace mock {

temp_file_t::temp_file_t(const char *tmpl) {
    size_t len = strlen(tmpl);
    filename.init(len + 1);
    memcpy(filename.data(), tmpl, len+1);
    int fd = mkstemp(filename.data());
    guarantee_err(fd != -1, "Couldn't create a temporary file");
    close(fd);
}

temp_file_t::~temp_file_t() {
    unlink(filename.data());
}

void let_stuff_happen() {
#ifdef VALGRIND
    nap(2000);
#else
    nap(100);
#endif
}

int randport() {
    return 10000 + randint(20000);
}

void run_in_thread_pool(const boost::function<void()>& fun, int num_workers) {
    ::run_in_thread_pool(fun, num_workers);
}

}  // namespace mock
