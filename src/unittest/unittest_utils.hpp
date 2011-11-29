#ifndef __UNITTEST_UNITTEST_UTILS_HPP__
#define __UNITTEST_UNITTEST_UTILS_HPP__

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

/* `run_in_thread_pool()` starts a RethinkDB thread pool, runs the given
function in a coroutine inside of it, waits for the function to return, and then
shuts down the thread pool. */

void run_in_thread_pool(const boost::function<void()>& fun, int num_threads = 1);

#endif // __UNITTEST_UTILS__
