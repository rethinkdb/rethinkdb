// Copyright 2010-2016 RethinkDB, all rights reserved.
#include "crypto/hmac.hpp"

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/opensslv.h>

#include "crypto/error.hpp"

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)

inline HMAC_CTX *HMAC_CTX_new() {
    HMAC_CTX *tmp = reinterpret_cast<HMAC_CTX *>(OPENSSL_malloc(sizeof(HMAC_CTX)));
    if (tmp)
        HMAC_CTX_init(tmp);
    return tmp;
}

inline void HMAC_CTX_free(HMAC_CTX *ctx) {
    if (ctx) {
        HMAC_CTX_cleanup(ctx);
        OPENSSL_free(ctx);
    }
}

#endif

namespace crypto {

class hmac_ctx_wrapper_t {
public:
    hmac_ctx_wrapper_t() {
        m_hmac_ctx = HMAC_CTX_new();
        if (m_hmac_ctx == nullptr) {
            throw openssl_error_t(ERR_get_error());
        }
    }

    ~hmac_ctx_wrapper_t() {
        HMAC_CTX_free(m_hmac_ctx);
    }

    HMAC_CTX *get() {
        return m_hmac_ctx;
    }

private:
    HMAC_CTX *m_hmac_ctx;
};

std::array<unsigned char, SHA256_DIGEST_LENGTH> detail::hmac_sha256(
        unsigned char const *key,
        size_t key_size,
        unsigned char const *data,
        size_t data_size) {
    std::array<unsigned char, SHA256_DIGEST_LENGTH> hmac;

    hmac_ctx_wrapper_t hmac_ctx;
    if (HMAC_Init_ex(hmac_ctx.get(), key, key_size, EVP_sha256(), nullptr) != 1) {
        throw openssl_error_t(ERR_get_error());
    }
    if (HMAC_Update(hmac_ctx.get(), data, data_size) != 1) {
        throw openssl_error_t(ERR_get_error());
    }
    if (HMAC_Final(hmac_ctx.get(), hmac.data(), nullptr) != 1) {
        throw openssl_error_t(ERR_get_error());
    }

    return hmac;
}

}  // namespace crypto
