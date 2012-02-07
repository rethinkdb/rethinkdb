#ifndef __CLUSTERING_MULTI_NAMESPACE_ACCESS_HPP__
#define __CLUSTERING_MULTI_NAMESPACE_ACCESS_HPP__

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
