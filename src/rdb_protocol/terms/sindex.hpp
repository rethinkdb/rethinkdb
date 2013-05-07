#ifndef RDB_PROTOCOL_TERMS_SINDEX_HPP_
#define RDB_PROTOCOL_TERMS_SINDEX_HPP_

#include <string>

#include "rdb_protocol/error.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/pb_utils.hpp"

namespace ql {

class sindex_create_term_t : public op_term_t {
public:
    sindex_create_term_t(env_t *env, const Term *term)
        : op_term_t(env, term, argspec_t(2, 3)) { }

    virtual val_t *eval_impl() {
        table_t *table = arg(0)->as_table();
        const datum_t *name_datum = arg(1)->as_datum();
        std::string name = name_datum->as_str();
        rcheck(name != table->get_pkey(),
               strprintf("Index name conflict: `%s` is the name of the primary key.",
                         name.c_str()));
        func_t *index_func = NULL;
        if (num_args() ==3) {
            index_func = arg(2)->as_func();
        } else {
            env_wrapper_t<Term> *twrap = env->add_ptr(new env_wrapper_t<Term>());
            Term *func_term = &twrap->t;
            int x = env->gensym();
            Term *arg = pb::set_func(func_term, x);
            N2(GETATTR, NVAR(x), NDATUM(name_datum));
            prop_bt(func_term);
            index_func = env->add_ptr(new func_t(env, func_term));
        }
        r_sanity_check(index_func != NULL);

        bool success = table->sindex_create(name, index_func);
        if (success) {
            datum_t *res = env->add_ptr(new datum_t(datum_t::R_OBJECT));
            UNUSED bool b = res->add("created", env->add_ptr(new datum_t(1.0)));
            return new_val(res);
        } else {
            rfail("Index `%s` already exists.", name.c_str());
        }
    }

    virtual const char *name() const { return "sindex_create"; }
};

class sindex_drop_term_t : public op_term_t {
public:
    sindex_drop_term_t(env_t *env, const Term *term)
        : op_term_t(env, term, argspec_t(2)) { }

    virtual val_t *eval_impl() {
        table_t *table = arg(0)->as_table();
        std::string name = arg(1)->as_datum()->as_str();
        bool success = table->sindex_drop(name);
        if (success) {
            datum_t *res = env->add_ptr(new datum_t(datum_t::R_OBJECT));
            UNUSED bool b = res->add("dropped", env->add_ptr(new datum_t(1.0)));
            return new_val(res);
        } else {
            rfail("Index `%s` does not exist.", name.c_str());
        }
    }

    virtual const char * name() const { return "sindex_drop"; }
};

class sindex_list_term_t : public op_term_t {
public:
    sindex_list_term_t(env_t *env, const Term *term)
        : op_term_t(env, term, argspec_t(1)) { }

    virtual val_t *eval_impl() {
        table_t *table = arg(0)->as_table();

        return new_val(table->sindex_list());
    }

    virtual const char * name() const { return "sindex_list"; }
};

} // namespace ql

#endif
