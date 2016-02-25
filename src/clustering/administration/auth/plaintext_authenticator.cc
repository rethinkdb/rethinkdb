// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/plaintext_authenticator.hpp"

#include "errors.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include "clustering/administration/metadata.hpp"

namespace auth {

namespace detail {

    template <std::size_t N>
    inline bool secure_compare(
            std::array<unsigned char, N> const &lhs,
            std::array<unsigned char, N> const &rhs) {
        unsigned char d = 0u;
        for (std::size_t i = 0; i < N; i++) {
            d |= lhs[i] ^ rhs[i];
        }
        return 1 & ((d - 1) >> 8);
    }

}  // namespace detail

plaintext_authenticator_t::plaintext_authenticator_t(
       clone_ptr_t<watchable_t<auth_semilattice_metadata_t>> auth_watchable) {
    auth_watchable->apply_read(
        [&](auth_semilattice_metadata_t const *auth_metadata) {
            auto user = auth_metadata->m_users.find(auth::username_t("admin"));
            rassert(
                user != auth_metadata->m_users.end() ||
                !static_cast<bool>(user->second.get_ref()));
            m_admin = user->second.get_ref().get();
    });
}

bool plaintext_authenticator_t::is_authentication_required() const {
    return m_admin.has_password();
}

bool plaintext_authenticator_t::authenticate(std::string foo_password) const {
    if (!m_admin.has_password()) {
        // FIXME
    }

    // FIXME, SASLPrep password?

    auth::password_t const &bar_password = m_admin.get_password().get();
    std::array<unsigned char, SHA256_DIGEST_LENGTH> hash;
    if (PKCS5_PBKDF2_HMAC(
            foo_password.data(),
            foo_password.size(),
            bar_password.get_salt().data(),
            bar_password.get_salt().size(),
            bar_password.get_iteration_count(),
            EVP_sha256(),
            hash.size(),
            hash.data()) != 1) {
        // FIXME
    }

    return detail::secure_compare(bar_password.get_hash(), hash);
}

}  // namespace auth
