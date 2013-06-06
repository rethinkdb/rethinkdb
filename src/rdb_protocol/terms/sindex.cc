#include "rdb_protocol/terms/terms.hpp"

#include <string>

#include "rdb_protocol/error.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/pb_utils.hpp"

#pragma GCC diagnostic ignored "-Wshadow"

namespace ql {

// We need to use inheritance rather than composition for
// `env_t::special_var_shadower_t` because it needs to be initialized before
// `op_term_t`.
class sindex_create_term_t : private env_t::special_var_shadower_t, public op_term_t {
public:
    sindex_create_term_t(env_t *env, protob_t<const Term> term)
        : env_t::special_var_shadower_t(env, env_t::SINDEX_ERROR_VAR),
          op_term_t(env, term, argspec_t(2, 3)) { }

    virtual counted_t<val_t> eval_impl() {
        counted_t<table_t> table = arg(0)->as_table();
        counted_t<const datum_t> name_datum = arg(1)->as_datum();
        std::string name = name_datum->as_str();
        rcheck(name != table->get_pkey(),
               base_exc_t::GENERIC,
               strprintf("Index name conflict: `%s` is the name of the primary key.",
                         name.c_str()));
        counted_t<func_t> index_func;
        if (num_args() == 3) {
            index_func = arg(2)->as_func();
        } else {
            protob_t<Term> func_term = make_counted_term();
            int x = env->gensym();
            {
                Term *arg = pb::set_func(func_term.get(), x);
                N2(GETATTR, NVAR(x), NDATUM(name_datum));
            }

            prop_bt(func_term.get());
            index_func = make_counted<func_t>(env, func_term);
        }
        r_sanity_check(index_func.has());

        bool success = table->sindex_create(name, index_func);
        if (success) {
            scoped_ptr_t<datum_t> res(new datum_t(datum_t::R_OBJECT));
            UNUSED bool b = res->add("created", make_counted<datum_t>(1.0));
            return new_val(counted_t<const datum_t>(res.release()));
        } else {
            rfail(base_exc_t::GENERIC, "Index `%s` already exists.", name.c_str());
        }
    }

    virtual const char *name() const { return "sindex_create"; }
};

class sindex_drop_term_t : public op_term_t {
public:
    sindex_drop_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(2)) { }

    virtual counted_t<val_t> eval_impl() {
        counted_t<table_t> table = arg(0)->as_table();
        std::string name = arg(1)->as_datum()->as_str();
        bool success = table->sindex_drop(name);
        if (success) {
            scoped_ptr_t<datum_t> res(new datum_t(datum_t::R_OBJECT));
            UNUSED bool b = res->add("dropped", make_counted<datum_t>(1.0));
            return new_val(counted_t<const datum_t>(res.release()));
        } else {
            rfail(base_exc_t::GENERIC, "Index `%s` does not exist.", name.c_str());
        }
    }

    virtual const char *name() const { return "sindex_drop"; }
};

class sindex_list_term_t : public op_term_t {
public:
    sindex_list_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(1)) { }

    virtual counted_t<val_t> eval_impl() {
        counted_t<table_t> table = arg(0)->as_table();

        return new_val(table->sindex_list());
    }

    virtual const char *name() const { return "sindex_list"; }
};

counted_t<term_t> make_sindex_create_term(env_t *env, protob_t<const Term> term) {
    return make_counted<sindex_create_term_t>(env, term);
}
counted_t<term_t> make_sindex_drop_term(env_t *env, protob_t<const Term> term) {
    return make_counted<sindex_drop_term_t>(env, term);
}
counted_t<term_t> make_sindex_list_term(env_t *env, protob_t<const Term> term) {
    return make_counted<sindex_list_term_t>(env, term);
}


} // namespace ql

