#ifndef RDB_PROTOCOL_TERM_WALKER_HPP_
#define RDB_PROTOCOL_TERM_WALKER_HPP_

class Term;
class Backtrace;

namespace ql {

// Fills in the backtraces of a term and checks that it's well-formed with
// regard to write placement.
void preprocess_term(Term *root);

// Propagates a backtrace down a tree until it hits a node that already has a
// backtrace (this is used for e.g. rewrite terms so that they return reasonable
// backtraces in the macroexpanded nodes).
void propagate_backtrace(Term *root, const Backtrace *bt);

} // namespace ql

#endif // RDB_PROTOCOL_TERM_WALKER_HPP_
