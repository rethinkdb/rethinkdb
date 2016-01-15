// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_RDB_BACKTRACE_HPP_
#define RDB_PROTOCOL_RDB_BACKTRACE_HPP_

#include <vector>
#include <stdexcept>

#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/ql2.pb.h"

namespace ql {

class backtrace_registry_t;

// A query-language exception with its backtrace resolved to a datum_t
// This should only be thrown from outside term evaluation - it is only meant
// to be constructed or caught on the query's home coroutine.
class bt_exc_t : public std::exception {
public:
    bt_exc_t(Response::ResponseType _response_type,
             Response::ErrorType _error_type,
             const std::string &_message,
             datum_t _bt_datum)
        : response_type(_response_type),
          error_type(_error_type),
          message(_message),
          bt_datum(_bt_datum) { }
    virtual ~bt_exc_t() throw () { }

    const char *what() const throw () { return message.c_str(); }

    const Response::ResponseType response_type;
    const Response::ErrorType error_type;
    const std::string message;
    const datum_t bt_datum;
};

class backtrace_registry_t {
public:
    backtrace_registry_t();
    backtrace_registry_t(backtrace_registry_t &&) = default;

    backtrace_id_t new_frame(backtrace_id_t parent_bt,
                             const datum_t &val);

    datum_t datum_backtrace(const exc_t &ex) const;
    datum_t datum_backtrace(backtrace_id_t bt, size_t dummy_frames = 0) const;

    static const datum_t EMPTY_BACKTRACE;

private:
    struct frame_t {
        frame_t(); // Only for creating the HEAD term
        frame_t(backtrace_id_t _parent, datum_t _val);
        bool is_head() const;

        backtrace_id_t parent;
        datum_t val;
    };

    std::vector<frame_t> frames;
    DISABLE_COPYING(backtrace_registry_t);
};

} // namespace ql

#endif // RDB_PROTOCOL_BACKTRACE_HPP_
