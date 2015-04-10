#include "kh_debug.hpp"

#include "errors.hpp"
#include <boost/optional.hpp>

std::vector<std::pair<boost::optional<key_range_t>, std::string> > key_history;

void khd_all(const std::string &msg) {
    debugf("%s\n", msg.c_str());
    key_history.push_back(std::make_pair(boost::optional<key_range_t>(), msg));
}

void khd_range(const key_range_t &kr, const std::string &msg) {
    key_history.push_back(std::make_pair(boost::optional<key_range_t>(kr), msg));
}

void khd_dump(const store_key_t &key) {
    debugf_print("Key history dump for", key);
    for (const auto &pair : key_history) {
        if (!static_cast<bool>(pair.first)) {
            fprintf(stderr, "---------- %s ----------\n", pair.second.c_str());
        } else if (pair.first->contains_key(key)) {
            fprintf(stderr, "%s %s\n", debug_strprint(*pair.first).c_str(), pair.second.c_str());
        }
    }
}

