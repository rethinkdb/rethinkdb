// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RPC_SEMILATTICE_JOINS_MAP_HPP_
#define RPC_SEMILATTICE_JOINS_MAP_HPP_

#include <map>

/* We join `std::map`s by taking their union and resolving conflicts by doing a
semilattice join on the values. */

namespace std {

template<class key_t, class value_t>
void semilattice_join(std::map<key_t, value_t> *a, const std::map<key_t, value_t> &b) {
    for (typename std::map<key_t, value_t>::const_iterator it = b.begin(); it != b.end(); it++) {
        typename std::map<key_t, value_t>::iterator it2 = a->find((*it).first);
        if (it2 == a->end()) {
            (*a)[(*it).first] = (*it).second;
        } else {
            semilattice_join(&(*it2).second, (*it).second);
        }
    }
}

}   /* namespace std */

#endif /* RPC_SEMILATTICE_JOINS_MAP_HPP_ */
