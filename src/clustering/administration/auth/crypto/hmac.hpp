// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_CRYPTO_HMAC_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_CRYPTO_HMAC_HPP

#include <array>
#include <string>

#include <openssl/sha.h>

namespace auth { namespace crypto {

namespace detail {

std::array<unsigned char, SHA256_DIGEST_LENGTH> hmac_sha256(
        unsigned char const *key,
        size_t key_size,
        unsigned char const *data,
        size_t data_size);

}  // namespace detail

template <std::size_t N>
inline std::array<unsigned char, SHA256_DIGEST_LENGTH> hmac_sha256(
        std::string const &key,
        std::array<unsigned char, N> const &data) {
    return detail::hmac_sha256(
        key.data(),
        key.size(),
        data.data(),
        data.size());
}

template <std::size_t N>
inline std::array<unsigned char, SHA256_DIGEST_LENGTH> hmac_sha256(
        std::array<unsigned char, N> const &key,
        std::string const &data) {
    return detail::hmac_sha256(
        key.data(),
        key.size(),
        reinterpret_cast<unsigned char const *>(data.data()),
        data.size());
}


} }  // namespace auth::crypto

#endif  // CLUSTERING_ADMINISTRATION_AUTH_CRYPTO_HMAC_HPP
