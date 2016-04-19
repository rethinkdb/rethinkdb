// Copyright 2010-2016 RethinkDB, all rights reserved.
#ifndef CRYPTO_ERROR_HPP
#define CRYPTO_ERROR_HPP

#include <openssl/err.h>

#include <exception>
#include <stdexcept>
#include <string>
#include <system_error>

namespace crypto {

class error_t : public std::runtime_error {
public:
    explicit error_t(std::string const &_what)
        : std::runtime_error(_what) {
    }
};

static const class openssl_error_category_t : public std::error_category {
public:
    openssl_error_category_t();

    /* virtual */ char const *name() const noexcept;
    /* virtual */ std::string message(int condition) const;
} openssl_error_category;

class openssl_error_t : public std::system_error {
public:
    explicit openssl_error_t(unsigned long ec)  // NOLINT(runtime/int)
        : std::system_error(ec, openssl_error_category) {
    }
};

}  // namespace crypto

#endif  // CRYPTO_ERROR_HPP
