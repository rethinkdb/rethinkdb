#include "rdb_protocol/op.hpp"

namespace ql {
op_term_t::op_term_t(env_t *env, const Term2 *term,
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
        const Term2_AssocPair *ap = &term->optargs(i);
        if (!optargspec.is_make_object()) {
            rcheck(optargspec.contains(ap->key()),
                   strprintf("Unrecognized optional argument `%s`.", ap->key().c_str()));
        }
        rcheck(optargs.count(ap->key()) == 0,
               strprintf("Duplicate %s: %s",
                         (term->type() == Term2_TermType_MAKE_OBJ ?
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
    return args[i].eval(use_cached_val);
}

val_t *op_term_t::optarg(const std::string &key, val_t *def/*ault*/) {
    boost::ptr_map<const std::string, term_t>::iterator it = optargs.find(key);
    if (it != optargs.end()) return it->second->eval(use_cached_val);
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
