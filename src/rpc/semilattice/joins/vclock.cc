#include "rpc/semilattice/joins/vclock.hpp"

#include "containers/uuid.hpp"
#include "utils.hpp"

namespace vclock_details {
bool dominates(const version_map_t &a, const version_map_t &b) {
    //check that a doesn't dominate b for any machine
    for (version_map_t::const_iterator it = a.begin();
         it != a.end();
         ++it) {
        version_map_t::const_iterator other_it = b.find(it->first);
        if ((other_it == b.end() && it->second > 0) || //zero is the defaul value
            other_it->second < it->second) {
            return false;
        }
    }

    //check if we have an element of b that's greater
    bool have_one_greater = false;
    for (version_map_t::const_iterator it = b.begin();
         it != b.end();
         ++it) {
        version_map_t::const_iterator other_it = a.find(it->first);
        if ((other_it == a.end() && it->second > 0) ||
            other_it->second < it->second) {
            have_one_greater = true;
            break;
        }
    }

    //when we get here we already know that a doesn't dominate b anywhere
    return have_one_greater;
}

version_map_t vmap_max(const version_map_t& x, const version_map_t &y) {
    version_map_t res = x;
    for (version_map_t::const_iterator it = y.begin();
         it != y.end();
         ++it) {
        int val = std::max(res[it->first], it->second);
        res[it->first] = val;
    }

    return res;
}

void print_version_map(const version_map_t &vm) {
    debugf("Version map:\n");
    for (version_map_t::const_iterator it = vm.begin();
         it != vm.end();
         ++it) {
        debugf("%s -> %d\n", uuid_to_str(it->first).c_str(), it->second);
    }
}
} //namespace vclock_details

