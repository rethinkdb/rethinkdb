// Copyright 2010-2016 RethinkDB, all rights reserved.
#ifndef CRYPTO_HASH_HPP_
#define CRYPTO_HASH_HPP_

#include <openssl/sha.h>
#include <stddef.h>

#include <array>
#include <string>

/**
 * @defgroup Cryptography Cryptographic Utilities
 * @brief Hashing, HMAC, and related crypto functions
 */

namespace crypto {

namespace detail {

/**
 * @ingroup Cryptography
 * @brief Compute SHA-256 hash of raw data.
 *
 * @param data Pointer to the data to hash.
 * @param size Size in bytes of the data.
 * @return A 32-byte array containing the SHA-256 hash.
 */
std::array<unsigned char, SHA256_DIGEST_LENGTH> sha256(
    unsigned char const *data, size_t size);

}  // namespace detail

/**
 * @ingroup Cryptography
 * @brief Compute SHA-256 hash of an array.
 *
 * @tparam N The size of the array.
 * @param array The array to hash.
 * @return A 32-byte array containing the SHA-256 hash.
 *
 * Example:
 * @code
 * std::array<unsigned char, 10> data = {...};
 * auto hash = sha256(data);
 * @endcode
 */
template <std::size_t N>
inline std::array<unsigned char, SHA256_DIGEST_LENGTH> sha256(
       std::array<unsigned char, N> array) {
    return detail::sha256(array.data(), array.size());
}

/**
 * @ingroup Cryptography
 * @brief Compute SHA-256 hash of a string.
 *
 * @param string The string to hash.
 * @return A 32-byte array containing the SHA-256 hash.
 *
 * Example:
 * @code
 * auto hash = sha256("hello world");
 * @endcode
 */
inline std::array<unsigned char, SHA256_DIGEST_LENGTH> sha256(
        std::string const &string) {
    return detail::sha256(
        reinterpret_cast<unsigned char const *>(string.data()), string.size());
}

}  // namespace crypto

#endif  // CRYPTO_HASH_HPP_
