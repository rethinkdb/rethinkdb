#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"

namespace ql {

class obj_or_seq_op_term_t : public op_term_t {
public:
    obj_or_seq_op_term_t(env_t *env, const Term2 *term, argspec_t argspec)
        : op_term_t(env, term, argspec) {
        map_func.set_type(Term2_TermType_FUNC);

        int varnum = env->gensym();

        Term2 *arg1 = map_func.add_args();
        arg1->set_type(Term2_TermType_DATUM);
        Datum *vars = arg1->mutable_datum();
        vars->set_type(Datum_DatumType_R_ARRAY);
        Datum *var1 = vars->add_r_array();
        var1->set_type(Datum_DatumType_R_NUM);
        var1->set_r_num(varnum);

        Term2 *arg2 = map_func.add_args();
        *arg2 = *term;
        Term2 *arg2_1 = arg2->mutable_args(0);
        *arg2_1 = Term2();
        arg2_1->set_type(Term2_TermType_VAR);
        Term2 *arg2_1_1 = arg2_1->add_args();
        arg2_1_1->set_type(Term2_TermType_DATUM);
        Datum *var1_ref = arg2_1_1->mutable_datum();
        var1_ref->set_type(Datum_DatumType_R_NUM);
        var1_ref->set_r_num(varnum);
    }
private:
    virtual val_t *obj_eval() = 0;
    virtual val_t *eval_impl() {
        if (arg(0)->get_type().is_convertible(val_t::type_t::DATUM)) {
            if (arg(0)->as_datum()->get_type() == datum_t::R_OBJECT) return obj_eval();
        }
        if (arg(0)->get_type().is_convertible(val_t::type_t::SEQUENCE)) {
            datum_stream_t *ds = arg(0)->as_seq();
            return new_val(ds->map(env->new_func(&map_func)));
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
            const datum_t *el = obj->el(key, false);
            //                               ^^^^^ return 0 instead of throwing an error
            if (el) {
                bool conflict = out->add(key, el);
                r_sanity_check(!conflict);
            }
        }
        return new_val(out.release());
    }
    RDB_NAME("pluck")
};

} // namespace ql

