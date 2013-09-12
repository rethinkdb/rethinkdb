#include "rdb_protocol/pb_utils.hpp"

#include "rdb_protocol/env.hpp"

namespace ql {
namespace pb {

sym_t dummy_var_to_sym(dummy_var_t dummy_var) {
    switch (dummy_var) {
    case dummy_var_t::A: return sym_t(-1);
    case dummy_var_t::B: return sym_t(-2);
    case dummy_var_t::C: return sym_t(-3);
    default:
        unreachable();
    }
}


Datum *set_datum(Term *d) {
    d->set_type(Term::DATUM);
    return d->mutable_datum();
}

Term *set_func(Term *f, dummy_var_t dummy_var, sym_t *varnum_out) {
    f->set_type(Term::FUNC);

    Datum *vars = set_datum(f->add_args());
    vars->set_type(Datum::R_ARRAY);
    Datum *var1 = vars->add_r_array();
    var1->set_type(Datum::R_NUM);
    const sym_t varnum = dummy_var_to_sym(dummy_var);
    var1->set_r_num(varnum.value);

    *varnum_out = varnum;
    return f->add_args();
}

Term *set_func(Term *f,
               dummy_var_t dummy_var1, sym_t *varnum1_out,
               dummy_var_t dummy_var2, sym_t *varnum2_out) {
    f->set_type(Term::FUNC);

    Datum *vars = set_datum(f->add_args());
    vars->set_type(Datum::R_ARRAY);
    Datum *var1 = vars->add_r_array();
    var1->set_type(Datum::R_NUM);
    const sym_t varnum1 = dummy_var_to_sym(dummy_var1);
    var1->set_r_num(varnum1.value);
    Datum *var2 = vars->add_r_array();
    var2->set_type(Datum::R_NUM);
    const sym_t varnum2 = dummy_var_to_sym(dummy_var2);
    var2->set_r_num(varnum2.value);

    *varnum1_out = varnum1;
    *varnum2_out = varnum2;
    return f->add_args();
}

void set_var(Term *v, sym_t varnum) {
    v->set_type(Term::VAR);
    Datum *vn = set_datum(v->add_args());
    vn->set_type(Datum::R_NUM);
    vn->set_r_num(varnum.value);
}

void set_null(Term *t) {
    Datum *d = set_datum(t);
    d->set_type(Datum::R_NULL);
}

void set(Term *out, Term_TermType type, std::vector<Term *> *args_out, int num_args) {
    out->set_type(type);
    for (int i = 0; i < num_args; ++i) args_out->push_back(out->add_args());
}

} // namespace pb
} // namespace ql
