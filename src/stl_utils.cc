#include "stl_utils.hpp"

std::vector<std::string> split_string(const std::string &s, char sep) {
    const size_t n = s.size();

    std::vector<std::string> builder;
    size_t i = 0;
    for (;;) {
        size_t j = i;
        while (j != n && s[j] != sep) {
            ++j;
        }
        builder.push_back(s.substr(i, j - i));
        if (j == n) {
            return builder;
        }
        i = j + 1;
    }
}

