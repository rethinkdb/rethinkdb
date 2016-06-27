// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_BASE_AUTHENTICATOR_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_BASE_AUTHENTICATOR_HPP

#include <string>

#include "clustering/administration/auth/authentication_error.hpp"
#include "clustering/administration/auth/username.hpp"
#include "clustering/administration/metadata.hpp"

namespace auth {

class base_authenticator_t {
public:
    base_authenticator_t(
            clone_ptr_t<watchable_t<auth_semilattice_metadata_t>> auth_watchable)
        : m_auth_watchable(auth_watchable) {
    }
    virtual ~base_authenticator_t() {
    }

    virtual std::string next_message(std::string const &)
            THROWS_ONLY(authentication_error_t) = 0;
    virtual username_t get_authenticated_username() const
            THROWS_ONLY(authentication_error_t) = 0;

protected:
    clone_ptr_t<watchable_t<auth_semilattice_metadata_t>> m_auth_watchable;
};

}  // namespace auth

#endif  // CLUSTERING_ADMINISTRATION_AUTH_BASE_AUTHENTICATOR_HPP
