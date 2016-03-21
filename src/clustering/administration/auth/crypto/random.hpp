// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_CRYPTO_RANDOM_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_CRYPTO_RANDOM_HPP

#include <array>

namespace auth { namespace crypto {

namespace detail {

void random_bytes(unsigned char *, size_t);

}  // namespace detail

template <std::size_t N>
inline std::array<unsigned char, N> random_bytes() {
    std::array<unsigned char, N> bytes;
    detail::random_bytes(bytes.data(), bytes.size());
    return bytes;
}

} }  // namespace auth::crypto

#endif  // CLUSTERING_ADMINISTRATION_AUTH_CRYPTO_RANDOM_HPP
