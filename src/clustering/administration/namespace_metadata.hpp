#ifndef CLUSTERING_ADMINISTRATION_NAMESPACE_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_NAMESPACE_METADATA_HPP_

#include <map>
#include <set>
#include <string>
#include <utility>

#include "utils.hpp"
#include <boost/bind.hpp>

#include "clustering/administration/database_metadata.hpp"
#include "clustering/administration/datacenter_metadata.hpp"
#include "clustering/administration/http/json_adapters.hpp"
#include "clustering/administration/persistable_blueprint.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "clustering/reactor/directory_echo.hpp"
#include "clustering/reactor/reactor_json_adapters.hpp"
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
    vclock_t<database_id_t> database;

    RDB_MAKE_ME_SERIALIZABLE_11(blueprint, primary_datacenter, replica_affinities, ack_expectations, shards, name, port, primary_pinnings, secondary_pinnings, primary_key, database);
};


class vclock_builder_t {
public:
    vclock_builder_t(const machine_id_t &_machine) : machine(_machine) { }
    template<class T>
    vclock_t<T> build(const T &arg) { return vclock_t<T>(arg, machine); }
private:
    machine_id_t machine;
};

template<class protocol_t>
namespace_semilattice_metadata_t<protocol_t> new_namespace(
    uuid_t machine, uuid_t database, uuid_t datacenter,
    const std::string &name, const std::string &key, int port) {

    vclock_builder_t vc(machine);
    namespace_semilattice_metadata_t<protocol_t> ns;
    ns.database           = vc.build(database);
    ns.primary_datacenter = vc.build(datacenter);
    ns.name               = vc.build(name);
    ns.primary_key        = vc.build(key);
    ns.port               = vc.build(port);

    std::map<uuid_t, int> affinities;
    affinities.insert(std::make_pair(datacenter, 0));
    ns.replica_affinities = vc.build(affinities);

    std::map<uuid_t, int> ack_expectations;
    ack_expectations.insert(std::make_pair(datacenter, 1));
    ns.ack_expectations = vc.build(ack_expectations);

    std::set<typename protocol_t::region_t> shards;
    shards.insert(protocol_t::region_t::universe());
    ns.shards = vc.build(shards);

    region_map_t<protocol_t, uuid_t> primary_pinnings(
        protocol_t::region_t::universe(), nil_uuid());
    ns.primary_pinnings = vc.build(primary_pinnings);

    region_map_t<protocol_t, std::set<uuid_t> > secondary_pinnings(
        protocol_t::region_t::universe(), std::set<machine_id_t>());
    ns.secondary_pinnings = vc.build(secondary_pinnings);

    return ns;
}

template<class protocol_t>
RDB_MAKE_SEMILATTICE_JOINABLE_11(namespace_semilattice_metadata_t<protocol_t>, blueprint, primary_datacenter, replica_affinities, ack_expectations, shards, name, port, primary_pinnings, secondary_pinnings, primary_key, database);

template<class protocol_t>
RDB_MAKE_EQUALITY_COMPARABLE_11(namespace_semilattice_metadata_t<protocol_t>, blueprint, primary_datacenter, replica_affinities, ack_expectations, shards, name, port, primary_pinnings, secondary_pinnings, primary_key, database);

//json adapter concept for namespace_semilattice_metadata_t
template <class protocol_t>
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(namespace_semilattice_metadata_t<protocol_t> *target, const vclock_ctx_t &ctx);

template <class protocol_t>
cJSON *with_ctx_render_as_json(namespace_semilattice_metadata_t<protocol_t> *target, const vclock_ctx_t &ctx);

template <class protocol_t>
void with_ctx_apply_json_to(cJSON *change, namespace_semilattice_metadata_t<protocol_t> *target, const vclock_ctx_t &ctx);

template <class protocol_t>
void with_ctx_on_subfield_change(namespace_semilattice_metadata_t<protocol_t> *, const vclock_ctx_t &);

/* This is the metadata for all of the namespaces of a specific protocol. */
template <class protocol_t>
class namespaces_semilattice_metadata_t {
public:
    typedef std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > > namespace_map_t;
    namespace_map_t namespaces;

    RDB_MAKE_ME_SERIALIZABLE_1(namespaces);
};

template<class protocol_t>
RDB_MAKE_SEMILATTICE_JOINABLE_1(namespaces_semilattice_metadata_t<protocol_t>, namespaces);

template<class protocol_t>
RDB_MAKE_EQUALITY_COMPARABLE_1(namespaces_semilattice_metadata_t<protocol_t>, namespaces);

// json adapter concept for namespaces_semilattice_metadata_t
template <class protocol_t>
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(namespaces_semilattice_metadata_t<protocol_t> *target, const vclock_ctx_t &ctx);

template <class protocol_t>
cJSON *with_ctx_render_as_json(namespaces_semilattice_metadata_t<protocol_t> *target, const vclock_ctx_t &ctx);

template <class protocol_t>
void with_ctx_apply_json_to(cJSON *change, namespaces_semilattice_metadata_t<protocol_t> *target, const vclock_ctx_t &ctx);

template <class protocol_t>
void with_ctx_on_subfield_change(namespaces_semilattice_metadata_t<protocol_t> *target, const vclock_ctx_t &ctx);

template <class protocol_t>
class namespaces_directory_metadata_t {
public:
    typedef std::map<namespace_id_t, directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > reactor_bcards_map_t;

    reactor_bcards_map_t reactor_bcards;

    RDB_MAKE_ME_SERIALIZABLE_1(reactor_bcards);
};

// ctx-less json adapter concept for namespaces_directory_metadata_t
template <class protocol_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(namespaces_directory_metadata_t<protocol_t> *target);

template <class protocol_t>
cJSON *render_as_json(namespaces_directory_metadata_t<protocol_t> *target);

template <class protocol_t>
void apply_json_to(cJSON *change, namespaces_directory_metadata_t<protocol_t> *target);

template <class protocol_t>
void on_subfield_change(UNUSED namespaces_directory_metadata_t<protocol_t> *target);

#endif /* CLUSTERING_ADMINISTRATION_NAMESPACE_METADATA_HPP_ */
