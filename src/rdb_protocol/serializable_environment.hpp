// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_SERIALIZABLE_ENVIRONMENT_HPP_
#define RDB_PROTOCOL_SERIALIZABLE_ENVIRONMENT_HPP_

#include "rdb_protocol/scope.hpp"

namespace query_language {

enum term_type_t {
    TERM_TYPE_JSON,
    TERM_TYPE_STREAM,
    TERM_TYPE_VIEW,

    // TODO: in the clients errors are just JSON for simplicity.  Maybe we should do that here too?
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

    RDB_DECLARE_ME_SERIALIZABLE;
};

/* Wrapper for the scopes in the runtime environment. Makes it convenient to
 * serialize all the in scope variables. */
struct scopes_t {
    RDB_DECLARE_ME_SERIALIZABLE;
};


} //namespace query_language

#endif  // RDB_PROTOCOL_SERIALIZABLE_ENVIRONMENT_HPP_
