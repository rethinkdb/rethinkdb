#include "serializer/types.hpp"
#include <string>

// This could be faster, but it should only be used for debugging purposes anyway, so who cares?
std::string block_sequence_string(ser_block_sequence_id_t id) {
    if (!id) return std::string("0");
    // Generate the digits in reverse (little-endian) order
    std::string temp;
    while (id) {
        int digit = id % 10;
        id /= 10;
        temp.push_back('0' + digit);
    }
    std::string result;
    result.reserve(temp.length());
    for (std::string::reverse_iterator it = temp.rbegin(); it < temp.rend(); ++it) {
        result.push_back(*it);
    }
    return result;
}
