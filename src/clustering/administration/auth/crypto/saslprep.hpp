// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_CRYPTO_SASLPREP_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_CRYPTO_SASLPREP_HPP

#include <string>

namespace auth { namespace crypto {

std::string saslprep(std::string const &source);

} }  // namespace auth::crypto

#endif  // CLUSTERING_ADMINISTRATION_AUTH_CRYPTO_SASLPREP_HPP
