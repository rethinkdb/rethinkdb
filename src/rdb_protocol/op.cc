#include "rdb_protocol/op.hpp"

namespace ql {
op_term_t::op_term_t(env_t *env, const Term2 *term) : term_t(env) {
    for (int i = 0; i < term->args_size(); ++i) {
        term_t *t = compile_term(env, &term->args(i));
        t->set_bt(i);
        args.push_back(t);
    }
    for (int i = 0; i < term->optargs_size(); ++i) {
        const Term2_AssocPair *ap = &term->optargs(i);
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
term_t *op_term_t::arg(size_t i) {
    rcheck(0 <= i && i < num_args(), strprintf("Index out of range: %lu", i));
    return &args[i];
}

void op_term_t::check_no_optargs() const {
    rcheck(optargs.size() == 0,
           strprintf("Operator %s takes no optional arguments, got %lu.",
                     name(), optargs.size()));
}
void op_term_t::check_only_optargs(int n_keys, const char **keys) const {
    for (boost::ptr_map<const std::string, term_t>::const_iterator
             it = optargs.begin(); it != optargs.end(); ++it) {
        for (int i = 0; i < n_keys; ++i) if (it->first == keys[i]) goto valid_optarg;
        rfail("Unexpected optional argument: %s", it->first.c_str());
    valid_optarg:;
    }
}

term_t *op_term_t::optarg(const std::string &key, term_t *def/*ault*/) {
    boost::ptr_map<const std::string, term_t>::iterator
        it = optargs.find(key);
    if (it == optargs.end()) return def;
    return it->second;
}

simple_op_term_t::simple_op_term_t(env_t *env, const Term2 *term)
    : op_term_t(env, term) { }
simple_op_term_t::~simple_op_term_t() { }
val_t *simple_op_term_t::eval_impl() {
    check_no_optargs();
    std::vector<val_t *> new_args;
    for (size_t i = 0; i < num_args(); ++i) new_args.push_back(arg(i)->eval());
    return simple_call_impl(&new_args);
}
} //namespace ql
