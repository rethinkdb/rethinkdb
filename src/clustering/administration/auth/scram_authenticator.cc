// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/scram_authenticator.hpp"

#include "errors.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include "clustering/administration/metadata.hpp"

namespace auth {

scram_authenticator_t::scram_authenticator_t(
        clone_ptr_t<watchable_t<auth_semilattice_metadata_t>> auth_watchable)
    : m_auth_watchable(auth_watchable) {
    m_auth_watchable->apply_read(
        [&](auth_semilattice_metadata_t const *auth_metadata) {
            auto user = auth_metadata->m_users.find(auth::username_t("admin"));
            rassert(
                user != auth_metadata->m_users.end() ||
                !static_cast<bool>(user->second.get_ref()));
            m_admin_has_password = user->second.get_ref()->has_password();
    });
}

bool scram_authenticator_t::is_authentication_required() const {
    return m_admin_has_password;
}

std::string scram_authenticator_t::first_message(std::string const &message) {
    if (message.empty()) {
        // FIXME, error
    }

    return "r=rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0,s=W22ZaJ0SNY7soEsUEjb6gQ==,i=4096";
}

/* std::string scram_authenticator_t::second_message(std::string const &message) {
    if (message.empty()) {
        // FIXME, error
    }

    return "v=6rriTRBi23WpRR/wtup+mMhUZUn/dB5nLTJRsjl95G4=";
} */

}  // namespace auth
