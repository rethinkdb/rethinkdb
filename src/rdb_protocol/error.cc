// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/error.hpp"

#include "backtrace.hpp"
#include "containers/archive/stl_types.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/term_walker.hpp"
#include "rdb_protocol/val.hpp"

namespace ql {

#ifdef RQL_ERROR_BT
#define RQL_ERROR_VAR
#else
#define RQL_ERROR_VAR UNUSED
#endif

void runtime_fail(base_exc_t::type_t type,
                  RQL_ERROR_VAR const char *test, RQL_ERROR_VAR const char *file,
                  RQL_ERROR_VAR int line,
                  std::string msg, backtrace_id_t bt) {
#ifdef RQL_ERROR_BT
    msg = strprintf("%s\nFailed assertion: %s\nAt: %s:%d",
                    msg.c_str(), test, file, line);
#endif
    throw exc_t(type, msg, bt);
}
void runtime_fail(base_exc_t::type_t type,
                  RQL_ERROR_VAR const char *test, RQL_ERROR_VAR const char *file,
                  RQL_ERROR_VAR int line, std::string msg) {
#ifdef RQL_ERROR_BT
    msg = strprintf("%s\nFailed assertion: %s\nAt: %s:%d",
                    msg.c_str(), test, file, line);
#endif
    throw datum_exc_t(type, msg);
}

void runtime_sanity_check_failed(const char *file, int line, const char *test,
                                 const std::string &msg) {
    lazy_backtrace_formatter_t bt;
    std::string error_msg = "[" + std::string(test) + "]";
    if (!msg.empty()) {
        error_msg += " " + msg;
    }
    throw exc_t(base_exc_t::INTERNAL,
                strprintf("SANITY CHECK FAILED: %s at `%s:%d` (server is buggy).  "
                          "Backtrace:\n%s",
                          error_msg.c_str(), file, line, bt.addrs().c_str()),
                backtrace_id_t::empty());
}

base_exc_t::type_t exc_type(const datum_t *d) {
    r_sanity_check(d);
    return d->get_type() == datum_t::R_NULL
        ? base_exc_t::NON_EXISTENCE
        : base_exc_t::LOGIC;
}
base_exc_t::type_t exc_type(const datum_t &d) {
    r_sanity_check(d.has());
    return exc_type(&d);
}
base_exc_t::type_t exc_type(const val_t *v) {
    r_sanity_check(v);
    if (v->get_type().is_convertible(val_t::type_t::DATUM)) {
        return exc_type(v->as_datum());
    } else {
        return base_exc_t::LOGIC;
    }
}
base_exc_t::type_t exc_type(const scoped_ptr_t<val_t> &v) {
    r_sanity_check(v.has());
    return exc_type(v.get());
}
std::string format_array_size_error(size_t limit) {
    
  return "Array over size limit `" + std::to_string(limit) + "`.  To raise the number "
         "of allowed elements, modify the `array_limit` option to `.run` "
         "(not available in the Data Explorer), or use an index.";
}
RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(backtrace_id_t, id);
RDB_IMPL_SERIALIZABLE_4_SINCE_v1_13(exc_t, type, message, bt, dummy_frames_);
RDB_IMPL_SERIALIZABLE_2_SINCE_v1_13(datum_exc_t, type, message);

} // namespace ql
