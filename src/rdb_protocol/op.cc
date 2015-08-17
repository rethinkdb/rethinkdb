// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/op.hpp"

#include "rdb_protocol/func.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/validate.hpp"

#include "debug.hpp"

namespace ql {
argspec_t::argspec_t(int n) : min(n), max(n), eval_flags(NO_FLAGS) { }
argspec_t::argspec_t(int _min, int _max)
    : min(_min), max(_max), eval_flags(NO_FLAGS) { }
argspec_t::argspec_t(int _min, int _max, eval_flags_t _eval_flags)
    : min(_min), max(_max), eval_flags(_eval_flags) { }
std::string argspec_t::print() const {
    if (min == max) {
        return strprintf("%d argument%s", min, (min == 1 ? "" : "s"));
    } else if (max == -1) {
        return strprintf("%d or more arguments", min);
    } else {
        return strprintf("between %d and %d arguments", min, max);
    }
}
bool argspec_t::contains(int n) const {
    return min <= n && (max < 0 || n <= max);
}

optargspec_t::optargspec_t(std::initializer_list<const char *> args) {
    init(args.size(), args.begin());
    for (auto i = legal_args.cbegin(); i != legal_args.cend(); ++i) {
        r_sanity_check(optarg_is_valid(*i), "Optarg `%s` not listed in "
                                            "`acceptable_keys`", i->c_str());
    }
}


void optargspec_t::init(int num_args, const char *const *args) {
    for (int i = 0; i < num_args; ++i) {
        legal_args.insert(args[i]);
    }
}

bool optargspec_t::contains(const std::string &key) const {
    return legal_args.count(key) != 0;
}

optargspec_t optargspec_t::with(std::initializer_list<const char *> args) const {
    optargspec_t ret(args);
    ret.legal_args.insert(legal_args.begin(), legal_args.end());
    return ret;
}

class faux_term_t : public runtime_term_t {
public:
    faux_term_t(backtrace_id_t bt, datum_t _d)
        : runtime_term_t(bt), d(std::move(_d)) { }
    const char *name() const final { return "<EXPANDED FROM r.args>"; }
private:
    scoped_ptr_t<val_t> term_eval(scope_env_t *, eval_flags_t) const final {
        return new_val(d);
    }
    datum_t d;
};


class arg_terms_t : public bt_rcheckable_t {
public:
    arg_terms_t(const protob_t<const Term> _src, argspec_t _argspec,
                std::vector<counted_t<const term_t> > _original_args);
    // Evals the r.args arguments, and returns the expanded argument list.
    argvec_t start_eval(scope_env_t *env, eval_flags_t flags) const;

    const std::vector<counted_t<const term_t> > &get_original_args() const {
        return original_args;
    }
private:
    const protob_t<const Term> src;
    const argspec_t argspec;
    const std::vector<counted_t<const term_t> > original_args;

