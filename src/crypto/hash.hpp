// Copyright 2010-2016 RethinkDB, all rights reserved.
#ifndef CRYPTO_HASH_HPP
#define CRYPTO_HASH_HPP

#include <openssl/sha.h>

#include <array>
#include <string>

namespace crypto {

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

}  // namespace crypto

#endif  // CRYPTO_HASH_HPP
