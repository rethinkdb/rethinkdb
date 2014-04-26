// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/op.hpp"

#include "rdb_protocol/func.hpp"
#include "rdb_protocol/minidriver.hpp"

namespace ql {
argspec_t::argspec_t(int n) : min(n), max(n) { }
argspec_t::argspec_t(int _min, int _max) : min(_min), max(_max) { }
std::string argspec_t::print() {
    if (min == max) {
        return strprintf("%d argument(s)", min);
    } else if (max == -1) {
        return strprintf("%d or more argument(s)", min);
    } else {
        return strprintf("between %d and %d arguments", min, max);
    }
}
bool argspec_t::contains(int n) const {
    return min <= n && (max < 0 || n <= max);
}

optargspec_t::optargspec_t(std::initializer_list<const char *> args) {
    init(args.size(), args.begin());
}


optargspec_t::optargspec_t(bool _is_make_object_val)
    : is_make_object_val(_is_make_object_val) { }

void optargspec_t::init(int num_args, const char *const *args) {
    is_make_object_val = false;
    for (int i = 0; i < num_args; ++i) {
        legal_args.insert(args[i]);
    }
}

optargspec_t optargspec_t::make_object() {
    return optargspec_t(true);
}
bool optargspec_t::is_make_object() const {
    return is_make_object_val;
}
bool optargspec_t::contains(const std::string &key) const {
    r_sanity_check(!is_make_object());
    return legal_args.count(key) != 0;
}

optargspec_t optargspec_t::with(std::initializer_list<const char *> args) const {
    optargspec_t ret(args);
    ret.legal_args.insert(legal_args.begin(), legal_args.end());
    return ret;
}

class faux_term_t : public term_t {
public:
    faux_term_t(protob_t<const Term> src, counted_t<const datum_t> _d)
        : term_t(std::move(src)), d(std::move(_d)) { }
    virtual const char *name() const { return "<EXPANDED FROM r.args>"; }
    virtual bool is_deterministic() const { return true; }
    virtual void accumulate_captures(var_captures_t *) const { }
private:
    virtual counted_t<val_t> term_eval(scope_env_t *, eval_flags_t) {
        return new_val(d);
    }
    counted_t<const datum_t> d;
};


class args_t : public pb_rcheckable_t {
public:
    args_t(protob_t<const Term> _src,
           argspec_t _argspec,
           std::vector<counted_t<term_t> > _original_args);
    // Must be called before `eval_arg`.
    void start_eval(scope_env_t *env, eval_flags_t flags,
                    counted_t<val_t> _arg0 = counted_t<val_t>());
    counted_t<val_t> eval_arg(scope_env_t *env, size_t i, eval_flags_t flags);
    size_t size() { return args.size(); }

