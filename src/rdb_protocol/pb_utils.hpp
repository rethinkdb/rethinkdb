#ifndef RDB_PROTOCOL_PB_UTILS_HPP_
#define RDB_PROTOCOL_PB_UTILS_HPP_

#include <string>
#include <vector>

#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/sym.hpp"

namespace ql {
namespace pb {

// Dummy variables are used to identify the variables we use when constructing
// home-made reql functions.  set_func and map_wire_func_t::make_safely both make it
// difficult to create a reql function referencing an external dummy variable -- they
// generate a sym_t for you to use inside the function body (and nowhere else!).
//
// You aren't perfectly safe though -- what if you put your variable inside a term that
// expands its body into a reql function!  So, we use a bunch of different dummy
// variable names for use in different types.
enum class dummy_var_t {
    IGNORED,  // For functions that ignore their parameter.  There's no special meaning
              // to this value, we just don't need to create separate names for these
              // instances.
    VAL_UPSERT_REPLACEMENT,
    GROUPBY_MAP_OBJ,
    GROUPBY_MAP_ATTR,
    GROUPBY_REDUCE_A,
    GROUPBY_REDUCE_B,
    GROUPBY_FINAL_OBJ,
    GROUPBY_FINAL_VAL,
    INNERJOIN_N,
    INNERJOIN_M,
    OUTERJOIN_N,
    OUTERJOIN_M,
    OUTERJOIN_LST,
    EQJOIN_ROW,
    EQJOIN_V,
    UPDATE_OLDROW,
    UPDATE_NEWROW,
    DIFFERENCE_ROW,
    SINDEXCREATE_X,
    OBJORSEQ_VARNUM,
    FUNC_GETFIELD,
    FUNC_PLUCK,
    FUNC_EQCOMPARISON,
};

// Don't use this!  set_func and map_wire_func_t::make_safely use this.  Returns the
// sym_t corresponding to a dummy_var_t.
sym_t dummy_var_to_sym(dummy_var_t dummy_var);

// Set `d` to be a datum term, return a pointer to its datum member.
Datum *set_datum(Term *d);
// Set `f` to be a function of one variable, whose name is specified by dummy_var,
// return a pointer to its body (and output the sym_t name of the variable to use
// when constructing the body).
Term *set_func(Term *f, dummy_var_t dummy_var, sym_t *varnum_out);
// As above but with two variables.
Term *set_func(Term *f,
               dummy_var_t dummy_var1, sym_t *varnum1_out,
               dummy_var_t dummy_var2, sym_t *varnum2_out);
// Set `v` to be the variable `varnum`.
void set_var(Term *v, sym_t varnum);

// Set `t` to be a particular datum.
void set_null(Term *t);

// Set `out` to be `type` with the appropriate arguments.
void set(Term *out, Term_TermType type, std::vector<Term *> *args_out, int num_args);

// These macros implement anaphoric stack machine logic.  They are used
// extensively in rewrites.hpp and probably one or two other places.  They need
// to have the -Wshadow diagnostic disabled for their extent; see rewrites.hpp
// for how to do this.
//
// These macros expect `arg` to point to the Term they should modify.  They set
// its type to PB, and then for each of their arguments, rebind `arg` to point
// to the appropriate portion of the new Term and execute that argument.
#define N0(PB) do {                             \
        arg->set_type(Term_TermType_##PB);     \
    } while (0);
#define N1(PB, ARG1) do {                       \
        arg->set_type(Term_TermType_##PB);     \
        Term *PB_arg1 = arg->add_args();       \
        Term *arg = PB_arg1;                   \
        ARG1;                                   \
    } while (0);
#define N2(PB, ARG1, ARG2) do {                 \
        arg->set_type(Term_TermType_##PB);     \
        Term *PB_arg1 = arg->add_args();       \
        Term *PB_arg2 = arg->add_args();       \
        Term *arg = PB_arg1;                   \
        ARG1;                                   \
        arg = PB_arg2;                          \
        ARG2;                                   \
    } while (0);
#define N3(PB, ARG1, ARG2, ARG3) do {           \
        arg->set_type(Term_TermType_##PB);     \
        Term *PB_arg1 = arg->add_args();       \
        Term *PB_arg2 = arg->add_args();       \
        Term *PB_arg3 = arg->add_args();       \
        Term *arg = PB_arg1;                   \
        ARG1;                                   \
        arg = PB_arg2;                          \
        ARG2;                                   \
        arg = PB_arg3;                          \
        ARG3;                                   \
    } while (0);
#define N4(PB, ARG1, ARG2, ARG3, ARG4) do {     \
        arg->set_type(Term_TermType_##PB);     \
        Term *PB_arg1 = arg->add_args();       \
        Term *PB_arg2 = arg->add_args();       \
        Term *PB_arg3 = arg->add_args();       \
        Term *PB_arg4 = arg->add_args();       \
        Term *arg = PB_arg1;                   \
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
void run(counted_t<const datum_t> d, U arg) {
    run(*d, arg);
}

template<class T, class U>
void run(T t, U arg) { run(datum_t(t), arg); }

template<class A, class B, class U>
void run(A a, B b, U arg) { run(datum_t(a, b), arg); }

} // namespace ndatum_impl

#define NDATUM(val) ql::pb::ndatum_impl::run(val, arg)
#define NDATUM_BOOL(val) ql::pb::ndatum_impl::run(ql::datum_t::R_BOOL, val, arg)

// Like `N*` but for optional arguments.  See rewrites.hpp for examples if you
// need to use them.
#define OPT1(PB, STR1, ARG1) do {                       \
        arg->set_type(Term_TermType_##PB);             \
        Term_AssocPair *PB_ap1 = arg->add_optargs();   \
        PB_ap1->set_key(STR1);                          \
        arg = PB_ap1->mutable_val();                    \
        ARG1;                                           \
    } while (0);
#define OPT2(PB, STR1, ARG1, STR2, ARG2) do {           \
        arg->set_type(Term_TermType_##PB);             \
        Term_AssocPair *PB_ap1 = arg->add_optargs();   \
        PB_ap1->set_key(STR1);                          \
        Term_AssocPair *PB_ap2 = arg->add_optargs();   \
        PB_ap2->set_key(STR2);                          \
        arg = PB_ap1->mutable_val();                    \
        ARG1;                                           \
        arg = PB_ap2->mutable_val();                    \
        ARG2;                                           \
    } while (0);
#define OPT3(PB, STR1, ARG1, STR2, ARG2, STR3, ARG3) do {           \
        arg->set_type(Term_TermType_##PB);             \
        Term_AssocPair *PB_ap1 = arg->add_optargs();   \
        PB_ap1->set_key(STR1);                          \
        Term_AssocPair *PB_ap2 = arg->add_optargs();   \
        PB_ap2->set_key(STR2);                          \
        Term_AssocPair *PB_ap3 = arg->add_optargs();   \
        PB_ap3->set_key(STR3);                          \
        arg = PB_ap1->mutable_val();                    \
        ARG1;                                           \
        arg = PB_ap2->mutable_val();                    \
        ARG2;                                           \
        arg = PB_ap3->mutable_val();                    \
        ARG3;                                           \
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
