#ifndef RPC_SEMILATTICE_JOINS_SET_HPP_
#define RPC_SEMILATTICE_JOINS_SET_HPP_

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

#endif /* RPC_SEMILATTICE_JOINS_SET_HPP_ */
