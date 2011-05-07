#ifndef __UNITTEST_UNITTEST_UTILS_HPP__
#define __UNITTEST_UNITTEST_UTILS_HPP__

#include <cstdlib>
#include "errors.hpp"

class temp_file_t {
    char * filename;
public:
    temp_file_t(const char * tmpl) {
        size_t len = strlen(tmpl);
        filename = new char[len+1];
        memcpy(filename, tmpl, len+1);
        int fd = mkstemp(filename);
        guarantee_err(fd != -1, "Couldn't create a temporary file");
        close(fd);
    }
    std::string name() {
        return filename;
    }
    ~temp_file_t() {
        unlink(filename);
        delete [] filename;
    }
};

#endif // __UNITTEST_UTILS__
