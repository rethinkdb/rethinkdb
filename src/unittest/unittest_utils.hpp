#ifndef UNITTEST_UNITTEST_UTILS_HPP_
#define UNITTEST_UNITTEST_UTILS_HPP_

#include "errors.hpp"
#include <boost/function.hpp>

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
    char *filename;
public:
    explicit temp_file_t(const char *tmpl);
    const char *name() { return filename; }
    ~temp_file_t();
};

}  // namespace unittest

#endif  // UNITTEST_UNITTEST_UTILS_HPP_
