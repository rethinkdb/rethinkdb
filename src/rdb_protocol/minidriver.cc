#include "rdb_protocol/minidriver.hpp"

namespace ql {

namespace r {

reql_t::reql_t(scoped_ptr_t<Term> &&term_) : term(std::move(term_)) { }

reql_t::reql_t(const double val) { set_datum(datum_t(val)); }

reql_t::reql_t(const std::string &val) { set_datum(datum_t(val.c_str())); }

reql_t::reql_t(const datum_t &d) { set_datum(d); }

reql_t::reql_t(const Datum &d) : term(make_scoped<Term>()) {
    term->set_type(Term::DATUM);
    *term->mutable_datum() = d;
}

reql_t::reql_t(const Term &t) : term(make_scoped<Term>(t)) { }

reql_t::reql_t(std::vector<reql_t> &&val) : term(make_scoped<Term>()) {
    term->set_type(Term::MAKE_ARRAY);
    for (auto i = val.begin(); i != val.end(); i++) {
        add_arg(std::move(*i));
    }
}

reql_t reql_t::copy() const { return reql_t(make_scoped<Term>(get())); }

reql_t::reql_t(reql_t &&other) : term(std::move(other.term)) {
    guarantee(term.has());
}

reql_t::reql_t(pb::dummy_var_t var) : term(make_scoped<Term>()) {
    term->set_type(Term::VAR);
    add_arg(static_cast<double>(dummy_var_to_sym(var).value));
}

reql_t boolean(bool b) {
    return reql_t(datum_t(datum_t::construct_boolean_t(), b));
}

void reql_t::copy_optargs_from_term(const Term &from) {
    for (int i = 0; i < from.optargs_size(); ++i) {
        const Term_AssocPair &o = from.optargs(i);
        add_arg(r::optarg(o.key(), o.val()));
    }
}

void reql_t::copy_args_from_term(const Term &from, size_t start_index) {
    for (int i = start_index; i < from.args_size(); ++i) {
        add_arg(from.args(i));
    }
}

reql_t &reql_t::operator=(reql_t &&other) {
    term.swap(other.term);
    return *this;
}

reql_t fun(reql_t&& body) {
    return reql_t(Term::FUNC, array(), std::move(body));
}

reql_t fun(pb::dummy_var_t a, reql_t&& body) {
    std::vector<reql_t> v;
    v.emplace_back(static_cast<double>(dummy_var_to_sym(a).value));
    return reql_t(Term::FUNC, std::move(v), std::move(body));
}

reql_t fun(pb::dummy_var_t a, pb::dummy_var_t b, reql_t&& body) {
    std::vector<reql_t> v;
    v.emplace_back(static_cast<double>(dummy_var_to_sym(a).value));
    v.emplace_back(static_cast<double>(dummy_var_to_sym(b).value));
    return reql_t(Term::FUNC, std::move(v), std::move(body));
}

reql_t null() {
    auto t = make_scoped<Term>();
    t->set_type(Term::DATUM);
    t->mutable_datum()->set_type(Datum::R_NULL);
    return reql_t(std::move(t));
}

Term *reql_t::release() {
    guarantee(term.has());
    return term.release();
}

Term &reql_t::get() {
    guarantee(term.has());
    return *term;
}

const Term &reql_t::get() const {
    guarantee(term.has());
    return *term;
}

protob_t<Term> reql_t::release_counted() {
    guarantee(term.has());
    protob_t<Term> ret = make_counted_term();
    auto t = scoped_ptr_t<Term>(term.release());
    ret->Swap(t.get());
    return ret;
}

void reql_t::swap(Term &t) {
    t.Swap(term.get());
}

reql_t reql_t::operator !() RVALUE_THIS {
    return std::move(*this).call(Term::NOT);
}

reql_t reql_t::do_(pb::dummy_var_t arg, reql_t &&body) RVALUE_THIS {
        return fun(arg, std::move(body))(std::move(*this));
}

void reql_t::set_datum(const datum_t &d) {
    term = make_scoped<Term>();
    term->set_type(Term::DATUM);
    d.write_to_protobuf(term->mutable_datum(), use_json_t::NO);
}

reql_t db(const std::string &name) {
    return reql_t(Term::DB, expr(name));
}

reql_t var(pb::dummy_var_t v) {
    return reql_t(Term::VAR, static_cast<double>(dummy_var_to_sym(v).value));
}

reql_t var(const sym_t &v) {
    return reql_t(Term::VAR, static_cast<double>(v.value));
}

} // namespace r

} // namespace ql
