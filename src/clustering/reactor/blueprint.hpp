#ifndef __CLUSTERING_REACTOR_BLUEPRINT_HPP__
#define __CLUSTERING_REACTOR_BLUEPRINT_HPP__

#include "utils.hpp"
#include <boost/optional.hpp>

#include "rpc/connectivity/connectivity.hpp"

template<class protocol_t>
class blueprint_t {
public:
    enum role_t {
        role_primary,
        role_secondary,
        role_listener,
        role_nothing
    };

    void assert_valid() THROWS_NOTHING;

    std::map<peer_id_t, std::map<typename protocol_t::region_t, role_t> > peers;
};

#endif /* __CLUSTERING_REACTOR_BLUEPRINT_HPP__ */
