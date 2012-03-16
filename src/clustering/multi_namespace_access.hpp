#ifndef CLUSTERING_MULTI_NAMESPACE_ACCESS_HPP_
#define CLUSTERING_MULTI_NAMESPACE_ACCESS_HPP_

#include <string>

template <class protocol_t>
class multi_namespace_access_t {
public:
    class namespace_access_t {
    public:
        namespace_access_t(multi_namespace_access_t *, std::string);
        namespace_interface_t<protocol_t> *get();
    };
};

#endif
