// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_PERMISSION_ERROR_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_PERMISSION_ERROR_HPP

#include <stdexcept>
#include <string>

#include "clustering/administration/auth/username.hpp"
#include "utils.hpp"

namespace auth {

class permission_error_t : public std::runtime_error {
public:
    permission_error_t(std::string const &permission)
        : std::runtime_error(
            "Internal permission error, required `" + permission + "` permission.") {
    }

    permission_error_t(username_t const &username, std::string const &permission)
        : std::runtime_error(
            "User `" + username.to_string() + "`does not have the required `" +
                permission + "` permission.") {
    }
};

}  // namespace auth

#endif  // CLUSTERING_ADMINISTRATION_AUTH_PERMISSION_ERROR_HPP
