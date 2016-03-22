// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_CRYPTO_COMPARE_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_CRYPTO_COMPARE_HPP

#include <array>

#include "errors.hpp"
#include <openssl/crypto.h>

namespace auth { namespace crypto {

template <std::size_t N>
inline bool compare(
        std::array<unsigned char, N> const &lhs,
        std::array<unsigned char, N> const &rhs) {
    return CRYPTO_memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
}

} }  // namespace auth::crypto

#endif  // CLUSTERING_ADMINISTRATION_AUTH_CRYPTO_COMPARE_HPP
