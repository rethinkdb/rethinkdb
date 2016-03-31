// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_SCRAM_AUTHENTICATOR_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_SCRAM_AUTHENTICATOR_HPP

#include <map>
#include <string>

#include "clustering/administration/auth/base_authenticator.hpp"
#include "clustering/administration/auth/password.hpp"
#include "clustering/administration/auth/username.hpp"
#include "clustering/administration/metadata.hpp"

namespace auth {

class scram_authenticator_t : public base_authenticator_t {
public:
    scram_authenticator_t(
        clone_ptr_t<watchable_t<auth_semilattice_metadata_t>> auth_watchable);

    /* virtual */ std::string next_message(std::string const &)
            THROWS_ONLY(authentication_error_t);
    /* virtual */ username_t get_authenticated_username() const
            THROWS_ONLY(authentication_error_t);

    static std::map<char, std::string> split_attributes(std::string const &message);
    static username_t saslname_decode(std::string const &saslname);

private:
    enum class state_t{FIRST_MESSAGE, FINAL_MESSAGE, ERROR, AUTHENTICATED} m_state;
    std::string m_client_first_message_bare;
    username_t m_username;
    password_t m_password;
    bool m_is_user_known;
    std::string m_nonce;
    std::string m_server_first_message;
};

}  // namespace auth

#endif  // CLUSTERING_ADMINISTRATION_AUTH_SCRAM_AUTHENTICATOR_HPP
