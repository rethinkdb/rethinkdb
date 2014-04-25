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

op_term_t::op_term_t(compile_env_t *env, protob_t<const Term> term,
                     argspec_t argspec, optargspec_t optargspec)
    : term_t(term), arg_verifier(NULL) {
    args.reserve(term->args_size());
    for (int i = 0; i < term->args_size(); ++i) {
        counted_t<term_t> t = compile_term(env, term.make_child(&term->args(i)));
        args.push_back(t);
    }

    rcheck(argspec.contains(args.size()),
           base_exc_t::GENERIC,
           strprintf("Expected %s but found %zu.",
                     argspec.print().c_str(), args.size()));

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
op_term_t::~op_term_t() { r_sanity_check(arg_verifier == NULL); }

// We use this to detect double-evals.  (We've had several problems with those
// in the past.)
class arg_verifier_t {
public:
    arg_verifier_t(std::vector<counted_t<term_t> > *_args,
                   arg_verifier_t **_slot)
        : args(_args), args_consumed(args->size(), false), slot(_slot) {
        *slot = this;
    }
    ~arg_verifier_t() {
        r_sanity_check(*slot == this);
        *slot = NULL;
    }

    counted_t<term_t> consume(size_t i) {
        r_sanity_check(i < args->size());
        r_sanity_check(!args_consumed[i]);
        args_consumed[i] = true;
        return (*args)[i];
    }

private:
    std::vector<counted_t<term_t> > *args;
    std::vector<bool> args_consumed;
    arg_verifier_t **slot;
};

size_t op_term_t::num_args() const { return args.size(); }
counted_t<val_t> op_term_t::arg(scope_env_t *env, size_t i, eval_flags_t flags) {
    if (i == 0) {
        if (!arg0.has()) arg0 = arg_verifier->consume(0)->eval(env, flags);
        counted_t<val_t> v;
        v.swap(arg0);
        r_sanity_check(!arg0.has());
        return std::move(v);
    } else {
        r_sanity_check(arg_verifier != NULL);
        return arg_verifier->consume(i)->eval(env, flags);
    }
}

counted_t<val_t> op_term_t::term_eval(scope_env_t *env, eval_flags_t eval_flags) {
    scoped_ptr_t<arg_verifier_t> av(new arg_verifier_t(&args, &arg_verifier));
    counted_t<val_t> ret;
    if (num_args() != 0 && can_be_grouped()) {
        arg0 = arg_verifier->consume(0)->eval(env, eval_flags);
        counted_t<grouped_data_t> gd = is_grouped_seq_op()
            ? arg0->maybe_as_grouped_data()
            : arg0->maybe_as_promiscuous_grouped_data(env->env);
        if (gd.has()) {
            counted_t<grouped_data_t> out(new grouped_data_t());
            for (auto kv = gd->begin(); kv != gd->end(); ++kv) {
                arg0 = make_counted<val_t>(kv->second, backtrace());
                (*out)[kv->first] = eval_impl(env, eval_flags)->as_datum();
                av.reset();
                av.init(new arg_verifier_t(&args, &arg_verifier));
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

void op_term_t::accumulate_captures(var_captures_t *captures) const {
    for (auto it = args.begin(); it != args.end(); ++it) {
        (*it)->accumulate_captures(captures);
    }
    for (auto it = optargs.begin(); it != optargs.end(); ++it) {
        it->second->accumulate_captures(captures);
    }
    return;
}


bool op_term_t::is_deterministic() const {
    for (size_t i = 0; i < args.size(); ++i) {
        if (!args[i]->is_deterministic()) {
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

bool bounded_op_term_t::is_left_open(scope_env_t *env) {
    return open_bool(env, "left_bound", false);
}

bool bounded_op_term_t::is_right_open(scope_env_t *env) {
    return open_bool(env, "right_bound", true);
}

bool bounded_op_term_t::open_bool(
    scope_env_t *env, const std::string &key, bool def/*ault*/) {
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
