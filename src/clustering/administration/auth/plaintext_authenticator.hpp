// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_PLAINTEXT_AUTHENTICATOR_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_PLAINTEXT_AUTHENTICATOR_HPP

#include <string>

#include "clustering/administration/auth/user.hpp"

namespace auth {

class plaintext_authenticator_t
{
public:
    plaintext_authenticator_t(
        clone_ptr_t<watchable_t<auth_semilattice_metadata_t>> auth_watchable);

    bool is_authentication_required() const;

    bool authenticate(std::string password) const;

private:
    user_t m_admin;
};

}  // namespace auth

#endif  // CLUSTERING_ADMINISTRATION_AUTH_PLAINTEXT_AUTHENTICATOR_HPP
