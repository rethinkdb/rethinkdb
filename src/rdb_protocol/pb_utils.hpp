#ifndef RDB_PROTOCOL_PB_UTILS_HPP_
#define RDB_PROTOCOL_PB_UTILS_HPP_

#include "rdb_protocol/ql2.pb.h"

namespace ql {
namespace pb {

Datum *set_datum(Term2 *d);
Term2 *set_func(Term2 *f, int varnum);
void set_var(Term2 *v, int varnum);

void set(Term2 *out, Term2_TermType type, std::vector<Term2 *> *args_out, int num_args);

template<class T>
T *reset(T *t) { *t = T(); return t; }

// To debug: term->DebugString()

} // namespace pb
} // namespace ql

#endif // RDB_PROTOCOL_PB_UTILS_HPP_
