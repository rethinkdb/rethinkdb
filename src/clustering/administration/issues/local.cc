// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "clustering/administration/issues/local.hpp"

#include "utils.hpp"
#include "clustering/administration/persist.hpp"

RDB_IMPL_SERIALIZABLE_4_SINCE_v1_13(local_issue_t, type, critical, description, timestamp);
RDB_IMPL_EQUALITY_COMPARABLE_4(local_issue_t,
    type, critical, description, timestamp);


local_issue_t::local_issue_t(const std::string& _type, bool _critical, const std::string& _description)
        : type(_type), critical(_critical), description(_description), timestamp(get_secs()) { }

local_issue_t *local_issue_t::clone() const {
    local_issue_t *ret = new local_issue_t(type, critical, description);
    ret->timestamp = timestamp;
    return ret;
}
