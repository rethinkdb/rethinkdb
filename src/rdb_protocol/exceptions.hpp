// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_EXCEPTIONS_HPP_
#define RDB_PROTOCOL_EXCEPTIONS_HPP_

#include <string>

#include "utils.hpp"
#include "rdb_protocol/bt.hpp"
#include "rpc/serialize_macros.hpp"

namespace query_language {


class runtime_exc_t : public std::exception {
public:
    runtime_exc_t() { }
    runtime_exc_t(const std::string &_what, const backtrace_t &bt)
        : message(_what), backtrace(bt)
    { }
    ~runtime_exc_t() throw () { }

    const char *what() const throw() {
        return message.c_str();
    }

    std::string as_str() const throw() {
        return strprintf("%s\nBacktrace:\n%s",
                         message.c_str(), backtrace.print().c_str());
    }

    std::string message;
    backtrace_t backtrace;

    RDB_MAKE_ME_SERIALIZABLE_2(message, backtrace);
};


}// namespace query_language 
#endif /* RDB_PROTOCOL_EXCEPTIONS_HPP_ */
