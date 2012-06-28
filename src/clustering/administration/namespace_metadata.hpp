#ifndef CLUSTERING_ADMINISTRATION_NAMESPACE_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_NAMESPACE_METADATA_HPP_

#include <map>

#include "utils.hpp"
#include <boost/bind.hpp>

#include "clustering/administration/datacenter_metadata.hpp"
#include "clustering/administration/http/json_adapters.hpp"
#include "clustering/administration/persistable_blueprint.hpp"
#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "clustering/immediate_consistency/query/metadata.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "clustering/reactor/directory_echo.hpp"
#include "clustering/reactor/json_adapters.hpp"
#include "clustering/reactor/metadata.hpp"
#include "containers/uuid.hpp"
#include "http/json/json_adapter.hpp"
#include "rpc/semilattice/joins/deletable.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/semilattice/joins/map.hpp"
#include "rpc/semilattice/joins/vclock.hpp"
#include "rpc/serialize_macros.hpp"

typedef uuid_t namespace_id_t;

/* This is the metadata for a single namespace of a specific protocol. */

/* If you change this data structure, you must also update
`clustering/administration/issues/vector_clock_conflict.hpp`. */

template<class protocol_t>
class namespace_semilattice_metadata_t {
public:
    vclock_t<persistable_blueprint_t<protocol_t> > blueprint;
    vclock_t<datacenter_id_t> primary_datacenter;
    vclock_t<std::map<datacenter_id_t, int> > replica_affinities;
    vclock_t<std::map<datacenter_id_t, int> > ack_expectations;
    vclock_t<std::set<typename protocol_t::region_t> > shards;
    vclock_t<std::string> name;
    vclock_t<int> port;
    vclock_t<region_map_t<protocol_t, machine_id_t> > primary_pinnings;
    vclock_t<region_map_t<protocol_t, std::set<machine_id_t> > > secondary_pinnings;
    vclock_t<std::string> primary_key; //TODO this should actually never be changed...

    RDB_MAKE_ME_SERIALIZABLE_10(blueprint, primary_datacenter, replica_affinities, ack_expectations, shards, name, port, primary_pinnings, secondary_pinnings, primary_key);
};

template<class protocol_t>
RDB_MAKE_SEMILATTICE_JOINABLE_10(namespace_semilattice_metadata_t<protocol_t>, blueprint, primary_datacenter, replica_affinities, ack_expectations, shards, name, port, primary_pinnings, secondary_pinnings, primary_key);

template<class protocol_t>
RDB_MAKE_EQUALITY_COMPARABLE_10(namespace_semilattice_metadata_t<protocol_t>, blueprint, primary_datacenter, replica_affinities, ack_expectations, shards, name, port, primary_pinnings, secondary_pinnings, primary_key);

