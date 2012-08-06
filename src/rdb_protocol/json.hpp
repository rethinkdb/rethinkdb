#ifndef RDB_PROTOCOL_JSON_HPP_
#define RDB_PROTOCOL_JSON_HPP_

#include "rdb_protocol/backtrace.hpp"
#include "http/json.hpp"

/* This file is for storing a few extensions to json that are useful for
 * implementing the rdb_protocol. */

namespace query_language {

int cJSON_cmp(cJSON *l, cJSON *r, const backtrace_t &backtrace);

class shared_scoped_less {
public:
    shared_scoped_less() { }
    explicit shared_scoped_less(const backtrace_t &bt) : backtrace(bt) { }
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

} //namespace query_language

#endif /* RDB_PROTOCOL_JSON_HPP_ */
