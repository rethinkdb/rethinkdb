#ifndef __CLUSTERING_ARCHITECT_METADATA_HPP__
#define __CLUSTERING_ARCHITECT_METADATA_HPP__

#include <map>

#include "utils.hpp"
#include <boost/uuid/uuid.hpp>

#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "clustering/immediate_consistency/query/metadata.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "rpc/semilattice/joins/deletable.hpp"
#include "rpc/semilattice/joins/vclock.hpp"
#include "rpc/serialize_macros.hpp"
#include "clustering/reactor/metadata.hpp"

typedef boost::uuids::uuid namespace_id_t;

/* This is the metadata for a single namespace of a specific protocol. */
template<class protocol_t>
class namespace_semilattice_metadata_t {
public:
    vclock_t<blueprint_t<protocol_t> > blueprint;

    RDB_MAKE_ME_SERIALIZABLE_1(blueprint);
};

/* semilattice concept for namespace_semilattice_metadata_t */
template <class protocol_t>
bool operator==(const namespace_semilattice_metadata_t<protocol_t>& a, const namespace_semilattice_metadata_t<protocol_t>& b) {
    return a.blueprint == b.blueprint;
}

template <class protocol_t>
void semilattice_join(namespace_semilattice_metadata_t<protocol_t> *a, const namespace_semilattice_metadata_t<protocol_t> &b) {
    semilattice_join(&a->blueprint, b.blueprint);
}

/* This is the metadata for all of the namespaces of a specific protocol. */
template <class protocol_t>
class namespaces_semilattice_metadata_t {
public:
    typedef std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > > namespace_map_t; 
    namespace_map_t namespaces;

    branch_history_t<protocol_t> branch_history;

    RDB_MAKE_ME_SERIALIZABLE_2(namespaces, branch_history);
};

//semilattice concept for namespaces_semilattice_metadata_t 
template <class protocol_t>
bool operator==(const namespaces_semilattice_metadata_t<protocol_t> &a, const namespaces_semilattice_metadata_t<protocol_t> &b) {
    return a.namespaces == b.namespaces && a.branch_history == b.branch_history;
}

template <class protocol_t>
void semilattice_join(namespaces_semilattice_metadata_t<protocol_t> *a, const namespaces_semilattice_metadata_t<protocol_t> &b) {
    semilattice_join(&a->namespaces, b.namespaces);
    semilattice_join(&a->branch_history, b.branch_history);
}

template <class protocol_t>
class namespaces_directory_metadata_t {
public:
    std::map<namespace_id_t, reactor_business_card_t<protocol_t> > reactor_bcards;
    std::map<namespace_id_t, std::map<master_id_t, master_business_card_t<protocol_t> > > master_maps;

    RDB_MAKE_ME_SERIALIZABLE_2(reactor_bcards, master_maps);
};

#endif /* __CLUSTERING_ARCHITECT_METADATA_HPP__ */

