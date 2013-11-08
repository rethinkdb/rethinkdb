// Copyright 2010-2013 RethinkDB, all rights reserved.
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
#include "clustering/generic/nonoverlapping_regions.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "clustering/reactor/directory_echo.hpp"
#include "clustering/reactor/reactor_json_adapters.hpp"
#include "clustering/reactor/metadata.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/cow_ptr_type.hpp"
#include "containers/cow_ptr.hpp"
#include "containers/name_string.hpp"
#include "containers/uuid.hpp"
#include "http/json/json_adapter.hpp"
#include "rpc/semilattice/joins/deletable.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/semilattice/joins/map.hpp"
#include "rpc/semilattice/joins/vclock.hpp"
#include "rpc/serialize_macros.hpp"


/* This is the metadata for a single namespace of a specific protocol. */

/* If you change this data structure, you must also update
`clustering/administration/issues/vector_clock_conflict.hpp`. */

class ack_expectation_t {
public:
    ack_expectation_t() : expectation_(0), hard_durability_(true) { }

    explicit ack_expectation_t(uint32_t expectation, bool hard_durability) :
        expectation_(expectation),
        hard_durability_(hard_durability) { }

    uint32_t expectation() const { return expectation_; }
    bool is_hardly_durable() const { return hard_durability_; }

    RDB_DECLARE_ME_SERIALIZABLE;

    bool operator==(ack_expectation_t other) const;

private:
    friend json_adapter_if_t::json_adapter_map_t get_json_subfields(ack_expectation_t *target);

    uint32_t expectation_;
    bool hard_durability_;
};

void debug_print(printf_buffer_t *buf, const ack_expectation_t &x);

template<class protocol_t>
class namespace_semilattice_metadata_t {
public:
    namespace_semilattice_metadata_t() : cache_size(GIGABYTE) { }

    vclock_t<persistable_blueprint_t<protocol_t> > blueprint;
    vclock_t<datacenter_id_t> primary_datacenter;
    vclock_t<std::map<datacenter_id_t, int32_t> > replica_affinities;
    vclock_t<std::map<datacenter_id_t, ack_expectation_t> > ack_expectations;
    vclock_t<nonoverlapping_regions_t<protocol_t> > shards;
    vclock_t<name_string_t> name;
    vclock_t<int> port;
    vclock_t<region_map_t<protocol_t, machine_id_t> > primary_pinnings;
    vclock_t<region_map_t<protocol_t, std::set<machine_id_t> > > secondary_pinnings;
    vclock_t<std::string> primary_key; //TODO this should actually never be changed...
    vclock_t<database_id_t> database;
    vclock_t<int64_t> cache_size;

    RDB_MAKE_ME_SERIALIZABLE_12(blueprint, primary_datacenter, replica_affinities, ack_expectations, shards, name, port, primary_pinnings, secondary_pinnings, primary_key, database, cache_size);
};

template <class protocol_t>
void debug_print(printf_buffer_t *buf, const namespace_semilattice_metadata_t<protocol_t> &m) {
    buf->appendf("ns_sl_metadata{blueprint=");
    debug_print(buf, m.blueprint);
    buf->appendf(", primary_datacenter=");
    debug_print(buf, m.primary_datacenter);
    buf->appendf(", replica_affinities=");
    debug_print(buf, m.replica_affinities);
    buf->appendf(", ack_expectations=");
    debug_print(buf, m.ack_expectations);
    buf->appendf(", shards=");
    debug_print(buf, m.shards);
    buf->appendf(", name=");
    debug_print(buf, m.name);
    buf->appendf(", port=");
    debug_print(buf, m.port);
    buf->appendf(", primary_pinnings=");
    debug_print(buf, m.primary_pinnings);
    buf->appendf(", secondary_pinnings=");
    debug_print(buf, m.secondary_pinnings);
    buf->appendf(", primary_key=");
    debug_print(buf, m.primary_key);
    buf->appendf(", database=");
    debug_print(buf, m.database);
    buf->appendf("}");
}

template<class protocol_t>
namespace_semilattice_metadata_t<protocol_t> new_namespace(
    uuid_u machine, uuid_u database, uuid_u datacenter,
    const name_string_t &name, const std::string &key, int port,
    int64_t cache_size) {

    namespace_semilattice_metadata_t<protocol_t> ns;
    ns.database           = make_vclock(database, machine);
    ns.primary_datacenter = make_vclock(datacenter, machine);
    ns.name               = make_vclock(name, machine);
    ns.primary_key        = make_vclock(key, machine);
    ns.port               = make_vclock(port, machine);

    std::map<uuid_u, ack_expectation_t> ack_expectations;
    ack_expectations[datacenter] = ack_expectation_t(1, true);
    ns.ack_expectations = make_vclock(ack_expectations, machine);

    nonoverlapping_regions_t<protocol_t> shards;
    bool add_region_success = shards.add_region(protocol_t::region_t::universe());
    guarantee(add_region_success);
    ns.shards = make_vclock(shards, machine);

    region_map_t<protocol_t, uuid_u> primary_pinnings(
        protocol_t::region_t::universe(), nil_uuid());
    ns.primary_pinnings = make_vclock(primary_pinnings, machine);

    region_map_t<protocol_t, std::set<uuid_u> > secondary_pinnings(
        protocol_t::region_t::universe(), std::set<machine_id_t>());
    ns.secondary_pinnings = make_vclock(secondary_pinnings, machine);

    ns.cache_size = make_vclock(cache_size, machine);
    return ns;
}

template<class protocol_t>
RDB_MAKE_SEMILATTICE_JOINABLE_12(namespace_semilattice_metadata_t<protocol_t>, blueprint, primary_datacenter, replica_affinities, ack_expectations, shards, name, port, primary_pinnings, secondary_pinnings, primary_key, database, cache_size);

template<class protocol_t>
RDB_MAKE_EQUALITY_COMPARABLE_12(namespace_semilattice_metadata_t<protocol_t>, blueprint, primary_datacenter, replica_affinities, ack_expectations, shards, name, port, primary_pinnings, secondary_pinnings, primary_key, database, cache_size);

// ctx-less json adapter concept for ack_expectation_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(ack_expectation_t *target);
cJSON *render_as_json(ack_expectation_t *target);
void apply_json_to(cJSON *change, ack_expectation_t *target);

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
    /* This used to say `reactor_business_card_t<protocol_t>` instead of
    `cow_ptr_t<reactor_business_card_t<protocol_t> >`, but that
    was extremely slow because the size of the data structure grew linearly with
    the number of tables and so copying it became a major cost. Using a
    `boost::shared_ptr` instead makes it significantly faster. */
    typedef std::map<namespace_id_t, directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > > reactor_bcards_map_t;

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

#endif /* CLUSTERING_ADMINISTRATION_NAMESPACE_METADATA_HPP_ */