    counted_t<grouped_data_t> maybe_grouped_data(
        scope_env_t *env, bool is_grouped_seq_op);
    const std::vector<counted_t<term_t> > &get_original_args() {
        return original_args;
    }
private:
    counted_t<term_t> get(size_t i);
    protob_t<const Term> src;
    argspec_t argspec;
    counted_t<val_t> arg0;
    std::vector<counted_t<term_t> > original_args;
    std::vector<counted_t<term_t> > args;
};

counted_t<grouped_data_t> args_t::maybe_grouped_data(
    scope_env_t *env, bool is_grouped_seq_op) {
    counted_t<grouped_data_t> gd;
    if (args.size() != 0) {
        r_sanity_check(arg0.has());
        gd = is_grouped_seq_op
            ? arg0->maybe_as_grouped_data()
            : arg0->maybe_as_promiscuous_grouped_data(env->env);
    }
    if (gd.has()) {
        arg0.reset();
    }
    return std::move(gd);
}

args_t::args_t(protob_t<const Term> _src,
               argspec_t _argspec,
               std::vector<counted_t<term_t> > _original_args)
    : pb_rcheckable_t(get_backtrace(_src)),
      src(std::move(_src)),
      argspec(std::move(_argspec)),
      original_args(std::move(_original_args)) { }

void args_t::start_eval(scope_env_t *env, eval_flags_t flags,
                        counted_t<val_t> _arg0) {
    args.clear();
    for (auto it = original_args.begin(); it != original_args.end(); ++it) {
        if ((*it)->get_src()->type() == Term::ARGS) {
            // RSI: check how this works with LITERAL_OK
            counted_t<val_t> v = (*it)->eval(env, flags);
            counted_t<const datum_t> d = v->as_datum();
            for (size_t i = 0; i < d->size(); ++i) {
                args.push_back(counted_t<term_t>(new faux_term_t(src, d->get(i))));
            }
        } else {
            args.push_back(*it);
        }
    }
    rcheck(argspec.contains(args.size()),
           base_exc_t::GENERIC,
           strprintf("Expected %s but found %zu.",
                     argspec.print().c_str(), args.size()));
    if (args.size() != 0) {
        arg0 = _arg0.has() ? std::move(_arg0) : get(0)->eval(env, flags);
    }
}

counted_t<val_t> args_t::eval_arg(scope_env_t *env, size_t i, eval_flags_t flags) {
    if (i == 0) {
        r_sanity_check(arg0.has());
        counted_t<val_t> v;
        v.swap(arg0);
        return std::move(v);
    } else {
        return get(i)->eval(env, flags);
    }
}

counted_t<term_t> args_t::get(size_t i) {
    r_sanity_check(i < args.size());
    r_sanity_check(args[i].has());
    counted_t<term_t> ret;
    ret.swap(args[i]);
    r_sanity_check(!args[i].has());
    return std::move(ret);
}

op_term_t::op_term_t(compile_env_t *env, protob_t<const Term> term,
                     argspec_t argspec, optargspec_t optargspec)
    : term_t(term) {
    std::vector<counted_t<term_t> > original_args;
    original_args.reserve(term->args_size());
    for (int i = 0; i < term->args_size(); ++i) {
        counted_t<term_t> t = compile_term(env, term.make_child(&term->args(i)));
        original_args.push_back(t);
    }
    args.init(new args_t(term, std::move(argspec), std::move(original_args)));

    for (int i = 0; i < term->optargs_size(); ++i) {
        const Term_AssocPair *ap = &term->optargs(i);
        if (!optargspec.is_make_object()) {
            rcheck(optargspec.contains(ap->key()),
                   base_exc_t::GENERIC,
                   strprintf("Unrecognized optional argument `%s`.",
                             ap->key().c_str()));
        }
        rcheck(optargs.count(ap->key()) == 0,
               base_exc_t::GENERIC,
               strprintf("Duplicate %s: %s",
                         (term->type() == Term_TermType_MAKE_OBJ ?
                          "object key" : "optional argument"),
                         ap->key().c_str()));
        counted_t<term_t> t = compile_term(env, term.make_child(&ap->val()));
        optargs.insert(std::make_pair(ap->key(), t));
    }
}
op_term_t::~op_term_t() { }

size_t op_term_t::num_args() const { return args->size(); }
counted_t<val_t> op_term_t::arg(scope_env_t *env, size_t i, eval_flags_t flags) {
    return args->eval_arg(env, i, flags);
}

counted_t<val_t> op_term_t::term_eval(scope_env_t *env, eval_flags_t eval_flags) {
    args->start_eval(env, eval_flags);
    counted_t<val_t> ret;
    if (can_be_grouped()) {
        counted_t<grouped_data_t> gd
            = args->maybe_grouped_data(env, is_grouped_seq_op());
        if (gd.has()) {
            counted_t<grouped_data_t> out(new grouped_data_t());
            for (auto kv = gd->begin(); kv != gd->end(); ++kv) {
                args->start_eval(env, eval_flags,
                                 make_counted<val_t>(kv->second, backtrace()));
                (*out)[kv->first] = eval_impl(env, eval_flags)->as_datum();
            }
            return make_counted<val_t>(out, backtrace());
        }
    }
    return eval_impl(env, eval_flags);
}

bool op_term_t::can_be_grouped() { return true; }
bool op_term_t::is_grouped_seq_op() { return false; }

counted_t<val_t> op_term_t::optarg(scope_env_t *env, const std::string &key) {
    std::map<std::string, counted_t<term_t> >::iterator it = optargs.find(key);
    if (it != optargs.end()) {
        return it->second->eval(env);
    }
    // returns counted_t<val_t>() if the key isn't found
    return env->env->global_optargs.get_optarg(env->env, key);
}

counted_t<func_term_t> op_term_t::lazy_literal_optarg(compile_env_t *env, const std::string &key) {
    std::map<std::string, counted_t<term_t> >::iterator it = optargs.find(key);
    if (it != optargs.end()) {
        protob_t<Term> func(make_counted_term());
        r::fun(r::expr(*it->second->get_src().get())).swap(*func.get());
        return make_counted<func_term_t>(env, func);
    }
    return counted_t<func_term_t>();
}

// RSI: WTF does this do?
void op_term_t::accumulate_captures(var_captures_t *captures) const {
    const std::vector<counted_t<term_t> > &original_args
        = args->get_original_args();
    for (auto it = original_args.begin(); it != original_args.end(); ++it) {
        (*it)->accumulate_captures(captures);
    }
    for (auto it = optargs.begin(); it != optargs.end(); ++it) {
        it->second->accumulate_captures(captures);
    }
    return;
}


bool op_term_t::is_deterministic() const {
    const std::vector<counted_t<term_t> > &original_args
        = args->get_original_args();
    for (size_t i = 0; i < original_args.size(); ++i) {
        if (!original_args[i]->is_deterministic()) {
            return false;
        }
    }
    for (auto it = optargs.begin(); it != optargs.end(); ++it) {
        if (!it->second->is_deterministic()) {
            return false;
        }
    }
    return true;
}

bounded_op_term_t::bounded_op_term_t(compile_env_t *env, protob_t<const Term> term,
                                     argspec_t argspec, optargspec_t optargspec)
    : op_term_t(env, term, argspec,
                optargspec.with({"left_bound", "right_bound"})) { }

bool bounded_op_term_t::left_open(scope_env_t *env) {
    return open_bool(env, "left_bound", false);
}

bool bounded_op_term_t::right_open(scope_env_t *env) {
    return open_bool(env, "right_bound", true);
}

bool bounded_op_term_t::open_bool(scope_env_t *env, const std::string &key, bool def/*ault*/) {
    counted_t<val_t> v = optarg(env, key);
    if (!v.has()) return def;
    const wire_string_t &s = v->as_str();
    if (s == "open") {
        return true;
    } else if (s == "closed") {
        return false;
    } else {
        rfail(base_exc_t::GENERIC,
              "Expected `open` or `closed` for optarg `%s` (got `%s`).",
              key.c_str(), v->trunc_print().c_str());
    }
}


} // namespace ql
