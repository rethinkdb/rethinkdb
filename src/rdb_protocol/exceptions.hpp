#ifndef RDB_PROTOCOL_EXCEPTIONS_HPP_
#define RDB_PROTOCOL_EXCEPTIONS_HPP_

#include <string>

#include "rdb_protocol/backtrace.hpp"
#include "rpc/serialize_macros.hpp"

namespace query_language {

/* Thrown if the client sends a malformed or nonsensical query (e.g. a
   protocol buffer that doesn't match our schema or STOP for an
   unknown token). */
class broken_client_exc_t : public std::exception {
public:
    explicit broken_client_exc_t(const std::string &_what) : message(_what) { }
    ~broken_client_exc_t() throw () { }

    const char *what() const throw () {
        return message.c_str();
    }

    std::string message;
};

/* `bad_query_exc_t` is thrown if the user writes a query that
   accesses undefined variables or that has mismatched types. The
   difference between this and `broken_client_exc_t` is that
   `broken_client_exc_t` is the client's fault and
   `bad_query_exc_t` is the client's user's fault. */

class bad_query_exc_t : public std::exception {
public:
    bad_query_exc_t(const std::string &s, const backtrace_t &bt) : message(s), backtrace(bt) { }

    ~bad_query_exc_t() throw () { }

    const char *what() const throw () {
        return message.c_str();
    }

    std::string message;
    backtrace_t backtrace;
};

class runtime_exc_t {
public:
    runtime_exc_t() { }
    runtime_exc_t(const std::string &_what, const backtrace_t &bt)
        : message(_what), backtrace(bt)
    { }

    std::string what() const throw() {
        return message;
    }

    std::string message;
    backtrace_t backtrace;

    RDB_MAKE_ME_SERIALIZABLE_2(message, backtrace);
};


}// namespace query_language 
#endif /* RDB_PROTOCOL_EXCEPTIONS_HPP_ */
