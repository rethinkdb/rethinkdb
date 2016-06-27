// Copyright 2010-2016 RethinkDB, all rights reserved.
#include "crypto/hash.hpp"

#include <openssl/evp.h>
#include <openssl/opensslv.h>

#include <string>

#include "crypto/error.hpp"

namespace crypto {

class evp_md_ctx_wrapper_t {
public:
    evp_md_ctx_wrapper_t() {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        m_evp_md_ctx = EVP_MD_CTX_create();
#else
        m_evp_md_ctx = EVP_MD_CTX_new();
#endif
        if (m_evp_md_ctx == nullptr) {
            throw openssl_error_t(ERR_get_error());
        }
    }

    ~evp_md_ctx_wrapper_t() {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        EVP_MD_CTX_destroy(m_evp_md_ctx);
#else
        EVP_MD_CTX_free(m_evp_md_ctx);
#endif
    }

    EVP_MD_CTX *get() {
        return m_evp_md_ctx;
    }

private:
    EVP_MD_CTX *m_evp_md_ctx;
};

std::array<unsigned char, SHA256_DIGEST_LENGTH> detail::sha256(
        unsigned char const *data, size_t size) {
    std::array<unsigned char, SHA256_DIGEST_LENGTH> hash;

    evp_md_ctx_wrapper_t evp_md_ctx;
    if (EVP_DigestInit(evp_md_ctx.get(), EVP_sha256()) != 1) {
        throw openssl_error_t(ERR_get_error());
    }
    if (EVP_DigestUpdate(evp_md_ctx.get(), data, size) != 1) {
        throw openssl_error_t(ERR_get_error());
    }
    if (EVP_DigestFinal(evp_md_ctx.get(), hash.data(), nullptr) != 1) {
        throw openssl_error_t(ERR_get_error());
    }

    return hash;
}

}  // namespace crypto
