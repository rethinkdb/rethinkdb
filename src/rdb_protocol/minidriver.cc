// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/minidriver.hpp"

namespace ql {

minidriver_t::minidriver_t(backtrace_id_t _bt) :
    bt(_bt) { }

minidriver_t::reql_t &minidriver_t::reql_t::operator=(
        const minidriver_t::reql_t &other) {
    r = other.r;
    term = other.term;
    return *this;
}

minidriver_t::reql_t::reql_t(const reql_t &other) :
        r(other.r), term(other.term) { }

minidriver_t::reql_t::reql_t(minidriver_t *_r, const raw_term_t &other) :
        r(_r), term(make_counted<generated_term_t>(other.type(), other.bt())) {
    term->datum = other.datum();
    copy_args_from_term(other, 0);
    copy_optargs_from_term(other);
}

minidriver_t::reql_t::reql_t(minidriver_t *_r, const reql_t &other) :
        r(_r), term(other.term) { }

minidriver_t::reql_t::reql_t(minidriver_t *_r, double val) :
        r(_r), term(make_counted<generated_term_t>(Term::DATUM, r->bt)) {
    term->datum = datum_t(val);
}

minidriver_t::reql_t::reql_t(minidriver_t *_r, const std::string &val) :
        r(_r), term(make_counted<generated_term_t>(Term::DATUM, r->bt)) {
    term->datum = datum_t(val.c_str());
}

minidriver_t::reql_t::reql_t(minidriver_t *_r, const datum_t &d) :
        r(_r), term(make_counted<generated_term_t>(Term::DATUM, r->bt)) {
    term->datum = d;
}

minidriver_t::reql_t::reql_t(minidriver_t *_r, std::vector<reql_t> &&val) :
        r(_r), term(make_counted<generated_term_t>(Term::MAKE_ARRAY, r->bt)) {
    for (const auto &item : val) {
        term->args.push_back(item.term);
    }
}

minidriver_t::reql_t::reql_t(minidriver_t *_r, dummy_var_t var) :
        r(_r), term(make_counted<generated_term_t>(Term::VAR, r->bt)) {
    counted_t<generated_term_t> arg_term =
        make_counted<generated_term_t>(Term::DATUM, r->bt);
    arg_term->datum = datum_t(static_cast<double>(dummy_var_to_sym(var).value));
    term->args.push_back(arg_term);
}

minidriver_t::reql_t minidriver_t::boolean(bool b) {
    return reql_t(this, datum_t(datum_t::construct_boolean_t(), b));
}

raw_term_t minidriver_t::reql_t::root_term() {
    return raw_term_t(term);
}

void minidriver_t::reql_t::copy_optargs_from_term(const raw_term_t &from) {
    from.each_optarg([&](const raw_term_t &optarg, const std::string &name) {
            add_arg(r->optarg(name, optarg));
        });
}

void minidriver_t::reql_t::copy_args_from_term(const raw_term_t &from,
                                               size_t start_index) {
    for (size_t i = start_index; i < from.num_args(); ++i) {
        add_arg(from.arg(i));
    }
}

minidriver_t::reql_t minidriver_t::fun(const minidriver_t::reql_t &body) {
    return reql_t(this, Term::FUNC, array(), std::move(body));
}

minidriver_t::reql_t minidriver_t::fun(dummy_var_t a,
                                       const minidriver_t::reql_t &body) {
    return reql_t(this, Term::FUNC,
                  reql_t(this, Term::MAKE_ARRAY, dummy_var_to_sym(a).value),
                  std::move(body));
}

minidriver_t::reql_t minidriver_t::fun(dummy_var_t a,
                                       dummy_var_t b,
                                       const minidriver_t::reql_t &body) {
    return reql_t(this, Term::FUNC,
                  reql_t(this, Term::MAKE_ARRAY,
                         dummy_var_to_sym(a).value,
                         dummy_var_to_sym(b).value),
                  std::move(body));
}

minidriver_t::reql_t minidriver_t::null() {
    return reql_t(this, datum_t::null());
}

minidriver_t::reql_t minidriver_t::reql_t::operator !() {
    return std::move(*this).call(Term::NOT);
}

minidriver_t::reql_t minidriver_t::reql_t::do_(dummy_var_t arg,
                                               const minidriver_t::reql_t &body) {
    return r->fun(arg, std::move(body))(std::move(*this));
}

minidriver_t::reql_t minidriver_t::db(const std::string &name) {
    return reql_t(this, Term::DB, expr(name));
}

minidriver_t::reql_t minidriver_t::var(dummy_var_t v) {
    return reql_t(this, Term::VAR, static_cast<double>(dummy_var_to_sym(v).value));
}

minidriver_t::reql_t minidriver_t::var(const sym_t &v) {
    return reql_t(this, Term::VAR, static_cast<double>(v.value));
}

sym_t minidriver_t::dummy_var_to_sym(dummy_var_t dummy_var) {
    // dummy_var values are non-negative, we map them to small negative values.
    return sym_t(-1 - static_cast<int64_t>(dummy_var));
}

} // namespace ql
