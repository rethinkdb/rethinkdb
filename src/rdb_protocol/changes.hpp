// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_CHANGES_HPP_
#define RDB_PROTOCOL_CHANGES_HPP_

#include "rpc/serialize_macros.hpp"

enum class return_changes_t {
    NO = 0,
    YES = 1
};
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        return_changes_t, int8_t,
        return_changes_t::NO, return_changes_t::YES);

#endif  // RDB_PROTOCOL_CHANGES_HPP_
