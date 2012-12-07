#include "rdb_protocol/ql2.hpp"

namespace ql {
void _runtime_check(bool test, const char *teststr, std::string msg) {
    if (test) return;
#ifndef NDEBUG
    msg = msg + "\nFailed assertion: " + teststr;
#endif
    throw exc_t(msg);
}
} // namespace ql
