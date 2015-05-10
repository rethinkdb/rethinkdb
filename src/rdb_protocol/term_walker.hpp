#ifndef RDB_PROTOCOL_TERM_WALKER_HPP_
#define RDB_PROTOCOL_TERM_WALKER_HPP_

#include "rdb_protocol/error.hpp"

class Term;

namespace ql {

class backtrace_registry_t;

// Fills in the backtraces of a term and checks that it's well-formed with
// regard to write placement.
void preprocess_term(Term *root, backtrace_registry_t *bt_reg);

// Propagates a backtrace down a tree until it hits a node that already has a
// backtrace (this is used for e.g. rewrite terms so that they return reasonable
// backtraces in the macroexpanded nodes).
void propagate_backtrace(Term *root, backtrace_id_t bt);

} // namespace ql

#endif // RDB_PROTOCOL_TERM_WALKER_HPP_
