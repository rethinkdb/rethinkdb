// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_TIME_HPP_
#define RDB_PROTOCOL_TIME_HPP_

#include "rdb_protocol/env.hpp"

namespace ql {
namespace pseudo {
extern const char *const time_string;
int time_cmp(const datum_t& x, const datum_t& y);
bool time_valid(const datum_t &time);
} //namespace pseudo 
} //namespace ql 

#endif //RDB_PROTOCOL_TIME_HPP_
