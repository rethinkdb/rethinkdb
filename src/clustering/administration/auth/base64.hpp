// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_BASE64_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_BASE64_HPP

#include <array>
#include <string>

namespace auth {

namespace detail {

std::string base64_encode(unsigned char const *, size_t);

}  // namespace detail

std::string base64_decode(std::string const &);

template <std::size_t N>
inline std::string base64_encode(std::array<unsigned char, N> const &array) {
    return detail::base64_encode(array.data(), array.size());
}

inline std::string base64_encode(std::string const &string) {
    return detail::base64_encode(
        reinterpret_cast<unsigned char const *>(string.data()), string.size());
}

}  // namespace auth

#endif  // CLUSTERING_ADMINISTRATION_AUTH_BASE64_HPP
