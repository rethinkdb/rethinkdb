#ifndef RDB_PROTOCOL_TERMS_OBJ_OR_SEQ_HPP_
#define RDB_PROTOCOL_TERMS_OBJ_OR_SEQ_HPP_

#include <string>

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"
#include "rdb_protocol/pb_utils.hpp"

namespace ql {

// This term is used for functions that are polymorphic on objects and
// sequences, like `pluck`.  It will handle the polymorphism; terms inheriting
// from it just need to implement evaluation on objects (`obj_eval`).
class obj_or_seq_op_term_t : public op_term_t {
public:
    obj_or_seq_op_term_t(env_t *env, const Term *term, argspec_t argspec)
        : op_term_t(env, term, argspec) {
        int varnum = env->gensym();
        Term *body = pb::set_func(&map_func, varnum);
        *body = *term;
        pb::set_var(pb::reset(body->mutable_args(0)), varnum);
    }
private:
    virtual counted_t<val_t> obj_eval() = 0;
    virtual counted_t<val_t> eval_impl() {
        counted_t<val_t> v0 = arg(0);
        if (v0->get_type().is_convertible(val_t::type_t::DATUM)) {
            if (v0->as_datum()->get_type() == datum_t::R_OBJECT) return obj_eval();
        }
        if (v0->get_type().is_convertible(val_t::type_t::SEQUENCE)) {
            return new_val(v0->as_seq()->map(env->new_func(&map_func)));
        }
        rfail("Cannot perform %s on a non-object non-sequence.", name());
        unreachable();
    }

    Term map_func;
};

class pluck_term_t : public obj_or_seq_op_term_t {
public:
    pluck_term_t(env_t *env, const Term *term) :
        obj_or_seq_op_term_t(env, term, argspec_t(1, -1)) { }
private:
    virtual counted_t<val_t> obj_eval() {
        counted_t<const datum_t> obj = arg(0)->as_datum();
        r_sanity_check(obj->get_type() == datum_t::R_OBJECT);

        scoped_ptr_t<datum_t> out(new datum_t(datum_t::R_OBJECT));
        for (size_t i = 1; i < num_args(); ++i) {
            const std::string &key = arg(i)->as_str();
            counted_t<const datum_t> el = obj->el(key);
            if (el) {
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
    without_term_t(env_t *env, const Term *term) :
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
    merge_term_t(env_t *env, const Term *term) :
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


} // namespace ql


#endif // RDB_PROTOCOL_TERMS_OBJ_OR_SEQ_HPP_
