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

inline bool is_standalone(char c)
{
    // 0xxxxxxx - ASCII character
    return (c & HIGH_BIT) == 0;
}

inline bool is_twobyte_start(char c)
{
    // 110xxxxx - two character multibyte
    return (c & HIGH_THREE_BITS) == HIGH_TWO_BITS;
}

inline bool is_threebyte_start(char c)
{
    // 1110xxxx - three character multibyte
    return (c & HIGH_FOUR_BITS) == HIGH_THREE_BITS;
}

inline bool is_fourbyte_start(char c)
{
    // 11110xxx - four character multibyte
    return (c & HIGH_FIVE_BITS) == HIGH_FOUR_BITS;
}

inline bool is_continuation(char c)
{
    return ((c & HIGH_TWO_BITS) != HIGH_BIT);
}

inline unsigned int continuation_data(char c)
{
    return ((c & ~HIGH_TWO_BITS) & 0xFF);
}

inline unsigned int extract_bits(char c, unsigned int bits)
{
    return ((c & ~bits) & 0xFF);
}

inline unsigned int extract_and_shift(char c, unsigned int bits, unsigned int amount)
{
    return extract_bits(c, bits) << amount;
}

template <class Iterator>
inline bool is_valid_byte(Iterator & p, const Iterator & end)
{
    if (is_standalone(*p)) {
        // 0xxxxxxx - ASCII character
        return true;
    } else if (is_twobyte_start(*p)) {
        // 110xxxxx - two character multibyte
        unsigned int result = extract_and_shift(*p, HIGH_THREE_BITS, 6);
        ++p;
        if (p == end) return false;
        if (is_continuation(*p)) return false;
        result |= continuation_data(*p);
        if (result < 0x0080) {
            // can be represented in one byte, so using two is illegal
            return false;
        }
        return true;
    } else if (is_threebyte_start(*p)) {
        // 1110xxxx - three character multibyte
        unsigned int result = extract_and_shift(*p, HIGH_FOUR_BITS, 12);
        ++p;
        if (p == end) return false;
        if (is_continuation(*p)) return false;
        result |= continuation_data(*p) << 6;
        p++;
        if (p == end) return false;
        if (is_continuation(*p)) return false;
        result |= continuation_data(*p);
        if (result < 0x0800) {
            // can be represented in two bytes, so using three is illegal
            return false;
        }
        return true;
    } else if (is_fourbyte_start(*p)) {
        // 11110xxx - four character multibyte
        unsigned int result = extract_and_shift(*p, HIGH_FIVE_BITS, 18);
        ++p;
        if (p == end) return false;
        if (is_continuation(*p)) return false;
        result |= continuation_data(*p) << 12;
        ++p;
        if (p == end) return false;
        if (is_continuation(*p)) return false;
        result |= continuation_data(*p) << 6;
        ++p;
        if (is_continuation(*p)) return false;
        result |= continuation_data(*p);
        if (result < 0x10000) {
            // can be represented in three bytes, so using four is illegal
            return false;
        }
        return true;
    } else {
        // high bit character outside of a surrogate context
        return false;
    }
}

template <class Iterator>
inline bool is_valid_int(const Iterator & begin, const Iterator & end)
{
    Iterator p = begin;
    while (p != end) {
        if (!is_valid_byte(p, end)) return false;
        ++p;
    }
    return true;
}

bool is_valid(const datum_string_t &str)
{
    return is_valid_int(str.data(), str.data() + str.size());
}

bool is_valid(const std::string &str)
{
    return is_valid_int(str.begin(), str.end());
}

bool is_valid(const char *str)
{
    size_t len = strlen(str);
    const char *end = str + len;
    return is_valid_int(str, end);
}

};
