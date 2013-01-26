#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"
#include "rdb_protocol/pb_utils.hpp"

namespace ql {

class obj_or_seq_op_term_t : public op_term_t {
public:
    obj_or_seq_op_term_t(env_t *env, const Term2 *term, argspec_t argspec)
        : op_term_t(env, term, argspec) {
        int varnum = env->gensym();
        Term2 *body = pb::set_func(&map_func, varnum);
        *body = *term;
        pb::set_var(pb::reset(body->mutable_args(0)), varnum);
    }
private:
    virtual val_t *obj_eval() = 0;
    virtual val_t *eval_impl() {
        if (arg(0)->get_type().is_convertible(val_t::type_t::DATUM)) {
            if (arg(0)->as_datum()->get_type() == datum_t::R_OBJECT) return obj_eval();
        }
        if (arg(0)->get_type().is_convertible(val_t::type_t::SEQUENCE)) {
            return new_val(arg(0)->as_seq()->map(env->new_func(&map_func)));
        }
        rfail("Cannot perform %s on a non-object non-sequence.", name());
        unreachable();
    }

    Term2 map_func;
};

class pluck_term_t : public obj_or_seq_op_term_t {
public:
    pluck_term_t(env_t *env, const Term2 *term) :
        obj_or_seq_op_term_t(env, term, argspec_t(1, -1)) { }
private:
    virtual val_t *obj_eval() {
        const datum_t *obj = arg(0)->as_datum();
        r_sanity_check(obj->get_type() == datum_t::R_OBJECT);

        scoped_ptr_t<datum_t> out(new datum_t(datum_t::R_OBJECT));
        for (size_t i = 1; i < num_args(); ++i) {
            const std::string &key = arg(i)->as_datum()->as_str();
            const datum_t *el = obj->el(key, NOTHROW);
            if (el) {
                bool conflict = out->add(key, el);
                r_sanity_check(!conflict);
            }
        }
        return new_val(out.release());
    }
    RDB_NAME("pluck")
};

class without_term_t : public obj_or_seq_op_term_t {
public:
    without_term_t(env_t *env, const Term2 *term) :
        obj_or_seq_op_term_t(env, term, argspec_t(1, -1)) { }
private:
    virtual val_t *obj_eval() {
        const datum_t *obj = arg(0)->as_datum();
        r_sanity_check(obj->get_type() == datum_t::R_OBJECT);

        scoped_ptr_t<datum_t> out(new datum_t(obj->as_object()));
        for (size_t i = 1; i < num_args(); ++i) {
            const std::string &key = arg(i)->as_datum()->as_str();
            UNUSED bool b = out->del(key);
        }
        return new_val(out.release());
    }
    RDB_NAME("without")
};

} // namespace ql

