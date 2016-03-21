// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_CRYPTO_CRYPTO_ERROR_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_CRYPTO_CRYPTO_ERROR_HPP

#include <stdexcept>
#include <string>

namespace auth { namespace crypto {

class crypto_error_t : public std::runtime_error {
public:
    crypto_error_t()
        : std::runtime_error("FUCK") { // FIXME
    }
};

} }  // namespace auth::crypto

#endif  // CLUSTERING_ADMINISTRATION_AUTH_CRYPTO_CRYPTO_ERROR_HPP
