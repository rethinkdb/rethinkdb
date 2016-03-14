// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_SCRAM_AUTHENTICATOR_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_SCRAM_AUTHENTICATOR_HPP

#include <string>

#include "clustering/administration/auth/user.hpp"

namespace auth {

class scram_authenticator_t
{
public:
    scram_authenticator_t(
        clone_ptr_t<watchable_t<auth_semilattice_metadata_t>> auth_watchable);

    bool is_authentication_required() const;

    std::string first_message(std::string const &message);

private:
    bool m_admin_has_password;
    clone_ptr_t<watchable_t<auth_semilattice_metadata_t>> m_auth_watchable;
};

}  // namespace auth

#endif  // CLUSTERING_ADMINISTRATION_AUTH_SCRAM_AUTHENTICATOR_HPP
