// Copyright 2010-2016 RethinkDB, all rights reserved.
#include "crypto/random.hpp"

#include <openssl/rand.h>

#include "crypto/error.hpp"

namespace crypto {

void detail::random_bytes(unsigned char *data, size_t size) {
    if (RAND_bytes(data, size) != 1) {
        throw openssl_error_t(ERR_get_error());
    }
}

}  // namespace crypto
