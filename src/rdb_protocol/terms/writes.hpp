// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_TERMS_WRITES_HPP_
#define RDB_PROTOCOL_TERMS_WRITES_HPP_

#include "protocol_api.hpp"

template <class> class scoped_ptr_t;

namespace ql {
class val_t;

durability_requirement_t parse_durability_optarg(const scoped_ptr_t<val_t> &arg);

}  // namespace ql

#endif  // RDB_PROTOCOL_TERMS_WRITES_HPP_
