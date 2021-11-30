// Copyright 2010-2016 RethinkDB, all rights reserved.
#ifndef CRYPTO_RANDOM_HPP
#define CRYPTO_RANDOM_HPP

#include <array>

namespace crypto {

namespace detail {

void random_bytes(unsigned char *, std::size_t);

}  // namespace detail

template <std::size_t N>
inline std::array<unsigned char, N> random_bytes() {
    std::array<unsigned char, N> bytes;
    detail::random_bytes(bytes.data(), bytes.size());
    return bytes;
}

}  // namespace crypto

#endif  // CRYPTO_RANDOM_HPP
