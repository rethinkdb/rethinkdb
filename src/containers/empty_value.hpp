// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CONTAINERS_EMPTY_VALUE_HPP_
#define CONTAINERS_EMPTY_VALUE_HPP_

#include "rpc/serialize_macros.hpp"

/* `empty_value_t` is a class with exactly one legal instance. It can be used as the
value type of a `watchable_map_t` in order to simulate a "watchable set". */

class empty_value_t {
public:
    empty_value_t() { }
    bool operator==(const empty_value_t &) const {
        return true;
    }
    RDB_MAKE_ME_SERIALIZABLE_0(empty_value_t);
};

#endif /* CONTAINERS_EMPTY_VALUE_HPP_ */

