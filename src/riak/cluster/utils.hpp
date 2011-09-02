#ifndef __RIAK_CLUSTER_UTILS_HPP__
#define __RIAK_CLUSTER_UTILS_HPP__

namespace riak {
namespace utils {

template <class E>
std::set<E> singleton_set(E e) {
    std::set<E> res;
    res.insert(e);
    return res;
}

template <class E>
std::vector<E> singleton_vector(E e) {
    std::vector<E> res;
    res.push_back(e);
    return res;
}

} //namespace utils 
} //namespace riak 

#endif
