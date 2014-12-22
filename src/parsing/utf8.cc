#include "parsing/utf8.hpp"

#include <string>
#include <string.h>

#include "rdb_protocol/datum_string.hpp"

namespace utf8 {

static unsigned int HIGH_BIT = 0x80;
static unsigned int HIGH_TWO_BITS = 0xC0;
static unsigned int HIGH_THREE_BITS = 0xE0;
static unsigned int HIGH_FOUR_BITS = 0xF0;
static unsigned int HIGH_FIVE_BITS = 0xF8;

inline bool is_standalone(char c) {
    // 0xxxxxxx - ASCII character
    return (c & HIGH_BIT) == 0;
}

inline bool is_twobyte_start(char c) {
    // 110xxxxx - two character multibyte
    return (c & HIGH_THREE_BITS) == HIGH_TWO_BITS;
}

inline bool is_threebyte_start(char c) {
    // 1110xxxx - three character multibyte
    return (c & HIGH_FOUR_BITS) == HIGH_THREE_BITS;
}

inline bool is_fourbyte_start(char c) {
    // 11110xxx - four character multibyte
    return (c & HIGH_FIVE_BITS) == HIGH_FOUR_BITS;
}

inline bool is_continuation(char c) {
    // 10xxxxxx - continuation character
    return ((c & HIGH_TWO_BITS) == HIGH_BIT);
}

inline unsigned int extract_bits(char c, unsigned int bits) {
    return ((c & ~bits) & 0xFF);
}

inline unsigned int continuation_data(char c) {
    return extract_bits(c, HIGH_TWO_BITS);
}

inline unsigned int extract_and_shift(char c, unsigned int bits, unsigned int amount) {
    return extract_bits(c, bits) << amount;
}

template <class Iterator>
inline bool check_continuation(const Iterator &p, const Iterator &end,
                               size_t position, reason_t *reason) {
    if (p == end) {
        reason->position = position;
        reason->explanation = "Expected continuation byte, saw end of string";
        return false;
    }
    if (!is_continuation(*p)) {
        reason->position = position;
        reason->explanation = "Expected continuation byte, saw something else";
        return false;
    }
    return true;
}

template <class Iterator>
inline bool is_valid_internal(const Iterator &begin, const Iterator &end,
                              reason_t *reason) {
    Iterator p = begin;
    size_t position = 0;
    while (p != end) {
        if (is_standalone(*p)) {
            // 0xxxxxxx - ASCII character
            // don't need to do anything
        } else if (is_twobyte_start(*p)) {
            // 110xxxxx - two character multibyte
            unsigned int result = extract_and_shift(*p, HIGH_THREE_BITS, 6);
            ++p; ++position;
            if (!check_continuation(p, end, position, reason)) return false;
            result |= continuation_data(*p);
            if (result < 0x0080) {
                // can be represented in one byte, so using two is illegal
                reason->position = position;
                reason->explanation = "Overlong encoding seen";
                return false;
            }
        } else if (is_threebyte_start(*p)) {
            // 1110xxxx - three character multibyte
            unsigned int result = extract_and_shift(*p, HIGH_FOUR_BITS, 12);
            ++p; ++position;
            if (!check_continuation(p, end, position, reason)) return false;
            result |= continuation_data(*p) << 6;
            ++p; ++position;
            if (!check_continuation(p, end, position, reason)) return false;
            result |= continuation_data(*p);
            if (result < 0x0800) {
                // can be represented in two bytes, so using three is illegal
                reason->position = position;
                reason->explanation = "Overlong encoding seen";
                return false;
            }
        } else if (is_fourbyte_start(*p)) {
            // 11110xxx - four character multibyte
            unsigned int result = extract_and_shift(*p, HIGH_FIVE_BITS, 18);
            ++p; ++position;
            if (!check_continuation(p, end, position, reason)) return false;
            result |= continuation_data(*p) << 12;
            ++p; ++position;
            if (!check_continuation(p, end, position, reason)) return false;
            result |= continuation_data(*p) << 6;
            ++p; ++position;
            if (!check_continuation(p, end, position, reason)) return false;
            result |= continuation_data(*p);
            if (result < 0x10000) {
                // can be represented in three bytes, so using four is illegal
                reason->position = position;
                reason->explanation = "Overlong encoding seen";
                return false;
            }
            if (result > 0x10FFFF) {
                // UTF-8 defined by RFC 3629 to end at U+10FFFF now
                reason->position = position;
                reason->explanation = "Non-Unicode character encoded (beyond U+10FFFF)";
                return false;
            }
        } else {
            // high bit character outside of a surrogate context
            reason->position = position;
            reason->explanation = "Invalid initial byte seen";
            return false;
        }
        ++p; ++position;
    }
    return true;
}

bool is_valid(const datum_string_t &str) {
    reason_t reason;
    return is_valid_internal(str.data(), str.data() + str.size(), &reason);
}

bool is_valid(const std::string &str) {
    reason_t reason;
    return is_valid_internal(str.begin(), str.end(), &reason);
}

bool is_valid(const char *start, const char *end) {
    reason_t reason;
    return is_valid_internal(start, end, &reason);
}

bool is_valid(const datum_string_t &str, reason_t *reason) {
    return is_valid_internal(str.data(), str.data() + str.size(), reason);
}

bool is_valid(const std::string &str, reason_t *reason) {
    return is_valid_internal(str.begin(), str.end(), reason);
}

bool is_valid(const char *start, const char *end, reason_t *reason) {
    return is_valid_internal(start, end, reason);
}

} // namespace utf8
