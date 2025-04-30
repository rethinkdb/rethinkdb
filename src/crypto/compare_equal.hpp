// Copyright 2010-2016 RethinkDB, all rights reserved.
#ifndef CRYPTO_COMPARE_EQUAL_HPP_
#define CRYPTO_COMPARE_EQUAL_HPP_

#include <openssl/crypto.h>

#include <array>

namespace crypto {

template <std::size_t N>
inline bool compare_equal(
        std::array<unsigned char, N> const &lhs,
        std::array<unsigned char, N> const &rhs) {
    return CRYPTO_memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
}

}  // namespace crypto

#endif // CRYPTO_COMPARE_EQUAL_HPP_
