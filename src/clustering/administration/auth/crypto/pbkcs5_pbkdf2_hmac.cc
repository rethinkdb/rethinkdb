// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/crypto/pbkcs5_pbkdf2_hmac.hpp"

#include <openssl/evp.h>
#include <openssl/sha.h>

#include "clustering/administration/auth/crypto/crypto_error.hpp"

std::array<unsigned char, SHA256_DIGEST_LENGTH>
auth::crypto::detail::pbkcs5_pbkdf2_hmac_sha256(
        char const *password,
        size_t password_size,
        unsigned char const *salt,
        size_t salt_size,
        uint32_t iteration_count) {
    std::array<unsigned char, SHA256_DIGEST_LENGTH> hmac;

    if (PKCS5_PBKDF2_HMAC(
            password,
            password_size,
            salt,
            salt_size,
            iteration_count,
            EVP_sha256(),
            SHA256_DIGEST_LENGTH,
            hmac.data()) != 1) {
        // FIXME
    }

    return hmac;
}