    DISABLE_COPYING(arg_terms_t);
};

arg_terms_t::arg_terms_t(const protob_t<const Term> _src, argspec_t _argspec,
                         std::vector<counted_t<const term_t> > _original_args)
    : bt_rcheckable_t(backtrace_id_t(_src.get())),
      src(std::move(_src)),
      argspec(std::move(_argspec)),
      original_args(std::move(_original_args)) {
    for (auto it = original_args.begin(); it != original_args.end(); ++it) {
        if ((*it)->get_src()->type() == Term::ARGS) {
            return;
        }
    }
    // We check this here *and* in `start_eval` because if `r.args` isn't in
    // play we want to give a compile-time error.
    rcheck(argspec.contains(original_args.size()),
           base_exc_t::LOGIC,
           strprintf("Expected %s but found %zu.",
                     argspec.print().c_str(), original_args.size()));
}

argvec_t arg_terms_t::start_eval(scope_env_t *env, eval_flags_t flags) const {
    eval_flags_t new_flags = static_cast<eval_flags_t>(
        flags | argspec.get_eval_flags());
    std::vector<counted_t<const runtime_term_t> > args;
    for (auto it = original_args.begin(); it != original_args.end(); ++it) {
        if ((*it)->get_src()->type() == Term::ARGS) {
            scoped_ptr_t<val_t> v = (*it)->eval(env, new_flags);
            datum_t d = v->as_datum();
            for (size_t i = 0; i < d.arr_size(); ++i) {
                args.push_back(make_counted<faux_term_t>(backtrace(), d.get(i)));
            }
        } else {
            args.push_back(counted_t<const runtime_term_t>(*it));
        }
    }
    rcheck(argspec.contains(args.size()),
           base_exc_t::LOGIC,
           strprintf("Expected %s but found %zu.",
                     argspec.print().c_str(), args.size()));
    return argvec_t(std::move(args));
}

argvec_t::argvec_t(std::vector<counted_t<const runtime_term_t> > &&v)
    : vec(std::move(v)) { }

counted_t<const runtime_term_t> argvec_t::remove(size_t i) {
    r_sanity_check(i < vec.size());
    r_sanity_check(vec[i].has());
    counted_t<const runtime_term_t> ret;
    ret.swap(vec[i]);
    return ret;
}


size_t args_t::num_args() const {
    return argv.size();
}

scoped_ptr_t<val_t> args_t::arg(scope_env_t *env, size_t i,
                             eval_flags_t flags) {
    if (i == 0 && arg0.has()) {
        scoped_ptr_t<val_t> v = std::move(arg0);
        arg0.reset();
        return v;
    } else {
        return argv.remove(i)->eval(env, flags);
    }
}

scoped_ptr_t<val_t> args_t::optarg(scope_env_t *env, const std::string &key) const {
    return op_term->optarg(env, key);
}

args_t::args_t(const op_term_t *_op_term, argvec_t _argv)
    : op_term(_op_term), argv(std::move(_argv)) { }
args_t::args_t(const op_term_t *_op_term,
               argvec_t _argv,
               scoped_ptr_t<val_t> _arg0)
    : op_term(_op_term), argv(std::move(_argv)), arg0(std::move(_arg0)) { }


op_term_t::op_term_t(compile_env_t *env, const protob_t<const Term> term,
                     argspec_t argspec, optargspec_t optargspec)
        : term_t(term) {
    std::vector<counted_t<const term_t> > original_args;
    original_args.reserve(term->args_size());
    for (int i = 0; i < term->args_size(); ++i) {
        counted_t<const term_t> t
            = compile_term(env, term.make_child(&term->args(i)));
        original_args.push_back(t);
    }
    arg_terms.init(new arg_terms_t(term, std::move(argspec), std::move(original_args)));

    for (int i = 0; i < term->optargs_size(); ++i) {
        const Term_AssocPair *ap = &term->optargs(i);
        rcheck_src(backtrace_id_t(&ap->val()), optargspec.contains(ap->key()),
                   base_exc_t::LOGIC, strprintf("Unrecognized optional argument `%s`.",
                                                  ap->key().c_str()));
        counted_t<const term_t> t =
            compile_term(env, term.make_child(&ap->val()));
        auto res = optargs.insert(std::make_pair(ap->key(), std::move(t)));
        rcheck_src(backtrace_id_t(&ap->val()), res.second,
                   base_exc_t::LOGIC, strprintf("Duplicate optional argument: %s",
                                                  ap->key().c_str()));
    }
}
op_term_t::~op_term_t() { }

scoped_ptr_t<val_t> op_term_t::term_eval(scope_env_t *env,
                                         eval_flags_t eval_flags) const {
    argvec_t argv = arg_terms->start_eval(env, eval_flags);
    if (can_be_grouped()) {
        counted_t<grouped_data_t> gd;
        scoped_ptr_t<val_t> arg0;
        maybe_grouped_data(env, &argv, eval_flags, &gd, &arg0);
        if (gd.has()) {
            // (arg0 is empty, because maybe_grouped_data sets at most one of gd and
            // arg0, so we don't have to worry about re-evaluating it.
            counted_t<grouped_data_t> out(new grouped_data_t());
            // We're processing gd into another grouped_data_t -- so gd's order
            // doesn't matter.
            for (auto kv = gd->begin(); kv != gd->end(); ++kv) {
                arg_terms->start_eval(env, eval_flags);
                args_t args(this, argv, make_scoped<val_t>(kv->second, backtrace()));
                (*out)[kv->first] = eval_impl(env, &args, eval_flags)->as_datum();
            }
            return make_scoped<val_t>(out, backtrace());
        } else {
            args_t args(this, std::move(argv), std::move(arg0));
            return eval_impl(env, &args, eval_flags);
        }
    } else {
        args_t args(this, std::move(argv));
        return eval_impl(env, &args, eval_flags);
    }
}

bool op_term_t::can_be_grouped() const { return true; }
bool op_term_t::is_grouped_seq_op() const { return false; }

scoped_ptr_t<val_t> op_term_t::optarg(scope_env_t *env, const std::string &key) const {
    std::map<std::string, counted_t<const term_t> >::const_iterator it
        = optargs.find(key);
    if (it != optargs.end()) {
        return it->second->eval(env);
    }
    // returns scoped_ptr_t<val_t>() if the key isn't found
    return env->env->get_optarg(env->env, key);
}

counted_t<func_term_t> op_term_t::lazy_literal_optarg(compile_env_t *env, const std::string &key) const {
    std::map<std::string, counted_t<const term_t> >::const_iterator it = optargs.find(key);
    if (it != optargs.end()) {
        protob_t<Term> func(make_counted_term());
        r::fun(r::expr(*it->second->get_src().get())).swap(*func.get());
        return make_counted<func_term_t>(env, func);
    }
    return counted_t<func_term_t>();
}

void accumulate_all_captures(
        const std::map<std::string, counted_t<const term_t> > &optargs,
        var_captures_t *captures) {
    for (auto it = optargs.begin(); it != optargs.end(); ++it) {
        it->second->accumulate_captures(captures);
    }
}

void op_term_t::accumulate_captures(var_captures_t *captures) const {
    const std::vector<counted_t<const term_t> > &original_args
        = arg_terms->get_original_args();
    for (auto it = original_args.begin(); it != original_args.end(); ++it) {
        (*it)->accumulate_captures(captures);
    }
    accumulate_all_captures(optargs, captures);
}

bool all_are_deterministic(
        const std::map<std::string, counted_t<const term_t> > &optargs) {
    for (auto it = optargs.begin(); it != optargs.end(); ++it) {
        if (!it->second->is_deterministic()) {
            return false;
        }
    }
    return true;
}

bool op_term_t::is_deterministic() const {
    const std::vector<counted_t<const term_t> > &original_args
        = arg_terms->get_original_args();
    for (size_t i = 0; i < original_args.size(); ++i) {
        if (!original_args[i]->is_deterministic()) {
            return false;
        }
    }
    return all_are_deterministic(optargs);
}

void op_term_t::maybe_grouped_data(scope_env_t *env,
                                   argvec_t *argv,
                                   eval_flags_t flags,
                                   counted_t<grouped_data_t> *grouped_data_out,
                                   scoped_ptr_t<val_t> *arg0_out) const {
    if (argv->empty()) {
        grouped_data_out->reset();
        arg0_out->reset();
    } else {
        scoped_ptr_t<val_t> arg0 = argv->remove(0)->eval(env, flags);

        counted_t<grouped_data_t> gd = is_grouped_seq_op()
            ? arg0->maybe_as_grouped_data()
            : arg0->maybe_as_promiscuous_grouped_data(env->env);

        if (gd.has()) {
            *grouped_data_out = std::move(gd);
            arg0_out->reset();
        } else {
            grouped_data_out->reset();
            *arg0_out = std::move(arg0);
        }
    }
}

bounded_op_term_t::bounded_op_term_t(compile_env_t *env, protob_t<const Term> term,
                                     argspec_t argspec, optargspec_t optargspec)
    : op_term_t(env, term, argspec,
                optargspec.with({"left_bound", "right_bound"})) { }

bool bounded_op_term_t::is_left_open(scope_env_t *env, args_t *args) const {
    return open_bool(env, args, "left_bound", false);
}

bool bounded_op_term_t::is_right_open(scope_env_t *env, args_t *args) const {
    return open_bool(env, args, "right_bound", true);
}

bool bounded_op_term_t::open_bool(
        scope_env_t *env, args_t *args, const std::string &key, bool def/*ault*/) const {
    scoped_ptr_t<val_t> v = args->optarg(env, key);
    if (!v.has()) {
        return def;
    }
    const datum_string_t &s = v->as_str();
    if (s == "open") {
        return true;
    } else if (s == "closed") {
        return false;
    } else {
        rfail(base_exc_t::LOGIC,
              "Expected `open` or `closed` for optarg `%s` (got `%s`).",
              key.c_str(), v->trunc_print().c_str());
    }
}


} // namespace ql
