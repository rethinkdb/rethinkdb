// Copyright 2010-2016 RethinkDB, all rights reserved.
#include "crypto/initialization_guard.hpp"

#include <openssl/err.h>
#include <openssl/opensslv.h>
#include <openssl/ssl.h>

namespace crypto {

initialization_guard_t::initialization_guard_t() {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    SSL_library_init();
    SSL_load_error_strings();
#else
    OPENSSL_init_ssl(0, nullptr);
    OPENSSL_init_crypto(0, nullptr);
#endif
}

initialization_guard_t::~initialization_guard_t() {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    ERR_free_strings();
#else
    OPENSSL_cleanup();
#endif
}

}  // namespace crypto
