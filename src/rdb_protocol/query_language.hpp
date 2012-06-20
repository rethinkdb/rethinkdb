#ifndef RDB_PROTOCOL_QUERY_LANGUAGE_HPP__
#define RDB_PROTOCOL_QUERY_LANGUAGE_HPP__

#include "rdb_protocol/query_language.pb.h"

/* Make sure that the protocol buffers we receive are a well defined type. That
 * is they specifiy which type they are and have the correct optional fields
 * filled in (and none of the others). */

bool is_well_defined(const VarTermTuple &);

bool is_well_defined(const Term &);

bool is_well_defined(const Builtin &);

bool is_well_defined(const Reduction &);

bool is_well_defined(const Mapping &);

bool is_well_defined(const Predicate &);

bool is_well_defined(const View &);

bool is_well_defined(const Builtin &);

bool is_well_defined(const WriteQuery &);

#endif
