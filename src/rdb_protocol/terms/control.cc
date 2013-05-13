#include "rdb_protocol/terms/terms.hpp"

#include <vector>

#include "rdb_protocol/error.hpp"
#include "rdb_protocol/op.hpp"

namespace ql {

// ALL and ANY are written strangely because I originally thought that we could
// have non-boolean values that evaluate to true, but then we decided not to do
// that.

class all_term_t : public op_term_t {
public:
    all_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(1, -1)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        for (size_t i = 0; i < num_args(); ++i) {
            counted_t<val_t> v = arg(i);
            if (!v->as_bool() || i == num_args() - 1) {
                return v;
            }
        }
        unreachable();
    }
    virtual const char *name() const { return "all"; }
};

class any_term_t : public op_term_t {
public:
    any_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(1, -1)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        for (size_t i = 0; i < num_args(); ++i) {
            counted_t<val_t> v = arg(i);
            if (v->as_bool()) {
                return v;
            }
        }
        return new_val_bool(false);
    }
    virtual const char *name() const { return "any"; }
};

class branch_term_t : public op_term_t {
public:
    branch_term_t(env_t *env, protob_t<const Term> term) : op_term_t(env, term, argspec_t(3)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        bool b = arg(0)->as_bool();
        return b ? arg(1) : arg(2);
    }
    virtual const char *name() const { return "branch"; }
};


class funcall_term_t : public op_term_t {
public:
    funcall_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(1, -1)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        counted_t<func_t> f = arg(0)->as_func(IDENTITY_SHORTCUT);
        std::vector<counted_t<const datum_t> > args;
        for (size_t i = 1; i < num_args(); ++i) args.push_back(arg(i)->as_datum());
        return f->call(args);
    }
    virtual const char *name() const { return "funcall"; }
};


counted_t<term_t> make_all_term(env_t *env, protob_t<const Term> term) {
    return make_counted<all_term_t>(env, term);
}
counted_t<term_t> make_any_term(env_t *env, protob_t<const Term> term) {
    return make_counted<any_term_t>(env, term);
}
counted_t<term_t> make_branch_term(env_t *env, protob_t<const Term> term) {
    return make_counted<branch_term_t>(env, term);
}
counted_t<term_t> make_funcall_term(env_t *env, protob_t<const Term> term) {
    return make_counted<funcall_term_t>(env, term);
}

}  // namespace ql
