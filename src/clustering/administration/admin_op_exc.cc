// copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/admin_op_exc.hpp"

admin_op_exc_t::admin_op_exc_t(const char *format, ...) :
    std::runtime_error([&]() {
            va_list ap;
            va_start(ap, format);
            std::string msg = vstrprintf(format, ap);
            va_end(ap);
            return msg;
        }())
    { }
