#ifndef __CLUSTERING_ADMINISTRATION_METADATA_HPP_
#define __CLUSTERING_ADMINISTRATION_METADATA_HPP_

#include "clustering/administration/machine_metadata.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "clustering/administration/datacenter_metadata.hpp"
#include "memcached/protocol.hpp"
#include "memcached/protocol_json_adapter.hpp"
#include "mock/dummy_protocol.hpp"
#include "mock/dummy_protocol_json_adapter.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/serialize_macros.hpp"

class cluster_semilattice_metadata_t {
public:
    cluster_semilattice_metadata_t() { }
    explicit cluster_semilattice_metadata_t(const machine_id_t &us) {
        machines.machines.insert(std::make_pair(us, machine_semilattice_metadata_t()));
    }

    namespaces_semilattice_metadata_t<mock::dummy_protocol_t> dummy_namespaces;
    namespaces_semilattice_metadata_t<memcached_protocol_t> memcached_namespaces;
    machines_semilattice_metadata_t machines;
    std::map<datacenter_id_t, datacenter_semilattice_metadata_t> datacenters;

    RDB_MAKE_ME_SERIALIZABLE_4(dummy_namespaces, memcached_namespaces, machines, datacenters);
};

RDB_MAKE_SEMILATTICE_JOINABLE_4(cluster_semilattice_metadata_t, dummy_namespaces, memcached_namespaces, machines, datacenters);
RDB_MAKE_EQUALITY_COMPARABLE_4(cluster_semilattice_metadata_t, dummy_namespaces, memcached_namespaces, machines, datacenters);

//json adapter concept for cluster_semilattice_metadata_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(cluster_semilattice_metadata_t *target, const ctx_t &ctx) {
    typename json_adapter_if_t<ctx_t>::json_adapter_map_t res;
    res["dummy_namespaces"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t>, ctx_t>(&target->dummy_namespaces));
    res["memcached_namespaces"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<namespaces_semilattice_metadata_t<memcached_protocol_t>, ctx_t>(&target->memcached_namespaces));
    res["machines"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<machines_semilattice_metadata_t, ctx_t>(&target->machines));
    res["datacenters"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_with_inserter_t<typename std::map<datacenter_id_t, datacenter_semilattice_metadata_t>, ctx_t>(&target->datacenters, boost::bind(&generate_uuid)));
    res["me"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<boost::uuids::uuid, ctx_t>(const_cast<boost::uuids::uuid*>(&ctx.us)));
    return res;
}

template <class ctx_t>
cJSON *render_as_json(cluster_semilattice_metadata_t *target, const ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

template <class ctx_t>
void apply_json_to(cJSON *change, cluster_semilattice_metadata_t *target, const ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

template <class ctx_t>
void on_subfield_change(cluster_semilattice_metadata_t *, const ctx_t &) { }

class cluster_directory_metadata_t {
public:
    cluster_directory_metadata_t() { }
    explicit cluster_directory_metadata_t(machine_id_t mid) : machine_id(mid) { }

    namespaces_directory_metadata_t<mock::dummy_protocol_t> dummy_namespaces;
    namespaces_directory_metadata_t<memcached_protocol_t> memcached_namespaces;

    /* Tell the other peers what our machine ID is */
    machine_id_t machine_id;

    std::list<clone_ptr_t<local_issue_t> > local_issues;

    RDB_MAKE_ME_SERIALIZABLE_3(dummy_namespaces, memcached_namespaces, machine_id);
};

#endif
