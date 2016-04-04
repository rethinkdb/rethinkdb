// Copyright 2010-2016 RethinkDB, all rights reserved.
#include "crypto/base64.hpp"

#include "crypto/error.hpp"

namespace crypto {

namespace detail {

static char base64_encode_map[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_encode(unsigned char const *data, size_t size) {
    std::string encoded;
    encoded.reserve(((size / 3) + (size % 3 > 0)) * 4);

    int32_t buffer;
    for (; size > 3; size -= 3) {
        buffer = (*data++) << 16;
        buffer += (*data++) << 8;
        buffer += (*data++);
        encoded.push_back(base64_encode_map[(buffer & 0x00fc0000) >> 18]);
        encoded.push_back(base64_encode_map[(buffer & 0x0003f000) >> 12]);
        encoded.push_back(base64_encode_map[(buffer & 0x00000fc0) >> 6]);
        encoded.push_back(base64_encode_map[(buffer & 0x0000003f)]);
    }

    switch (size) {
        case 2:
            buffer = (*data++) << 16;
            buffer += (*data++) << 8;
            encoded.push_back(base64_encode_map[(buffer & 0x00fc0000) >> 18]);
            encoded.push_back(base64_encode_map[(buffer & 0x0003f000) >> 12]);
            encoded.push_back(base64_encode_map[(buffer & 0x00000fc0) >> 6]);
            encoded.push_back('=');
            break;
        case 1:
            buffer = (*data++) << 16;
            encoded.push_back(base64_encode_map[(buffer & 0x00fc0000) >> 18]);
            encoded.push_back(base64_encode_map[(buffer & 0x0003f000) >> 12]);
            encoded.append(2, '=');
            break;
        case 0:
            break;
    };

    return encoded;
}

} // namespace detail

std::string base64_decode(std::string const &encoded) {
    std::string decoded;
    decoded.reserve((encoded.size() / 4) * 3);

    int32_t buffer = 0;
    std::string::const_iterator iterator = encoded.begin();
    while (iterator < encoded.end()) {
        for (int i = 0; i < 4; i++) {
            buffer <<= 6;
            if (*iterator >= 0x41 && *iterator <= 0x5a) {
                buffer |= *iterator - 0x41;
            } else if (*iterator >= 0x61 && *iterator <= 0x7a) {
                buffer |= *iterator - 0x47;
            } else if (*iterator >= 0x30 && *iterator <= 0x39) {
                buffer |= *iterator + 0x04;
            } else if (*iterator == 0x2b) {
                buffer |= 0x3e;
            } else if (*iterator == 0x2f) {
                buffer |= 0x3f;
            } else if (*iterator == '=') {
                switch (encoded.end() - iterator) {
                    case 2:
                        if (*++iterator == '=') {
                            decoded.push_back((buffer >> 10) & 0x000000ff);
                            return decoded;
                        } else {
                            throw error_t("Invalid padding in `" + encoded + "`");
                        }
                    case 1:
                        decoded.push_back((buffer >> 16) & 0x000000ff);
                        decoded.push_back((buffer >> 8) & 0x000000ff);
                        return decoded;
                    default:
                        throw error_t("Invalid padding in `" + encoded + "`");
                }
            } else {
                throw error_t("Invalid padding in `" + encoded + "`");
            }
            iterator++;
        }
        decoded.push_back((buffer >> 16) & 0x000000ff);
        decoded.push_back((buffer >> 8) & 0x000000ff);
        decoded.push_back(buffer & 0x000000ff);
    }

    return decoded;
}

}  // namespace crypto
