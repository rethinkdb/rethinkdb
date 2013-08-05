// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_PSEUDO_LITERAL_HPP_
#define RDB_PROTOCOL_PSEUDO_LITERAL_HPP_

#include "rdb_protocol/datum.hpp"

namespace ql {
namespace pseudo {
extern const char *const literal_string;
extern const char *const value_string;

void rcheck_literal_valid(const datum_t *lit);

} // namespace pseudo
} // namespace ql

#endif
