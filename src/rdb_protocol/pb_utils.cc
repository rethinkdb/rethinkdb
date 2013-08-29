#include "rdb_protocol/env.hpp"

namespace ql {
namespace pb {

Datum *set_datum(Term *d) {
    d->set_type(Term::DATUM);
    return d->mutable_datum();
}

Term *set_func(Term *f, sym_t varnum) {
    f->set_type(Term::FUNC);

    Datum *vars = set_datum(f->add_args());
    vars->set_type(Datum::R_ARRAY);
    Datum *var1 = vars->add_r_array();
    var1->set_type(Datum::R_NUM);
    var1->set_r_num(varnum.value);

    return f->add_args();
}

Term *set_func(Term *f, sym_t varnum1, sym_t varnum2) {
    f->set_type(Term::FUNC);

    Datum *vars = set_datum(f->add_args());
    vars->set_type(Datum::R_ARRAY);
    Datum *var1 = vars->add_r_array();
    var1->set_type(Datum::R_NUM);
    var1->set_r_num(varnum1.value);
    Datum *var2 = vars->add_r_array();
    var2->set_type(Datum::R_NUM);
    var2->set_r_num(varnum2.value);

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

void set_int(Term *t, int num) {
    Datum *d = set_datum(t);
    d->set_type(Datum::R_NUM);
    d->set_r_num(num);
}

void set_str(Term *t, const std::string &s) {
    Datum *d = set_datum(t);
    d->set_type(Datum::R_STR);
    d->set_r_str(s);
}

void set(Term *out, Term_TermType type, std::vector<Term *> *args_out, int num_args) {
    out->set_type(type);
    for (int i = 0; i < num_args; ++i) args_out->push_back(out->add_args());
}

} // namespace pb
} // namespace ql
