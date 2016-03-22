// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/plaintext_authenticator.hpp"

#include "clustering/administration/auth/crypto/compare.hpp"
#include "clustering/administration/auth/crypto/pbkcs5_pbkdf2_hmac.hpp"
#include "clustering/administration/auth/crypto/saslprep.hpp"
#include "clustering/administration/metadata.hpp"

namespace auth {

plaintext_authenticator_t::plaintext_authenticator_t(
       clone_ptr_t<watchable_t<auth_semilattice_metadata_t>> auth_watchable,
       std::string const &username) {
    auth_watchable->apply_read(
        [&](auth_semilattice_metadata_t const *auth_metadata) {
            auto user = auth_metadata->m_users.find(auth::username_t(username));
            if (user == auth_metadata->m_users.end() ||
                    !static_cast<bool>(user->second.get_ref())) {
                // FIXME, user not found
            }
            m_user = user->second.get_ref().get();
    });
}

bool plaintext_authenticator_t::authenticate(std::string const &password) const {
    if (!m_user.has_password()) {
        // FIXME
    }

    // FIXME, SASLPrep password?

    std::array<unsigned char, SHA256_DIGEST_LENGTH> hash =
        crypto::pbkcs5_pbkdf2_hmac_sha256(
            password,
            m_user.get_password().get().get_salt(),
            m_user.get_password().get().get_iteration_count());

    return crypto::compare(m_user.get_password().get().get_hash(), hash);
}

}  // namespace auth
