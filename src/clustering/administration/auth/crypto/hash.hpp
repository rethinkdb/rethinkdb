// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_CRYPTO_HASH_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_CRYPTO_HASH_HPP

#include <array>
#include <string>

#include <openssl/sha.h>

namespace auth { namespace crypto {

namespace detail {

std::array<unsigned char, SHA256_DIGEST_LENGTH> sha256(
    unsigned char const *data, size_t size);

}  // namespace detail

template <std::size_t N>
inline std::array<unsigned char, SHA256_DIGEST_LENGTH> sha256(
       std::array<unsigned char, N> array) {
    return detail::sha256(array.data(), array.size());
}

inline std::array<unsigned char, SHA256_DIGEST_LENGTH> sha256(
        std::string const &string) {
    return detail::sha256(
        reinterpret_cast<unsigned char const *>(string.data()), string.size());
}

} }  // namespace auth::crypto

#endif  // CLUSTERING_ADMINISTRATION_AUTH_CRYPTO_HASH_HPP
