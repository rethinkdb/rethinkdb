#include "rdb_protocol/op.hpp"

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

optargspec_t optargspec_t::make_object() {
    return optargspec_t(-1, 0);
}
bool optargspec_t::is_make_object() const {
    return num_legal_args < 0;
}
bool optargspec_t::contains(const std::string &key) const {
    r_sanity_check(!is_make_object());
    for (int i = 0; i < num_legal_args; ++i) if (key == legal_args[i]) return true;
    return false;
}


op_term_t::op_term_t(env_t *env, const Term *term,
                     argspec_t argspec, optargspec_t optargspec)
    : term_t(env, term) {
    for (int i = 0; i < term->args_size(); ++i) {
        term_t *t = compile_term(env, &term->args(i));
        args.push_back(t);
    }
    rcheck(argspec.contains(args.size()),
           strprintf("Expected %s but found %zu.",
                     argspec.print().c_str(), args.size()));

    for (int i = 0; i < term->optargs_size(); ++i) {
        const Term_AssocPair *ap = &term->optargs(i);
        if (!optargspec.is_make_object()) {
            rcheck(optargspec.contains(ap->key()),
                   strprintf("Unrecognized optional argument `%s`.", ap->key().c_str()));
        }
        rcheck(optargs.count(ap->key()) == 0,
               strprintf("Duplicate %s: %s",
                         (term->type() == Term_TermType_MAKE_OBJ ?
                          "object key" : "optional argument"),
                         ap->key().c_str()));
        term_t *t = compile_term(env, &ap->val());
        optargs.insert(ap->key(), t);
    }
}
op_term_t::~op_term_t() { }

size_t op_term_t::num_args() const { return args.size(); }
val_t *op_term_t::arg(size_t i) {
    rcheck(i < num_args(), strprintf("Index out of range: %zu", i));
    return args[i].eval();
}

val_t *op_term_t::optarg(const std::string &key, val_t *def/*ault*/) {
    boost::ptr_map<const std::string, term_t>::iterator it = optargs.find(key);
    if (it != optargs.end()) return it->second->eval();
    val_t *v = env->get_optarg(key);
    return v ? v : def;
}

bool op_term_t::is_deterministic_impl() const {
    for (size_t i = 0; i < num_args(); ++i) {
        if (!args[i].is_deterministic()) return false;
    }
    for (boost::ptr_map<const std::string, term_t>::const_iterator
             it = optargs.begin(); it != optargs.end(); ++it) {
        if (!it->second->is_deterministic()) return false;
    }
    return true;
}

} //namespace ql
