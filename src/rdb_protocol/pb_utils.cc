#include "rdb_protocol/env.hpp"

namespace ql {
namespace pb {

#define T(A) Term2_TermType_##A
#define D(A) Datum_DatumType_##A

Datum *set_datum(Term2 *d) {
    d->set_type(T(DATUM));
    return d->mutable_datum();
}

Term2 *set_func(Term2 *f, int varnum) {
    f->set_type(T(FUNC));

    Datum *vars = set_datum(f->add_args());
    vars->set_type(D(R_ARRAY));
    Datum *var1 = vars->add_r_array();
    var1->set_type(D(R_NUM));
    var1->set_r_num(varnum);

    return f->add_args();
}

void set_var(Term2 *v, int varnum) {
    v->set_type(T(VAR));
    Datum *vn = set_datum(v->add_args());
    vn->set_type(D(R_NUM));
    vn->set_r_num(varnum);
}

void set_int(Term2 *i, int num) {
    Datum *d = set_datum(i);
    d->set_type(D(R_NUM));
    d->set_r_num(num);
}

void set_str(Term2 *t, const std::string &s) {
    Datum *d = set_datum(t);
    d->set_type(D(R_STR));
    d->set_r_str(s);
}

void set(Term2 *out, Term2_TermType type, std::vector<Term2 *> *args_out, int num_args) {
    out->set_type(type);
    for (int i = 0; i < num_args; ++i) args_out->push_back(out->add_args());
}

} //namespace pb
} //namespace ql
