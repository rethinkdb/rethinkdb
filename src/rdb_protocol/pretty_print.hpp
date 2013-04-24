#ifndef RDB_PROTOCOL_PRETTY_PRINT_HPP_
#define RDB_PROTOCOL_PRETTY_PRINT_HPP_

#include "utils.hpp"

namespace ql {
class wire_func_t;
}

void debug_print(append_only_printf_buffer_t *buf, const ql::wire_func_t &func) {
    debug_print(buf, func.debug_str());
}

#endif // RDB_PROTOCOL_PRETTY_PRINT_HPP_
