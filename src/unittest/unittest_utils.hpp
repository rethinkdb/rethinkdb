#ifndef __UNITTEST_UNITTEST_UTILS_HPP__
#define __UNITTEST_UNITTEST_UTILS_HPP__

#include <cstdlib>
#include "errors.hpp"

#ifndef NDEBUG
#define trace_call(fn, args...) do {                                          \
        debugf("%s:%u: %s: entered\n", __FILE__, __LINE__, stringify(fn));  \
        fn(args);                                                           \
        debugf("%s:%u: %s: returned\n", __FILE__, __LINE__, stringify(fn)); \
    } while (0)
#define TRACEPOINT debugf("%s:%u reached\n", __FILE__, __LINE__)
#else
#define trace_call(fn, args...) fn(args)
// TRACEPOINT is not defined in release, so that TRACEPOINTS do not linger in the code unnecessarily
#endif

namespace unittest {

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

}  // namespace unittest

#endif // __UNITTEST_UTILS__
