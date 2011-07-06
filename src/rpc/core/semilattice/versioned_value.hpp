#ifndef __RPC_CORE_SEMILATTICE_VERSIONED_VALUE_HPP__
#define __RPC_CORE_SEMILATTICE_VERSIONED_VALUE_HPP__

#include "errors.hpp"
#include "rpc/core/semilattice/semilattice.hpp"

/*
An sl_versioned_value_t is a single value together with a version number. It
defines a trivial (totally ordered!) semilattice, where a >= b whenever a.version
>= b.version. The case of a.version == b.version is forbidden, unless a == b.
*/

template<typename value_t> class sl_versioned_value_t :
    public constructive_semilattice_t<sl_versioned_value_t<value_t> > {

public:
    sl_versioned_value_t(const value_t &v, const unsigned int version) : value(v), sl_version(version) {
    }
    
    void sl_join(const sl_versioned_value_t<value_t> &x) {        
        rassert(sl_version != x.sl_version || value == x.value, "Ambiguous join");
        if (x.sl_version > sl_version) {
            *this = x;
        }
    }
    
    unsigned int get_sl_version() const {
        return sl_version;
    }
    
    const value_t &get_value() const {
        return value;
    }

private:
    unsigned int sl_version;
    value_t value;
};

#endif // __RPC_CORE_SEMILATTICE_VERSIONED_VALUE_HPP__
