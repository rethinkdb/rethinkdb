// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/crypto/hmac.hpp"

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/opensslv.h>

#include "clustering/administration/auth/crypto/crypto_error.hpp"

std::array<unsigned char, SHA256_DIGEST_LENGTH> auth::crypto::detail::hmac_sha256(
        unsigned char const *key,
        size_t key_size,
        unsigned char const *data,
        size_t data_size) {
    std::array<unsigned char, SHA256_DIGEST_LENGTH> hmac;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    HMAC_CTX ctx;
    HMAC_CTX_init(&ctx);

    if (HMAC_Init_ex(&ctx, key, key_size, EVP_sha256(), nullptr) != 1) {
        // FIXME
    }
    if (HMAC_Update(&ctx, data, data_size) != 1) {
        // FIXME
    }
    if (HMAC_Final(&ctx, hmac.data(), nullptr) != 1) {
        // FIXME
    }
    HMAC_CTX_cleanup(&ctx);
#else
    HMAC_CTX *ctx = HMAC_CTX_new();
    if (ctx == nullptr) {
        // FIXME
    }
    if (HMAC_Init_ex(ctx, key, key_size, EVP_sha256(), nullptr) != 1) {
        // FIXME
    }
    if (HMAC_Update(ctx, data, data_size) != 1) {
        // FIXME
    }
    if (HMAC_Final(ctx, hmac.data(), nullptr) != 1) {
        // FIXME
    }
    HMAC_CTX_free(ctx);
#endif

    return hmac;
}
