// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/password.hpp"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include "containers/archive/stl_types.hpp"
#include "containers/archive/versioned.hpp"

namespace auth {

namespace detail {

    template <std::size_t N>
    bool secure_compare(
            std::array<unsigned char, N> const &lhs,
            std::array<unsigned char, N> const &rhs) {
        unsigned char d = 0u;
        for (std::size_t i = 0; i < N; i++) {
            d |= lhs[i] ^ rhs[i];
        }
        return 1 & ((d - 1) >> 8);
    }

}  // namespace detail

password_t::password_t() {
}

password_t::password_t(std::string const &password, uint32_t iteration_count)
    : m_iteration_count(iteration_count) {
    if (RAND_bytes(m_salt.data(), m_salt.size()) != 1) {
        // FIXME, ERR_get_error
    }

    if (PKCS5_PBKDF2_HMAC(
            password.data(),
            password.size(),
            m_salt.data(),
            m_salt.size(),
            m_iteration_count,
            EVP_sha256(),
            m_hash.size(),
            m_hash.data()) != 1) {
        // FIXME
    }
}

std::array<unsigned char, 16> const &password_t::get_salt() const {
    return m_salt;
}

std::array<unsigned char, SHA256_DIGEST_LENGTH> const &password_t::get_hash() const {
    return m_hash;
}

uint32_t password_t::get_iteration_count() const {
    return m_iteration_count;
}

bool password_t::operator==(password_t const &rhs) const {
    return
        m_hash == rhs.m_hash &&
        m_salt == rhs.m_salt &&
        m_iteration_count == rhs.m_iteration_count;
}

RDB_IMPL_SERIALIZABLE_3(
    password_t,
    m_hash,
    m_salt,
    m_iteration_count);
INSTANTIATE_SERIALIZABLE_SINCE_v2_3(password_t);

}  // namespace auth
