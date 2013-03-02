#ifndef RDB_PROTOCOL_PB_UTILS_HPP_
#define RDB_PROTOCOL_PB_UTILS_HPP_

#include <string>
#include <vector>

#include "rdb_protocol/ql2.pb.h"

namespace ql {
namespace pb {

// Set `d` to be a datum term, return a pointer to its datum member.
Datum *set_datum(Term2 *d);
// Set `f` to be a function of `varnum`, return a pointer to its body.
Term2 *set_func(Term2 *f, int varnum);
// As above but with two variables.
Term2 *set_func(Term2 *f, int varnum1, int varnum2);
// Set `v` to be the variable `varnum`.
void set_var(Term2 *v, int varnum);

// Set `t` to be a particular datum.
void set_null(Term2 *t);
void set_int(Term2 *t, int num);
void set_str(Term2 *t, const std::string &s);

// Set `out` to be `type` with the appropriate arguments.
void set(Term2 *out, Term2_TermType type, std::vector<Term2 *> *args_out, int num_args);

// These macros implement anaphoric stack machine logic.  They are used
// extensively in rewrites.hpp and probably one or two other places.  They need
// to have the -Wshadow diagnostic disabled for their extent; see rewrites.hpp
// for how to do this.
//
// These macros expect `arg` to point to the Term2 they should modify.  They set
// its type to PB, and then for each of their arguments, rebind `arg` to point
// to the appropriate portion of the new Term2 and execute that argument.
#define N0(PB) do {                             \
        arg->set_type(Term2_TermType_##PB);     \
    } while (0);
#define N1(PB, ARG1) do {                       \
        arg->set_type(Term2_TermType_##PB);     \
        Term2 *PB_arg1 = arg->add_args();       \
        Term2 *arg = PB_arg1;                   \
        ARG1;                                   \
    } while (0);
#define N2(PB, ARG1, ARG2) do {                 \
        arg->set_type(Term2_TermType_##PB);     \
        Term2 *PB_arg1 = arg->add_args();       \
        Term2 *PB_arg2 = arg->add_args();       \
        Term2 *arg = PB_arg1;                   \
        ARG1;                                   \
        arg = PB_arg2;                          \
        ARG2;                                   \
    } while (0);
#define N3(PB, ARG1, ARG2, ARG3) do {           \
        arg->set_type(Term2_TermType_##PB);     \
        Term2 *PB_arg1 = arg->add_args();       \
        Term2 *PB_arg2 = arg->add_args();       \
        Term2 *PB_arg3 = arg->add_args();       \
        Term2 *arg = PB_arg1;                   \
        ARG1;                                   \
        arg = PB_arg2;                          \
        ARG2;                                   \
        arg = PB_arg3;                          \
        ARG3;                                   \
    } while (0);
#define N4(PB, ARG1, ARG2, ARG3, ARG4) do {     \
        arg->set_type(Term2_TermType_##PB);     \
        Term2 *PB_arg1 = arg->add_args();       \
        Term2 *PB_arg2 = arg->add_args();       \
        Term2 *PB_arg3 = arg->add_args();       \
        Term2 *PB_arg4 = arg->add_args();       \
        Term2 *arg = PB_arg1;                   \
        ARG1;                                   \
        arg = PB_arg2;                          \
        ARG2;                                   \
        arg = PB_arg3;                          \
        ARG3;                                   \
        arg = PB_arg4;                          \
        ARG4;                                   \
    } while (0);

// Wrappers around `set_var` and `write_to_protobuf` that work with the `N*` macros.
#define NVAR(varnum) ql::pb::set_var(arg, varnum)

namespace ndatum_impl {
template<class U>
void run(const datum_t &d, U arg) {
    d.write_to_protobuf(ql::pb::set_datum(arg));
}
template<class U>
void run(const datum_t *d, U arg) { run(*d, arg); }
template<class T, class U>
void run(T t, U arg) { run(datum_t(t), arg); }
} // namespace ndatum_impl

#define NDATUM(val) ql::pb::ndatum_impl::run(val, arg)

// Like `N*` but for optional arguments.  See rewrites.hpp for examples if you
// need to use them.
#define OPT1(PB, STR1, ARG1) do {                       \
        arg->set_type(Term2_TermType_##PB);             \
        Term2_AssocPair *PB_ap1 = arg->add_optargs();   \
        PB_ap1->set_key(STR1);                          \
        arg = PB_ap1->mutable_val();                    \
        ARG1;                                           \
    } while (0);
#define OPT2(PB, STR1, ARG1, STR2, ARG2) do {           \
        arg->set_type(Term2_TermType_##PB);             \
        Term2_AssocPair *PB_ap1 = arg->add_optargs();   \
        PB_ap1->set_key(STR1);                          \
        Term2_AssocPair *PB_ap2 = arg->add_optargs();   \
        PB_ap2->set_key(STR2);                          \
        arg = PB_ap1->mutable_val();                    \
        ARG1;                                           \
        arg = PB_ap2->mutable_val();                    \
        ARG2;                                           \
    } while (0);

// Used to empty portions of protobufs when we're modifying them in-place.
template<class T>
T *reset(T *t) {
    *t = T();
    return t;
}

// To debug: term->DebugString()

} // namespace pb
} // namespace ql

#endif // RDB_PROTOCOL_PB_UTILS_HPP_
