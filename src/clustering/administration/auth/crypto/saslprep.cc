#include "unicode/unistr.h"
#include "unicode/usprep.h"

#include <string>

#include "clustering/administration/auth/crypto/crypto_error.hpp"

#include <iostream>

namespace auth { namespace crypto {

std::string saslprep(std::string const &source) {
    // FIXME missing data
    return source;

    /* UErrorCode status = U_ZERO_ERROR;
    UStringPrepProfile *profile = usprep_openByType(USPREP_RFC4013_SASLPREP, &status);
    if (U_FAILURE(status)) {
        // FIXME error
    }

    icu::UnicodeString unicode_source = icu::UnicodeString::fromUTF8(source);
    icu::UnicodeString unicode_destination;

    status = U_ZERO_ERROR;
    int32_t length = usprep_prepare(
        profile,
        unicode_source.getBuffer(),
        unicode_source.length(),
        unicode_destination.getBuffer(unicode_destination.length()),
        unicode_destination.getCapacity(),
        USPREP_DEFAULT,
        nullptr, // UParseError parse_error
        &status);
    unicode_destination.releaseBuffer(length);
    if (status == U_BUFFER_OVERFLOW_ERROR) {
        status = U_ZERO_ERROR;
        length = usprep_prepare(
            profile,
            unicode_source.getBuffer(),
            unicode_source.length(),
            unicode_destination.getBuffer(length),
            unicode_destination.getCapacity(),
            USPREP_DEFAULT,
            nullptr, // UParseError parse_error,
            &status);
        unicode_destination.releaseBuffer(length);
    }
    if (U_FAILURE(status)) {
        // FIXME error
    }

    usprep_close(profile);

    std::string destination;
    unicode_destination.toUTF8String(destination);
    return destination; */
}

} }  // namespace auth::crypto
