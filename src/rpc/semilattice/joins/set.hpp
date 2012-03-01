#ifndef __RPC_SEMILATTICE_JOINS_SET_HPP__
#define __RPC_SEMILATTICE_JOINS_SET_HPP__

#include <set>
#include <algorithm>

namespace std {

template <class T>
void semilattice_join(std::set<T> *a, const std::set<T> &b) {
    for (typename std::set<T>::iterator it  = b.begin();
                                        it != b.end();
                                        it++) {
        a->insert(*it);
    }
}

}

#endif
