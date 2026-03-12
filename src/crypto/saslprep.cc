// Copyright 2010-2016 RethinkDB, all rights reserved.
#include "crypto/saslprep.hpp"

/* Commented out by srh while removing ICU dependency.  All the code using it in this
   file is commented out at this time. */
// #include <unicode/unistr.h>
// #include <unicode/usprep.h>

#include <string>

#include "crypto/error.hpp"

namespace crypto {

std::string saslprep(std::string const &source) {
    // SASLPrep implementation without ICU library.
    // This performs basic ASCII-only SASLPrep validation:
    // - Rejects non-ASCII characters that would require Unicode normalization
    // - This is a security measure to prevent Unicode equivalence attacks
    // Note: Full SASLPrep compliance would require ICU library for proper
    // Unicode normalization (NFKC), case folding, and character mapping.
    for (char c : source) {
        if (static_cast<unsigned char>(c) > 0x7F) {
            throw crypto::error_t("SASLPrep: non-ASCII characters require Unicode "
                                  "normalization which is not available without ICU library");
        }
        // Reject ASCII control characters (C0 controls 0x00-0x1F, DEL 0x7F)
        if ((c >= 0x00 && c <= 0x1F) || c == 0x7F) {
            throw crypto::error_t("SASLPrep: control characters are not allowed");
        }
    }
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

}  // namespace crypto
