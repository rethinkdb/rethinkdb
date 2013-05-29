// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_SERIALIZABLE_ENVIRONMENT_HPP_
#define RDB_PROTOCOL_SERIALIZABLE_ENVIRONMENT_HPP_

#include "rdb_protocol/scope.hpp"

namespace query_language {

/* Wrapper for the scopes in the runtime environment. Makes it convenient to
 * serialize all the in scope variables. */
struct scopes_t {
    RDB_DECLARE_ME_SERIALIZABLE;
};


} //namespace query_language

#endif  // RDB_PROTOCOL_SERIALIZABLE_ENVIRONMENT_HPP_
