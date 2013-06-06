#include "rdb_protocol/terms/terms.hpp"

#include <string>

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/pb_utils.hpp"

namespace ql {

// This term is used for functions that are polymorphic on objects and
// sequences, like `pluck`.  It will handle the polymorphism; terms inheriting
// from it just need to implement evaluation on objects (`obj_eval`).
class obj_or_seq_op_term_t : public op_term_t {
public:
    obj_or_seq_op_term_t(env_t *env, protob_t<const Term> term, argspec_t argspec)
        : op_term_t(env, term, argspec), map_func(make_counted_term()) {
        int varnum = env->gensym();
        Term *body = pb::set_func(map_func.get(), varnum);
        *body = *term;
        pb::set_var(pb::reset(body->mutable_args(0)), varnum);
        prop_bt(map_func.get());
    }
private:
    virtual counted_t<val_t> obj_eval() = 0;
    virtual counted_t<val_t> eval_impl() {
        counted_t<val_t> v0 = arg(0);
        if (v0->get_type().is_convertible(val_t::type_t::DATUM)) {
            if (v0->as_datum()->get_type() == datum_t::R_OBJECT) return obj_eval();
        }
        if (v0->get_type().is_convertible(val_t::type_t::SEQUENCE)) {
            return new_val(v0->as_seq()->map(make_counted<func_t>(env, map_func)));
        }
        rfail_typed_target(
            v0, "Cannot perform %s on a non-object non-sequence.", name());
        unreachable();
    }

    protob_t<Term> map_func;
};

class pluck_term_t : public obj_or_seq_op_term_t {
public:
    pluck_term_t(env_t *env, protob_t<const Term> term) :
        obj_or_seq_op_term_t(env, term, argspec_t(1, -1)) { }
private:
    virtual counted_t<val_t> obj_eval() {
        counted_t<const datum_t> obj = arg(0)->as_datum();
        r_sanity_check(obj->get_type() == datum_t::R_OBJECT);

        scoped_ptr_t<datum_t> out(new datum_t(datum_t::R_OBJECT));
        for (size_t i = 1; i < num_args(); ++i) {
            const std::string &key = arg(i)->as_str();
            counted_t<const datum_t> el = obj->get(key, NOTHROW);
            if (el.has()) {
                bool conflict = out->add(key, el);
                r_sanity_check(!conflict);
            }
        }
        return new_val(counted_t<const datum_t>(out.release()));
    }
    virtual const char *name() const { return "pluck"; }
};

class without_term_t : public obj_or_seq_op_term_t {
public:
    without_term_t(env_t *env, protob_t<const Term> term) :
        obj_or_seq_op_term_t(env, term, argspec_t(1, -1)) { }
private:
    virtual counted_t<val_t> obj_eval() {
        counted_t<const datum_t> obj = arg(0)->as_datum();
        r_sanity_check(obj->get_type() == datum_t::R_OBJECT);

        scoped_ptr_t<datum_t> out(new datum_t(obj->as_object()));
        for (size_t i = 1; i < num_args(); ++i) {
            const std::string &key = arg(i)->as_str();
            UNUSED bool b = out->delete_key(key);
        }
        return new_val(counted_t<const datum_t>(out.release()));
    }
    virtual const char *name() const { return "without"; }
};

class merge_term_t : public obj_or_seq_op_term_t {
public:
    merge_term_t(env_t *env, protob_t<const Term> term) :
        obj_or_seq_op_term_t(env, term, argspec_t(1, -1)) { }
private:
    virtual counted_t<val_t> obj_eval() {
        counted_t<const datum_t> d = arg(0)->as_datum();
        for (size_t i = 1; i < num_args(); ++i) {
            d = d->merge(arg(i)->as_datum());
        }
        return new_val(d);
    }
    virtual const char *name() const { return "merge"; }
};


counted_t<term_t> make_pluck_term(env_t *env, protob_t<const Term> term) {
    return make_counted<pluck_term_t>(env, term);
}
counted_t<term_t> make_without_term(env_t *env, protob_t<const Term> term) {
    return make_counted<without_term_t>(env, term);
}
counted_t<term_t> make_merge_term(env_t *env, protob_t<const Term> term) {
    return make_counted<merge_term_t>(env, term);
}

} // namespace ql
