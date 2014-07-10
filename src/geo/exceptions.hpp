// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef GEO_EXCEPTIONS_HPP_
#define GEO_EXCEPTIONS_HPP_

#include <exception>
#include <string>

#include "utils.hpp"

class geo_exception_t : public std::exception {
public:
    geo_exception_t(std::string msg) : msg_(msg) { }
    const char *what() const THROWS_NOTHING {
        return msg_.c_str();
    }
private:
    std::string msg_;
};

// TODO! Improve those
class geo_range_exception_t : public geo_exception_t {
public:
    geo_range_exception_t(std::string msg) : geo_exception_t(msg) { }
};

#endif  // GEO_EXCEPTIONS_HPP_
