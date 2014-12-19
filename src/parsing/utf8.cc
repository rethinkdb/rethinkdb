#include "parsing/utf8.hpp"

#include <string.h>
#include <string>

// #include "unicode/utypes.h"
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
inline bool is_valid_internal(const Iterator & begin, const Iterator & end,
                              reason_t *reason) {
    iterator_t<Iterator> p(begin, end);
    while (!p.is_done()) {
        if (p.saw_error()) {
            *reason = p.error_explanation();
            return false;
        }
        ++p;
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

bool is_valid(const char *str, reason_t *reason) {
    size_t len = strlen(str);
    const char *end = str + len;
    return is_valid_internal(str, end, reason);
}

template <class Iterator>
inline void iterator_t<Iterator>::fail(const char *why) {
    reason.position = index - 1; // because we increment after looking
    reason.explanation = why;
    last_seen = U'\uFFFD';      // U+FFFD is the "replacement character"
}

template <class Iterator>
inline char iterator_t<Iterator>::next_octet() {
    char result = *position;
    ++index;
    ++position;
    return result;
}

template <class Iterator>
void iterator_t<Iterator>::advance(void) {
    reason.position = 0;
    reason.explanation = "";

    if (position == end) {
        seen_end = true;
        fail("End of string reached");
        return;
    }

    char current = next_octet();

    if (is_standalone(current)) {
        // 0xxxxxxx - ASCII character
        last_seen = current;
    } else if (is_twobyte_start(current)) {
        // 110xxxxx - two character multibyte
        char32_t result = extract_and_shift(current, HIGH_THREE_BITS, 6);
        if (!check()) return;
        result |= continuation_data(next_octet());
        if (result < 0x0080) {
            // Not minimum bytes required to represent character, so wrong
            fail("Overlong encoding seen");
        } else {
            last_seen = result;
        }
    } else if (is_threebyte_start(current)) {
        // 1110xxxx - three character multibyte
        char32_t result = extract_and_shift(current, HIGH_FOUR_BITS, 12);
        if (!check()) return;
        result |= continuation_data(next_octet()) << 6;
        if (!check()) return;
        result |= continuation_data(next_octet());
        if (result < 0x0800) {
            // Not minimum bytes required to represent character, so wrong
            fail("Overlong encoding seen");
        } else {
            last_seen = result;
        }
    } else if (is_fourbyte_start(current)) {
        // 11110xxx - four character multibyte
        char32_t result = extract_and_shift(current, HIGH_FIVE_BITS, 18);
        if (!check()) return;
        result |= continuation_data(next_octet()) << 12;
        if (!check()) return;
        result |= continuation_data(next_octet()) << 6;
        if (!check()) return;
        result |= continuation_data(next_octet());
        if (result < 0x10000) {
            // Not minimum bytes required to represent character, so wrong
            fail("Overlong encoding seen");
        } else if (result > 0x10FFFF) {
            // UTF-8 defined by RFC 3629 to end at U+10FFFF now
            fail("Non-Unicode character encoded (beyond U+10FFFF)");
        } else {
            last_seen = result;
        }
    } else {
        // high bit character outside of a surrogate context
        fail("Invalid initial byte seen");
    }
}

template <class Iterator>
Iterator && fail(const char *explanation,
                const Iterator &start, Iterator &&position,
                char32_t *codepoint, reason_t *reason) {
    reason->explanation = explanation;
    reason->position = position - start;
    // U+FFFD is the "replacement character", commonly used
    // for parsing errors like this
    *codepoint = U'\uFFFD';
    return std::move(position);
}

template <class Iterator>
const char *is_valid_continuation_byte(const Iterator &position, const Iterator &end) {
    if (position == end) {
        return "Expected continuation byte, saw end of string";
    } else if (!is_continuation(*position)) {
        return "Expected continuation byte, saw something else";
    } else {
        return 0;
    }
}

template <class Iterator>
Iterator && next_codepoint(const Iterator &start, const Iterator &end,
                          char32_t *codepoint, reason_t *reason) {
    Iterator position = start;
    reason->explanation = "";
    reason->position = 0;

    if (position == end) {
        return std::move(position);
    }

    char current = *position++;
    const char *explanation;

    if (is_standalone(current)) {
        // 0xxxxxxx - ASCII character
        *codepoint = current;
        return std::move(position);
    } else if (is_twobyte_start(current)) {
        // 110xxxxx - two character multibyte
        *codepoint = extract_and_shift(current, HIGH_THREE_BITS, 6);
        explanation = is_valid_continuation_byte(position, end);
        if (explanation != 0) {
            return fail(explanation, start, std::move(position), codepoint, reason);
        }
        *codepoint |= continuation_data(*position++);
        if (*codepoint < 0x0080) {
            // Not minimum bytes required to represent character, so wrong
            return fail("Overlong encoding seen",
                        start, std::move(position), codepoint, reason);
        } else {
            return std::move(position);
        }
    } else if (is_threebyte_start(current)) {
        // 1110xxxx - three character multibyte
        *codepoint = extract_and_shift(current, HIGH_FOUR_BITS, 12);
        explanation = is_valid_continuation_byte(position, end);
        if (explanation != 0) {
            return fail(explanation, start, std::move(position), codepoint, reason);
        }
        *codepoint |= continuation_data(*position++) << 6;
        explanation = is_valid_continuation_byte(position, end);
        if (explanation != 0) {
            return fail(explanation, start, std::move(position), codepoint, reason);
        }
        *codepoint |= continuation_data(*position++);
        if (*codepoint < 0x0800) {
            // Not minimum bytes required to represent character, so wrong
            return fail("Overlong encoding seen",
                        start, std::move(position), codepoint, reason);
        } else {
            return std::move(position);
        }
    } else if (is_fourbyte_start(current)) {
        // 11110xxx - four character multibyte
        *codepoint = extract_and_shift(current, HIGH_FIVE_BITS, 18);
        explanation = is_valid_continuation_byte(position, end);
        if (explanation != 0) {
            return fail(explanation, start, std::move(position), codepoint, reason);
        }
        *codepoint |= continuation_data(*position++) << 12;
        explanation = is_valid_continuation_byte(position, end);
        if (explanation != 0) {
            return fail(explanation, start, std::move(position), codepoint, reason);
        }
        *codepoint |= continuation_data(*position++) << 6;
        explanation = is_valid_continuation_byte(position, end);
        if (explanation != 0) {
            return fail(explanation, start, std::move(position), codepoint, reason);
        }
        *codepoint |= continuation_data(*position++);
        if (*codepoint < 0x10000) {
            // Not minimum bytes required to represent character, so wrong
            return fail("Overlong encoding seen",
                        start, std::move(position), codepoint, reason);
        } else if (*codepoint > 0x10FFFF) {
            // UTF-8 defined by RFC 3629 to end at U+10FFFF, as Unicode does
            return fail("Non-Unicode character encoded (beyond U+10FFFF)",
                        start, std::move(position), codepoint, reason);
        } else {
            return std::move(position);
        }
    } else {
        // high bit character outside of a surrogate context
        return fail("Invalid initial byte seen", start, std::move(position),
                    codepoint, reason);
    }
}

template <class Iterator>
inline bool iterator_t<Iterator>::check() {
    bool result;
    ++index; // readahead
    if (position == end) {
        fail("Expected continuation byte, saw end of string");
        result = false;
    } else if (!is_continuation(*position)) {
        fail("Expected continuation byte, saw something else");
        result = false;
    } else {
        result = true;
    }
    --index;
    return result;
}

/* inline bool is_combining_character(char32_t char)
{
    return (U_GET_GC_MASK(char) & U_GC_M_MASK);
}*/



template const char *&&
next_codepoint<const char *>(const char * const &start, const char * const &end,
                             char32_t *codepoint, reason_t *reason);
template std::string::iterator &&
next_codepoint<std::string::iterator>(const std::string::iterator &start,
                                      const std::string::iterator &end,
                                      char32_t *codepoint, reason_t *reason);
template std::string::const_iterator &&
next_codepoint<std::string::const_iterator>(const std::string::const_iterator &start,
                                            const std::string::const_iterator &end,
                                            char32_t *codepoint, reason_t *reason);

template class iterator_t<std::string::const_iterator>;
template class iterator_t<const char *>;

} // namespace utf8
