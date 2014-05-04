// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_RDB_PROTOCOL_JSON_HPP_
#define RDB_PROTOCOL_RDB_PROTOCOL_JSON_HPP_

#include <memory>

#include "http/json.hpp"
#include "rdb_protocol/datum.hpp"

/* This file is for storing a few extensions to json that are useful for
 * implementing the rdb_protocol. */

void serialize(write_message_t *wm, const std::shared_ptr<const scoped_cJSON_t> &cjson);
MUST_USE archive_result_t deserialize(read_stream_t *s, std::shared_ptr<const scoped_cJSON_t> *cjson);

namespace query_language {

int json_cmp(cJSON *l, cJSON *r);

class shared_scoped_less_t {
public:
    shared_scoped_less_t() { }
    bool operator()(const counted_t<const ql::datum_t> &a,
                    const counted_t<const ql::datum_t> &b) const {
        return *a < *b;
    }
};

} // namespace query_language

#endif /* RDB_PROTOCOL_RDB_PROTOCOL_JSON_HPP_ */
