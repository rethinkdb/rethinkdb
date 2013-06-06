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
    enum poly_type_t {
        MAP = 0,
        FILTER = 1
    };
    obj_or_seq_op_term_t(env_t *env, protob_t<const Term> term,
                         poly_type_t _poly_type, argspec_t argspec)
        : op_term_t(env, term, argspec), poly_type(_poly_type),
          func(make_counted_term()) {
        int varnum = env->gensym();
        Term *body = pb::set_func(func.get(), varnum);
        *body = *term;
        pb::set_var(pb::reset(body->mutable_args(0)), varnum);
        prop_bt(func.get());
    }
private:
    virtual counted_t<val_t> obj_eval(counted_t<val_t> v0) = 0;
    virtual counted_t<val_t> eval_impl() {
        counted_t<val_t> v0 = arg(0);
        if (v0->get_type().is_convertible(val_t::type_t::DATUM)) {
            if (v0->as_datum()->get_type() == datum_t::R_OBJECT) return obj_eval(v0);
        }
        if (v0->get_type().is_convertible(val_t::type_t::SEQUENCE)) {
            switch (poly_type) {
            case MAP:
                return new_val(v0->as_seq()->map(make_counted<func_t>(env, func)));
            case FILTER:
                return new_val(v0->as_seq()->filter(make_counted<func_t>(env, func)));
            default: unreachable();
            }
            unreachable();
        }
        rfail_typed_target(
            v0, "Cannot perform %s on a non-object non-sequence.", name());
        unreachable();
    }

    poly_type_t poly_type;
    protob_t<Term> func;
};

class pluck_term_t : public obj_or_seq_op_term_t {
public:
    pluck_term_t(env_t *env, protob_t<const Term> term) :
        obj_or_seq_op_term_t(env, term, MAP, argspec_t(1, -1)) { }
private:
    virtual counted_t<val_t> obj_eval(counted_t<val_t> v0) {
        counted_t<const datum_t> obj = v0->as_datum();
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
        obj_or_seq_op_term_t(env, term, MAP, argspec_t(1, -1)) { }
private:
    virtual counted_t<val_t> obj_eval(counted_t<val_t> v0) {
        counted_t<const datum_t> obj = v0->as_datum();
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
        obj_or_seq_op_term_t(env, term, MAP, argspec_t(1, -1)) { }
private:
    virtual counted_t<val_t> obj_eval(counted_t<val_t> v0) {
        counted_t<const datum_t> d = v0->as_datum();
        for (size_t i = 1; i < num_args(); ++i) {
            d = d->merge(arg(i)->as_datum());
        }
        return new_val(d);
    }
    virtual const char *name() const { return "merge"; }
};

class has_fields_term_t : public obj_or_seq_op_term_t {
public:
    has_fields_term_t(env_t *env, protob_t<const Term> term)
        : obj_or_seq_op_term_t(env, term, FILTER, argspec_t(1, -1)) { }
private:
    virtual counted_t<val_t> obj_eval(counted_t<val_t> v0) {
        counted_t<const datum_t> obj = v0->as_datum();
        for (size_t i = 1; i < num_args(); ++i) {
            counted_t<const datum_t> el = obj->get(arg(i)->as_str(), NOTHROW);
            if (!el.has() || el->get_type() == datum_t::R_NULL) {
                return new_val_bool(false);
            }
        }
        return new_val_bool(true);
    }
    virtual const char *name() const { return "has_fields"; }
};

counted_t<term_t> make_has_fields_term(env_t *env, protob_t<const Term> term) {
    return make_counted<has_fields_term_t>(env, term);
}

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
