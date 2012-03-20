#ifndef CLUSTERING_ADMINISTRATION_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_METADATA_HPP_

#include "errors.hpp"
#include <boost/serialization/list.hpp>
#include <sstream>

#include "clustering/administration/datacenter_metadata.hpp"
#include "clustering/administration/issues/local.hpp"
#include "clustering/administration/machine_metadata.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "http/json/json_adapter.hpp"
#include "memcached/protocol.hpp"
#include "memcached/protocol_json_adapter.hpp"
#include "mock/dummy_protocol.hpp"
#include "mock/dummy_protocol_json_adapter.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/serialize_macros.hpp"

class cluster_semilattice_metadata_t {
public:
    cluster_semilattice_metadata_t() { }

    namespaces_semilattice_metadata_t<mock::dummy_protocol_t> dummy_namespaces;
    namespaces_semilattice_metadata_t<memcached_protocol_t> memcached_namespaces;
    machines_semilattice_metadata_t machines;
    datacenters_semilattice_metadata_t datacenters;

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
    res["datacenters"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<datacenters_semilattice_metadata_t, ctx_t>(&target->datacenters));
    res["me"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_temporary_adapter_t<boost::uuids::uuid, ctx_t>(ctx.us));
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

    RDB_MAKE_ME_SERIALIZABLE_4(dummy_namespaces, memcached_namespaces, machine_id, local_issues);
};

// json adapter concept for directory_echo_wrapper_t
template <typename T,class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(directory_echo_wrapper_t<T> *target, const ctx_t &ctx) {
    return get_json_subfields(&target->internal, ctx);
}

template <typename T,class ctx_t>
cJSON *render_as_json(directory_echo_wrapper_t<T> *target, const ctx_t &ctx) {
    return render_as_json(&target->internal, ctx);
}

template <typename T,class ctx_t>
void apply_json_to(cJSON *change, directory_echo_wrapper_t<T> *target, const ctx_t &ctx) {
    apply_json_to(change, &target->internal, ctx);
}

template <typename T,class ctx_t>
void on_subfield_change(directory_echo_wrapper_t<T> *target, const ctx_t &ctx) {
    on_subfield_change(&target->internal, ctx);
}

// json adapter concept for reactor_business_card_t
template <typename protocol_t,class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(reactor_business_card_t<protocol_t> *target, const ctx_t &ctx) {
    typename json_adapter_if_t<ctx_t>::json_adapter_map_t res;
    for (typename reactor_business_card_t<protocol_t>::activity_map_t::const_iterator i = target->activities.begin(); i != target->activities.end(); ++i) {
        typename protocol_t::region_t key = i->second.first;
        typename reactor_business_card_t<protocol_t>::activity_t val = i->second.second;

        std::string key_str = get_string(render_as_json(&key, ctx));

        std::string val_str;
        {
            std::stringstream val_stream(val_str, std::stringstream::out | std::stringstream::binary);
            val_stream << val;
        }

        res[key_str] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_temporary_adapter_t<std::string, ctx_t>(val_str));
    }
    return res;
}

template <typename protocol_t,class ctx_t>
cJSON *render_as_json(reactor_business_card_t<protocol_t> *target, const ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

template <typename protocol_t,class ctx_t>
void apply_json_to(cJSON *, reactor_business_card_t<protocol_t> *, const ctx_t &) {
    // FIXME?: this method exists to please the compiler
}

template <typename protocol_t,class ctx_t>
void on_subfield_change(reactor_business_card_t<protocol_t> *, const ctx_t &) {
    // FIXME?: this method exists to please the compiler
}


//  json adapter concept for cluster_directory_metadata_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(cluster_directory_metadata_t *target, const ctx_t &) {
    typename json_adapter_if_t<ctx_t>::json_adapter_map_t res;
    res["dummy_namespaces"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_read_only_adapter_t<namespaces_directory_metadata_t<mock::dummy_protocol_t>, ctx_t>(&target->dummy_namespaces));
    res["memcached_namespaces"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<namespaces_directory_metadata_t<memcached_protocol_t>, ctx_t>(&target->memcached_namespaces));
    res["machine_id"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_temporary_adapter_t<machine_id_t, ctx_t>(target->machine_id)); // TODO: should be 'me'?
    return res;
}

template <class ctx_t>
cJSON *render_as_json(cluster_directory_metadata_t *target, const ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

template <class ctx_t>
void apply_json_to(cJSON *change, cluster_directory_metadata_t *target, const ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

template <class ctx_t>
void on_subfield_change(cluster_directory_metadata_t *, const ctx_t &) { }

#endif  // CLUSTERING_ADMINISTRATION_METADATA_HPP_
