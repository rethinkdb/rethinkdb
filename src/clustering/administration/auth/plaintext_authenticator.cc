// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/plaintext_authenticator.hpp"

#include "errors.hpp"
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include "clustering/administration/metadata.hpp"

namespace auth {

plaintext_authenticator_t::plaintext_authenticator_t(
       clone_ptr_t<watchable_t<auth_semilattice_metadata_t>> auth_watchable,
       std::string const &username) {
    auth_watchable->apply_read(
        [&](auth_semilattice_metadata_t const *auth_metadata) {
            auto user = auth_metadata->m_users.find(auth::username_t(username));
            if (user == auth_metadata->m_users.end() ||
                    static_cast<bool>(user->second.get_ref())) {
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

    std::array<unsigned char, SHA256_DIGEST_LENGTH> hash;
    if (PKCS5_PBKDF2_HMAC(
            password.data(),
            password.size(),
            m_user.get_password().get().get_salt().data(),
            m_user.get_password().get().get_salt().size(),
            m_user.get_password().get().get_iteration_count(),
            EVP_sha256(),
            hash.size(),
            hash.data()) != 1) {
        // FIXME
    }

    return CRYPTO_memcmp(
        m_user.get_password().get().get_hash().data(),
        hash.data(),
        hash.size()) == 0;
}

}  // namespace auth
