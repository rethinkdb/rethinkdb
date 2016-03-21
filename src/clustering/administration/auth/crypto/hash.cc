// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/crypto/hash.hpp"

#include <openssl/evp.h>
#include <openssl/opensslv.h>

#include "clustering/administration/auth/crypto/crypto_error.hpp"

std::array<unsigned char, SHA256_DIGEST_LENGTH> auth::crypto::detail::sha256(
        unsigned char const *data, size_t size) {
    std::array<unsigned char, SHA256_DIGEST_LENGTH> hash;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_MD_CTX *md_ctx = EVP_MD_CTX_create();
#else
    EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
#endif

    if (md_ctx == nullptr) {
        // FIXME
    }
    if (EVP_DigestInit(md_ctx, EVP_sha256()) != 1) {
        // FIXME
    }
    if (EVP_DigestUpdate(md_ctx, data, size) != 1) {
        // FIXME
    }
    if (EVP_DigestFinal(md_ctx, hash.data(), nullptr) != 1) {
        // FIXME
    }

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_MD_CTX_destroy(md_ctx);
#else
    EVP_MD_CTX_free(md_ctx);
#endif

    return hash;
}
