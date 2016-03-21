// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/crypto/random.hpp"

#include <openssl/rand.h>

#include "clustering/administration/auth/crypto/crypto_error.hpp"

void auth::crypto::detail::random_bytes(unsigned char *data, size_t size) {
    if (RAND_bytes(data, size) != 1) {
        // FIXME ERR_get_error
    }
}
