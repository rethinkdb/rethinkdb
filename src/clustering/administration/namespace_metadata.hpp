#ifndef __CLUSTERING_ADMINISTRATION_NAMESPACE_METADATA_HPP__
#define __CLUSTERING_ADMINISTRATION_NAMESPACE_METADATA_HPP__

#include <map>

#include "utils.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/bind.hpp>

#include "clustering/administration/datacenter_metadata.hpp"
#include "clustering/administration/json_adapters.hpp"
#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "clustering/immediate_consistency/query/metadata.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "clustering/reactor/directory_echo.hpp"
#include "clustering/reactor/metadata.hpp"
#include "http/json/json_adapter.hpp"
#include "rpc/semilattice/joins/deletable.hpp"
#include "rpc/semilattice/joins/vclock.hpp"
#include "rpc/serialize_macros.hpp"

typedef boost::uuids::uuid namespace_id_t;

/* This is the metadata for a single namespace of a specific protocol. */
template<class protocol_t>
class namespace_semilattice_metadata_t {
public:
    vclock_t<blueprint_t<protocol_t> > blueprint;

    vclock_t<datacenter_id_t> primary_datacenter;

    vclock_t<std::map<datacenter_id_t, int> > replica_affinities;

    vclock_t<std::set<typename protocol_t::region_t> > shards;

    RDB_MAKE_ME_SERIALIZABLE_4(blueprint, primary_datacenter, replica_affinities, shards);
};

//json adapter concept for namespace_semilattice_metadata_t
template <class ctx_t, class protocol_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(namespace_semilattice_metadata_t<protocol_t> *target, const ctx_t &) {
    typename json_adapter_if_t<ctx_t>::json_adapter_map_t res;
    res["blueprint"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<vclock_t<blueprint_t<protocol_t> >, ctx_t>(&target->blueprint));
    res["primary_datacenter"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<vclock_t<datacenter_id_t>, ctx_t>(&target->primary_datacenter));
    res["replica_affinities"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<vclock_t<std::map<datacenter_id_t, int> >, ctx_t>(&target->replica_affinities));
    return res;
}

template <class ctx_t, class protocol_t>
cJSON *render_as_json(namespace_semilattice_metadata_t<protocol_t> *target, const ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

template <class ctx_t, class protocol_t>
void apply_json_to(cJSON *change, namespace_semilattice_metadata_t<protocol_t> *target, const ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

template <class ctx_t, class protocol_t>
void on_subfield_change(namespace_semilattice_metadata_t<protocol_t> *, const ctx_t &) { }

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

//json adapter concept for namespaces_semilattice_metadata_t

template <class ctx_t, class protocol_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(namespaces_semilattice_metadata_t<protocol_t> *target, const ctx_t &) {
    typename json_adapter_if_t<ctx_t>::json_adapter_map_t res;
    res["namespaces"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_with_inserter_t<typename namespaces_semilattice_metadata_t<protocol_t>::namespace_map_t, ctx_t>(&target->namespaces, boost::bind(&generate_uuid)));
    return res;
}

template <class ctx_t, class protocol_t>
cJSON *render_as_json(namespaces_semilattice_metadata_t<protocol_t> *target, const ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

template <class ctx_t, class protocol_t>
void apply_json_to(cJSON *change, namespaces_semilattice_metadata_t<protocol_t> *target, const ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

template <class ctx_t, class protocol_t>
void on_subfield_change(namespaces_semilattice_metadata_t<protocol_t> *, const ctx_t &) { }

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
    std::map<namespace_id_t, directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > reactor_bcards;
    std::map<namespace_id_t, std::map<master_id_t, master_business_card_t<protocol_t> > > master_maps;

    RDB_MAKE_ME_SERIALIZABLE_2(reactor_bcards, master_maps);
};

struct namespace_metadata_ctx_t {
    boost::uuids::uuid us;
    explicit namespace_metadata_ctx_t(boost::uuids::uuid _us) 
        : us(_us)
    { }
};

#endif /* __CLUSTERING_ARCHITECT_METADATA_HPP__ */

