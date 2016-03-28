// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/plaintext_authenticator.hpp"

#include "clustering/administration/metadata.hpp"
#include "crypto/compare.hpp"
#include "crypto/pbkcs5_pbkdf2_hmac.hpp"
#include "crypto/saslprep.hpp"

namespace auth {

plaintext_authenticator_t::plaintext_authenticator_t(
        clone_ptr_t<watchable_t<auth_semilattice_metadata_t>> auth_watchable,
        std::string const &username) {
    auth_watchable->apply_read(
        [&](auth_semilattice_metadata_t const *auth_metadata) {
            auto user = auth_metadata->m_users.find(auth::username_t(username));
            if (user != auth_metadata->m_users.end()) {
                m_user = user->second.get_ref();
            }
    });
}

bool plaintext_authenticator_t::authenticate(std::string const &password) const {
    if (!static_cast<bool>(m_user)) {
        // The user does not exist
        return false;
    }

    std::array<unsigned char, SHA256_DIGEST_LENGTH> hash =
        crypto::pbkcs5_pbkdf2_hmac_sha256(
            crypto::saslprep(password),
            m_user->get_password().get_salt(),
            m_user->get_password().get_iteration_count());

    return crypto::compare(m_user->get_password().get_hash(), hash);
}

}  // namespace auth
