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
    std::string msg;
    switch (condition) {
        case 336027804:
            msg = "Received a plain HTTP request on the HTTPS port.";
            break;
        case 336027900:
            msg = "Unknown protocol. By default, RethinkDB requires at least TLSv1.2. "
                  "You can enable older TLS versions on the server through the "
                  "`--tls-min-protocol TLSv1` option.";
            break;
        case 336109761:
            msg = "No shared cipher. You can change the list of enabled ciphers on the "
                  "server through the `--tls-ciphers` option. Older OpenSSL clients "
                  "(e.g. on OS X) might require `--tls-ciphers "
                  "EECDH+AESGCM:EDH+AESGCM:AES256+EECDH:AES256+EDH:AES256-SHA`.";
            break;
        default:
            char const *openssl_message = ERR_reason_error_string(condition);
            msg = openssl_message == nullptr ? "unknown error" : openssl_message;
            break;
    }
    msg += strprintf(" (OpenSSL error %d)", condition);
    return msg;
}

}  // namespace crypto
