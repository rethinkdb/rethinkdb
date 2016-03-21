// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_SCRAM_AUTHENTICATOR_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_SCRAM_AUTHENTICATOR_HPP

#include <map>
#include <string>

#include "clustering/administration/auth/password.hpp"
#include "clustering/administration/auth/username.hpp"
#include "clustering/administration/metadata.hpp"

namespace auth {

class scram_authenticator_t
{
public:
    scram_authenticator_t(
        clone_ptr_t<watchable_t<auth_semilattice_metadata_t>> auth_watchable);

    std::string first_message(std::string const &message);
    std::string final_message(std::string const &message);

    static std::map<char, std::string> split_attributes(std::string const &message);
    static username_t saslname_decode(std::string const &saslname);

private:
    clone_ptr_t<watchable_t<auth_semilattice_metadata_t>> m_auth_watchable;

    std::string m_client_first_message_bare;
    password_t m_password;
    std::string m_nonce;
    std::string m_server_first_message;
};

}  // namespace auth

#endif  // CLUSTERING_ADMINISTRATION_AUTH_SCRAM_AUTHENTICATOR_HPP
