// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_RDB_PROTOCOL_JSON_HPP_
#define RDB_PROTOCOL_RDB_PROTOCOL_JSON_HPP_

#include <memory>

#include "rdb_protocol/bt.hpp"
#include "http/json.hpp"

/* This file is for storing a few extensions to json that are useful for
 * implementing the rdb_protocol. */

write_message_t &operator<<(write_message_t &msg, const std::shared_ptr<scoped_cJSON_t> &cjson);
MUST_USE archive_result_t deserialize(read_stream_t *s, std::shared_ptr<scoped_cJSON_t> *cjson);

namespace query_language {

int json_cmp(cJSON *l, cJSON *r);

class shared_scoped_less_t {
public:
    shared_scoped_less_t() { }
    explicit shared_scoped_less_t(const backtrace_t &bt) : backtrace(bt) { }
    bool operator()(const std::shared_ptr<scoped_cJSON_t> &a,
                    const std::shared_ptr<scoped_cJSON_t> &b) const {
        return json_cmp(a->get(), b->get()) < 0;
    }
private:
    backtrace_t backtrace;
};

void require_type(const cJSON *, int type, const backtrace_t &);

} // namespace query_language

#endif /* RDB_PROTOCOL_RDB_PROTOCOL_JSON_HPP_ */
