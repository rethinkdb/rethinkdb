// Copyright 2010-2016 RethinkDB, all rights reserved.
#ifndef CRYPTO_SASLPREP_HPP_
#define CRYPTO_SASLPREP_HPP_

#include <string>

namespace crypto {

std::string saslprep(std::string const &source);

}  // namespace crypto

#endif  // CRYPTO_SASLPREP_HPP_
