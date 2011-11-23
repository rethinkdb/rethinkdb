#include "redis/redis_ext.hpp"

void toUpper(std::string &str) {
    for(unsigned i = 0; i < str.length(); i++) {
        str[i] = toupper(str[i]);
    }
}
