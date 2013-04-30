#ifndef RDB_PROTOCOL_TERMS_SINDEX_HPP_
#define RDB_PROTOCOL_TERMS_SINDEX_HPP_

#include <string>

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {

class sindex_create_term_t : public op_term_t {
public:
    sindex_create_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(3)) { }

    virtual counted_t<val_t> eval_impl() {
        counted_t<table_t> table = arg(0)->as_table();
        std::string name = arg(1)->as_datum()->as_str();
        counted_t<func_t> index_func = arg(2)->as_func();

        bool success = table->sindex_create(name, index_func);
        if (success) {
            scoped_ptr_t<datum_t> res(new datum_t(datum_t::R_OBJECT));
            UNUSED bool b = res->add("created", make_counted<datum_t>(1.0));
            return new_val(counted_t<const datum_t>(res.release()));
        } else {
            rfail("Index `%s` already exists.", name.c_str());
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
            rfail("Index `%s` does not exist.", name.c_str());
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

} // namespace ql

#endif
