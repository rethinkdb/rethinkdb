// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef GEO_EXCEPTIONS_HPP_
#define GEO_EXCEPTIONS_HPP_

#include <exception>
#include <string>

#include "utils.hpp"

class geo_exception_t {
public:
    geo_exception_t(std::string msg) : msg_(msg) { }
    virtual ~geo_exception_t() { }
    const char *what() const {
        return msg_.c_str();
    }
private:
    std::string msg_;
};

class geo_range_exception_t : public geo_exception_t {
public:
    geo_range_exception_t(std::string msg) : geo_exception_t(msg) { }
};

#endif  // GEO_EXCEPTIONS_HPP_
