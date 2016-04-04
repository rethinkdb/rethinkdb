// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/plaintext_authenticator.hpp"

#include "clustering/administration/auth/authentication_error.hpp"
#include "clustering/administration/metadata.hpp"
#include "crypto/compare_equal.hpp"
#include "crypto/pbkcs5_pbkdf2_hmac.hpp"
#include "crypto/saslprep.hpp"

namespace auth {

plaintext_authenticator_t::plaintext_authenticator_t(
        clone_ptr_t<watchable_t<auth_semilattice_metadata_t>> auth_watchable,
        username_t const &username)
    : base_authenticator_t(auth_watchable),
      m_username(username),
      m_is_authenticated(false) {
}

/* virtual */ std::string plaintext_authenticator_t::next_message(
        std::string const &password) THROWS_ONLY(authentication_error_t) {
    boost::optional<user_t> user;

    m_auth_watchable->apply_read(
        [&](auth_semilattice_metadata_t const *auth_metadata) {
            auto iter = auth_metadata->m_users.find(m_username);
            if (iter != auth_metadata->m_users.end()) {
                user = iter->second.get_ref();
            }
    });

    if (!static_cast<bool>(user)) {
        // The user doesn't exist
        throw authentication_error_t(17, "Unknown user");
    }

    std::array<unsigned char, SHA256_DIGEST_LENGTH> hash =
        crypto::pbkcs5_pbkdf2_hmac_sha256(
            crypto::saslprep(password),
            user->get_password().get_salt(),
            user->get_password().get_iteration_count());

    if (!crypto::compare_equal(user->get_password().get_hash(), hash)) {
        throw authentication_error_t(12, "Wrong password");
    }

    m_is_authenticated = true;

    return "";
}

/* virtual */ username_t plaintext_authenticator_t::get_authenticated_username() const
        THROWS_ONLY(authentication_error_t) {
    if (m_is_authenticated) {
        return m_username;
    } else {
        throw authentication_error_t(20, "No authenticated user");
    }
}

}  // namespace auth
