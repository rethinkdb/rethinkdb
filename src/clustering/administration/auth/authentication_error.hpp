// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_AUTHENTICATION_ERROR_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_AUTHENTICATION_ERROR_HPP

#include <stdexcept>
#include <string>

namespace auth {

class authentication_error_t : public std::runtime_error {
public:
    authentication_error_t(std::string const &what)
        : std::runtime_error(what) {
    }
};

}  // namespace auth

#endif  // CLUSTERING_ADMINISTRATION_AUTH_AUTHENTICATION_ERROR_HPP