//json adapter concept for namespace_semilattice_metadata_t
template <class ctx_t, class protocol_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(namespace_semilattice_metadata_t<protocol_t> *target, const ctx_t &) {
    typename json_adapter_if_t<ctx_t>::json_adapter_map_t res;
    res["blueprint"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_read_only_adapter_t<vclock_t<persistable_blueprint_t<protocol_t> >, ctx_t>(&target->blueprint));
    res["primary_uuid"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_vclock_adapter_t<datacenter_id_t, ctx_t>(&target->primary_datacenter));
    res["replica_affinities"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_vclock_adapter_t<std::map<datacenter_id_t, int>, ctx_t>(&target->replica_affinities));
    res["ack_expectations"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_vclock_adapter_t<std::map<datacenter_id_t, int>, ctx_t>(&target->ack_expectations));
    res["name"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_vclock_adapter_t<std::string, ctx_t>(&target->name));
    res["shards"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_vclock_adapter_t<std::set<typename protocol_t::region_t>, ctx_t>(&target->shards));
    res["port"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_vclock_adapter_t<int, ctx_t>(&target->port));
    res["primary_pinnings"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_vclock_adapter_t<region_map_t<protocol_t, machine_id_t>, ctx_t>(&target->primary_pinnings));
    res["secondary_pinnings"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_vclock_adapter_t<region_map_t<protocol_t, std::set<machine_id_t> >, ctx_t>(&target->secondary_pinnings));
    res["primary_key"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_read_only_adapter_t<vclock_t<std::string>, ctx_t>(&target->primary_key));
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

/* This is the metadata for all of the namespaces of a specific protocol. */
template <class protocol_t>
class namespaces_semilattice_metadata_t {
public:
    typedef std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > > namespace_map_t;
    namespace_map_t namespaces;

    /* If a name uniquely identifies a namespace then return it, otherwise
     * return nothing. */
    boost::optional<std::pair<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > > > get_namespace_by_name(std::string name) {
        boost::optional<std::pair<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > > >  res;
        for (typename namespace_map_t::iterator it  = namespaces.begin();
                                                it != namespaces.end();
                                                ++it) {
            if (it->second.is_deleted() || it->second.get().name.in_conflict()) {
                return boost::optional<std::pair<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > > > ();
            }

            if (it->second.get().name.get() == name) {
                if (!res) {
                    *res = *it;
                } else {
                    return boost::optional<std::pair<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > > >();
                }
            }
        }
        return res;
    }

    branch_history_t<protocol_t> branch_history;

    RDB_MAKE_ME_SERIALIZABLE_2(namespaces, branch_history);
};

template<class protocol_t>
RDB_MAKE_SEMILATTICE_JOINABLE_2(namespaces_semilattice_metadata_t<protocol_t>, namespaces, branch_history);

template<class protocol_t>
RDB_MAKE_EQUALITY_COMPARABLE_2(namespaces_semilattice_metadata_t<protocol_t>, namespaces, branch_history);

// json adapter concept for namespaces_semilattice_metadata_t

template <class ctx_t, class protocol_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(namespaces_semilattice_metadata_t<protocol_t> *target, const ctx_t &ctx) {
    namespace_semilattice_metadata_t<protocol_t> default_namespace;
    std::set<typename protocol_t::region_t> default_shards;
    default_shards.insert(protocol_t::region_t::universe());
    default_namespace.shards = default_namespace.shards.make_new_version(default_shards, ctx.us);

    deletable_t<namespace_semilattice_metadata_t<protocol_t> > default_ns_in_deletable(default_namespace);
    return json_adapter_with_inserter_t<typename namespaces_semilattice_metadata_t<protocol_t>::namespace_map_t, ctx_t>(&target->namespaces, boost::bind(&generate_uuid), default_ns_in_deletable).get_subfields(ctx);
}

template <class ctx_t, class protocol_t>
cJSON *render_as_json(namespaces_semilattice_metadata_t<protocol_t> *target, const ctx_t &ctx) {
    return render_as_json(&target->namespaces, ctx);
}

template <class ctx_t, class protocol_t>
void apply_json_to(cJSON *change, namespaces_semilattice_metadata_t<protocol_t> *target, const ctx_t &ctx) {
    apply_as_directory(change, &target->namespaces, ctx);
}

template <class ctx_t, class protocol_t>
void on_subfield_change(namespaces_semilattice_metadata_t<protocol_t> *target, const ctx_t &ctx) {
    on_subfield_change(&target->namespaces, ctx);
}

template <class protocol_t>
class namespaces_directory_metadata_t {
public:
    typedef std::map<namespace_id_t, directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > reactor_bcards_map_t;
    typedef std::map<namespace_id_t, std::map<master_id_t, master_business_card_t<protocol_t> > > master_maps_map_t;

    reactor_bcards_map_t reactor_bcards;
    master_maps_map_t master_maps;

    RDB_MAKE_ME_SERIALIZABLE_2(reactor_bcards, master_maps);
};

struct namespace_metadata_ctx_t {
    uuid_t us;
    explicit namespace_metadata_ctx_t(uuid_t _us)
        : us(_us)
    { }
};

// json adapter concept for namespaces_directory_metadata_t

template <class ctx_t, class protocol_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(namespaces_directory_metadata_t<protocol_t> *target, UNUSED const ctx_t &ctx) {
    typename json_adapter_if_t<ctx_t>::json_adapter_map_t res;
    res["reactor_bcards"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_read_only_adapter_t<std::map<namespace_id_t, directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > >, ctx_t>(&target->reactor_bcards));
    return res;
}

template <class ctx_t, class protocol_t>
cJSON *render_as_json(namespaces_directory_metadata_t<protocol_t> *target, const ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

template <class ctx_t, class protocol_t>
void apply_json_to(cJSON *change, namespaces_directory_metadata_t<protocol_t> *target, const ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

template <class ctx_t, class protocol_t>
void on_subfield_change(UNUSED namespaces_directory_metadata_t<protocol_t> *target, UNUSED const ctx_t &ctx) { }

#endif /* CLUSTERING_ADMINISTRATION_NAMESPACE_METADATA_HPP_ */
