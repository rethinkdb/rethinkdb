// Copyright 2010-2016 RethinkDB, all rights reserved.
#include "crypto/error.hpp"

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
            char const *message = ERR_reason_error_string(condition);
            if (message == nullptr) {
                return "unknown error";
            } else {
                return message;
            }
    }
}

}  // crypto
