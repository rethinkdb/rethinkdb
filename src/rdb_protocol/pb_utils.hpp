#include "rdb_protocol/ql2.pb.h"

namespace ql {
namespace pb {

Datum *set_datum(Term2 *d);
Term2 *set_func(Term2 *f, int varnum);
void set_var(Term2 *v, int varnum);
template<class T>
T *reset(T *t) { *t = T(); return t; }

} // namespace pb
} // namespace ql
