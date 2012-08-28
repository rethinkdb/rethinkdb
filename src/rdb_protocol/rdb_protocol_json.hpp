#ifndef RDB_PROTOCOL_RDB_PROTOCOL_JSON_HPP_
#define RDB_PROTOCOL_RDB_PROTOCOL_JSON_HPP_

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "rdb_protocol/backtrace.hpp"
#include "http/json.hpp"

/* This file is for storing a few extensions to json that are useful for
 * implementing the rdb_protocol. */

// TODO: shared_ptr?  Die.
write_message_t &operator<<(write_message_t &msg, const boost::shared_ptr<scoped_cJSON_t> &cjson);
MUST_USE archive_result_t deserialize(read_stream_t *s, boost::shared_ptr<scoped_cJSON_t> *cjson);

namespace query_language {

int cJSON_cmp(cJSON *l, cJSON *r, const backtrace_t &backtrace);

class shared_scoped_less_t {
public:
    shared_scoped_less_t() { }
    explicit shared_scoped_less_t(const backtrace_t &bt) : backtrace(bt) { }
    bool operator()(const boost::shared_ptr<scoped_cJSON_t> &a,
                      const boost::shared_ptr<scoped_cJSON_t> &b) {
        if (a->type() == b->type()) {
            return cJSON_cmp(a->get(), b->get(), backtrace) < 0;
        } else {
            return a->type() > b->type();
        }
    }
private:
    backtrace_t backtrace;
};

void require_type(const cJSON *, int type, const backtrace_t &);

} //namespace query_language

#endif /* RDB_PROTOCOL_RDB_PROTOCOL_JSON_HPP_ */
