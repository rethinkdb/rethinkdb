#ifndef RDB_PROTOCOL_TERMS_SINDEX_HPP_
#define RDB_PROTOCOL_TERMS_SINDEX_HPP_

#include <string>

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {

class sindex_create_term_t : public op_term_t {
public:
    sindex_create_term_t(env_t *env, const Term *term) 
        : op_term_t(env, term, argspec_t(3)) { }

    virtual val_t *eval_impl() {
        table_t *table = arg(0)->as_table();
        std::string name = arg(1)->as_datum()->as_str();
        func_t *index_func = arg(2)->as_func();

        return new_val(table->sindex_create(name, index_func));
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

        return new_val(table->sindex_drop(name));
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

} //namespace ql 

#endif
