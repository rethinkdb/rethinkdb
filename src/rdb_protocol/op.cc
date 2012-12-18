#include "rdb_protocol/op.hpp"

namespace ql {
op_term_t::op_term_t(env_t *env, const Term2 *term,
                     argspec_t argspec, optargspec_t optargspec)
    : term_t(env) {
    for (int i = 0; i < term->args_size(); ++i) {
        term_t *t = compile_term(env, &term->args(i));
        t->set_bt(i);
        args.push_back(t);
    }
    rcheck(argspec.contains(args.size()),
           strprintf("Wrong number of arguments: %lu (expected %s)",
                     args.size(), argspec.print()));

    for (int i = 0; i < term->optargs_size(); ++i) {
        const Term2_AssocPair *ap = &term->optargs(i);
        if (!optargspec.is_make_object()) {
            rcheck(optargspec.contains(ap->key()),
                   strprintf("Unrecognized optional argument: %s", ap->key().c_str()));
        }
        rcheck(optargs.count(ap->key()) == 0,
               strprintf("Duplicate %s: %s",
                         (term->type() == Term2_TermType_MAKE_OBJ ?
                          "object key" : "optional argument"),
                         ap->key().c_str()));
        term_t *t = compile_term(env, &ap->val());
        t->set_bt(ap->key());
        optargs.insert(ap->key(), t);
    }
}
op_term_t::~op_term_t() { }

size_t op_term_t::num_args() const { return args.size(); }
val_t *op_term_t::arg(size_t i) {
    rcheck(i < num_args(), strprintf("Index out of range: %lu", i));
    return args[i].eval(use_cached_val);
}

val_t *op_term_t::optarg(const std::string &key, val_t *def/*ault*/) {
    boost::ptr_map<const std::string, term_t>::iterator it = optargs.find(key);
    if (it == optargs.end()) return def;
    return it->second->eval(use_cached_val);
}

// simple_op_term_t::simple_op_term_t(env_t *env, const Term2 *term)
//     : op_term_t(env, term) { }
// simple_op_term_t::~simple_op_term_t() { }
// val_t *simple_op_term_t::eval_impl() {
//     check_no_optargs();
//     std::vector<val_t *> new_args;
//     for (size_t i = 0; i < num_args(); ++i) {
//         new_args.push_back(arg(i)->eval(use_cached_val));
//     }
//     return simple_call_impl(&new_args);
// }

} //namespace ql
