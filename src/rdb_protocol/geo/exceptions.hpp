// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_GEO_EXCEPTIONS_HPP_
#define RDB_PROTOCOL_GEO_EXCEPTIONS_HPP_

#include <exception>
#include <string>

#include "utils.hpp"

class geo_exception_t : public std::exception {
public:
    explicit geo_exception_t(std::string msg) : msg_(msg) { }
    virtual ~geo_exception_t() throw() { }
    const char *what() const throw() {
        return msg_.c_str();
    }
private:
    std::string msg_;
};

class geo_range_exception_t : public geo_exception_t {
public:
    explicit geo_range_exception_t(std::string msg) : geo_exception_t(msg) { }
};

#endif  // RDB_PROTOCOL_GEO_EXCEPTIONS_HPP_
