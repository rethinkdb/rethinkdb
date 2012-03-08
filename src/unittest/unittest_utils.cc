#include "unittest/unittest_utils.hpp"

#include <stdlib.h>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "do_on_thread.hpp"
#include "arch/arch.hpp"

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

}  // namespace unittest
