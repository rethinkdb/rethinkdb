#include "rdb_protocol/ql2.hpp"

namespace ql {
void _runtime_check(bool test, const char *teststr, std::string msg) {
    if (test) return;
#ifndef NDEBUG
    msg = msg + "\nFailed assertion: " + teststr;
#endif
    throw exc_t(msg);
}

void fill_error(Response2 *res, Response2_ResponseType type, std::string msg) {
    guarantee(type == Response2::CLIENT_ERROR ||
              type == Response2::COMPILE_ERROR ||
              type == Response2::RUNTIME_ERROR);
    Datum error_msg;
    error_msg.set_type(Datum::R_STR);
    error_msg.set_r_str(msg);
    res->set_type(type);
    *res->add_response() = error_msg;
}
} // namespace ql
