// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_USERNAME_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_USERNAME_HPP

#include <string>

#include "rpc/serialize_macros.hpp"

namespace auth {

class username_t
{
public:
    username_t();
    username_t(std::string const &username);

    bool is_admin() const;

    std::string const &to_string() const;

    bool operator<(username_t const &rhs) const;
    bool operator==(username_t const &rhs) const;

    RDB_DECLARE_ME_SERIALIZABLE(username_t);

private:
    std::string m_username;
};

}  // namespace auth

#endif  // CLUSTERING_ADMINISTRATION_AUTH_USERNAME_HPP
