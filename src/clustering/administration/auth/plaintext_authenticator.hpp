// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_PLAINTEXT_AUTHENTICATOR_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_PLAINTEXT_AUTHENTICATOR_HPP

#include <string>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "clustering/administration/auth/base_authenticator.hpp"
#include "clustering/administration/auth/user.hpp"

namespace auth {

class plaintext_authenticator_t : public base_authenticator_t {
public:
    plaintext_authenticator_t(
        clone_ptr_t<watchable_t<auth_semilattice_metadata_t>> auth_watchable,
        username_t const &username = username_t("admin"));

    /* virtual */ std::string next_message(std::string const &);
    /* virtual */ username_t get_authenticated_username() const;

private:
    username_t m_username;
};

}  // namespace auth

#endif  // CLUSTERING_ADMINISTRATION_AUTH_PLAINTEXT_AUTHENTICATOR_HPP
