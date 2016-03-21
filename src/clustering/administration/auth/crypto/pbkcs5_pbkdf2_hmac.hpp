// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_CRYPTO_PKCS5_PBKDF2_HMAC_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_CRYPTO_PKCS5_PBKDF2_HMAC_HPP

#include <array>
#include <openssl/sha.h>

namespace auth { namespace crypto {

namespace detail {

std::array<unsigned char, SHA256_DIGEST_LENGTH> pbkcs5_pbkdf2_hmac_sha256(
        char const *password,
        size_t password_size,
        unsigned char const *salt,
        size_t salt_size,
        uint32_t iteration_count);

}  // namespace detail

template <std::size_t N>
inline std::array<unsigned char, SHA256_DIGEST_LENGTH> pbkcs5_pbkdf2_hmac_sha256(
        std::string const &password,
        std::array<unsigned char, N> const &salt,
        uint32_t iteration_count) {
    return detail::pbkcs5_pbkdf2_hmac_sha256(
        password.data(),
        password.size(),
        salt.data(),
        salt.size(),
        iteration_count);
}

template <std::size_t N>
inline std::array<unsigned char, SHA256_DIGEST_LENGTH> pbkcs5_pbkdf2_hmac_sha256(
        std::array<unsigned char, N> const &password,
        std::string const &salt,
        uint32_t iteration_count) {
    return detail::pbkcs5_pbkdf2_hmac_sha256(
        reinterpret_cast<char const *>(password.data()),
        password.size(),
        reinterpret_cast<unsigned char const *>(salt.data()),
        salt.size(),
        iteration_count);
}

} }  // namespace auth::crypto

#endif  // CLUSTERING_ADMINISTRATION_AUTH_CRYPTO_PKCS5_PBKDF2_HMAC_HPP
