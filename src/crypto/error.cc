// Copyright 2010-2016 RethinkDB, all rights reserved.
#include "crypto/error.hpp"

#include "utils.hpp"

namespace crypto {

openssl_error_category_t::openssl_error_category_t() {

}

/* virtual */ char const *openssl_error_category_t::name() const noexcept {
    return "OpenSSL";
}

/* virtual */ std::string openssl_error_category_t::message(int condition) const {
    switch (condition) {
        case 336027804:
            return "Received a plain HTTP request on the HTTPS port.";
        default:
            char const *_message = ERR_reason_error_string(condition);
            return strprintf("%s (OpenSSL error %d)",
                             _message == nullptr ? "unknown error" : _message,
                             condition);
    }
}

}  // namespace crypto
