#ifndef RDB_PROTOCOL_SERIALIZABLE_ENVIRONMENT_HPP_
#define RDB_PROTOCOL_SERIALIZABLE_ENVIRONMENT_HPP_

#include "rdb_protocol/scope.hpp"

namespace query_language {

enum term_type_t {
    TERM_TYPE_JSON,
    TERM_TYPE_STREAM,
    TERM_TYPE_VIEW,

    /* This is the type of `Error` terms. It's called "arbitrary" because an
    `Error` term can be either a stream or an object. It is a subtype of every
    type. */
    TERM_TYPE_ARBITRARY
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(term_type_t, int8_t, TERM_TYPE_JSON, TERM_TYPE_ARBITRARY);

class term_info_t {
public:
    term_info_t() { }
    term_info_t(term_type_t _type, bool _deterministic)
        : type(_type), deterministic(_deterministic)
    { }

    term_type_t type;
    bool deterministic;

    RDB_MAKE_ME_SERIALIZABLE_2(type, deterministic);
};

typedef variable_scope_t<term_info_t> variable_type_scope_t;

typedef variable_type_scope_t::new_scope_t new_scope_t;

typedef implicit_value_t<term_info_t> implicit_type_t;

struct type_checking_environment_t {
    variable_type_scope_t scope;
    implicit_type_t implicit_type;

    RDB_MAKE_ME_SERIALIZABLE_2(scope, implicit_type);
};

//Scopes for single pieces of json
typedef variable_scope_t<boost::shared_ptr<scoped_cJSON_t> > variable_val_scope_t;

typedef variable_val_scope_t::new_scope_t new_val_scope_t;

//Implicit value typedef
typedef implicit_value_t<boost::shared_ptr<scoped_cJSON_t> >::impliciter_t implicit_value_setter_t;

/* Wrapper for the scopes in the runtime environment. Makes it convenient to
 * serialize all the in scope variables. */
struct scopes_t {
    variable_val_scope_t scope;
    type_checking_environment_t type_env;

    implicit_value_t<boost::shared_ptr<scoped_cJSON_t> > implicit_attribute_value;

    RDB_MAKE_ME_SERIALIZABLE_3(scope, type_env, implicit_attribute_value);
};


} //namespace query_language

#endif  // RDB_PROTOCOL_SERIALIZABLE_ENVIRONMENT_HPP_
