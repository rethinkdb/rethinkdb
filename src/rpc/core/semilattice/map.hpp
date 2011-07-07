#ifndef __RPC_CORE_SEMILATTICE_MAP_HPP__
#define __RPC_CORE_SEMILATTICE_MAP_HPP__

#include <map>
#include <set>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>

#include "errors.hpp"
#include "rpc/core/semilattice/semilattice.hpp"

/*
TODO! Describe semantics and interface
value_t must be a constructive_semilattice_t
*/

template<typename key_t, typename value_t> class sl_map_t :
    public constructive_semilattice_t<sl_map_t<key_t, value_t> > {

public:
    const std::map<key_t, value_t> &get_map() const {
        return members;
    }
    
    void insert(const key_t &key, const value_t &value) {
        typename std::map<key_t, value_t>::iterator existing_member = members.find(key);
        if (existing_member == members.end()) {
            members.insert(std::pair<key_t, value_t>(key, value));
        } else {
            existing_member->second.sl_join(value);
        }
    }
    
    void remove(const key_t &key) {
        rassert(members.find(key) != members.end());
        members.erase(key);
        defunct.insert(key);
    }

    void sl_join(const sl_map_t<key_t, value_t> &x) {
        for (typename std::set<key_t>::const_iterator def = x.defunct.begin(); def != x.defunct.end(); ++def) {
            members.erase(*def);
        }
        
        for (typename std::map<key_t, value_t>::const_iterator mem = x.members.begin(); mem != x.members.end(); ++mem) {
            if (defunct.find(mem->first) == defunct.end()) {
                insert(mem->first, mem->second);
            }
        }
        
        defunct.insert(x.defunct.begin(), x.defunct.end());
    }

private:
    std::map<key_t, value_t> members;
    std::set<key_t> defunct;
    
private:
    // Serialization code
    friend class boost::serialization::access;
    template<typename Archive> void serialize(Archive &ar, UNUSED const unsigned int version)
    {
        ar & members;
        ar & defunct;
    }
};

// TODO: Would an sl_unique_map_t be useful?
// It would be implemented as an sl_map_t where value_t is such that
// it always fails on calls to value_t::sl_join().

#endif // __RPC_CORE_SEMILATTICE_MAP_HPP__
