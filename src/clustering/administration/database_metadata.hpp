#ifndef CLUSTERING_ADMINISTRATION_DATABASE_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_DATABASE_METADATA_HPP_

#include <map>
#include <string>

#include "clustering/administration/http/json_adapters.hpp"
#include "containers/uuid.hpp"
#include "http/json.hpp"
#include "http/json/json_adapter.hpp"
#include "rpc/semilattice/joins/deletable.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/semilattice/joins/map.hpp"
#include "rpc/semilattice/joins/vclock.hpp"
#include "rpc/serialize_macros.hpp"


typedef uuid_t database_id_t;

class database_semilattice_metadata_t {
public:
    vclock_t<std::string> name;
    RDB_MAKE_ME_SERIALIZABLE_1(name);
};

RDB_MAKE_SEMILATTICE_JOINABLE_1(database_semilattice_metadata_t, name);
RDB_MAKE_EQUALITY_COMPARABLE_1(database_semilattice_metadata_t, name);

template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(database_semilattice_metadata_t *target, const ctx_t &ctx) {
    typename json_adapter_if_t<ctx_t>::json_adapter_map_t res;
    res["name"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_vclock_adapter_t<std::string, ctx_t>(&target->name, ctx));
    return res;
}

template <class ctx_t>
cJSON *render_as_json(database_semilattice_metadata_t *target, const ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

template <class ctx_t>
void apply_json_to(cJSON *change, database_semilattice_metadata_t *target, const ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

template <class ctx_t>
void on_subfield_change(database_semilattice_metadata_t *, const ctx_t &) { }

class databases_semilattice_metadata_t {
public:
    typedef std::map<database_id_t, deletable_t<database_semilattice_metadata_t> > database_map_t;
    database_map_t databases;

    RDB_MAKE_ME_SERIALIZABLE_1(databases);
};

RDB_MAKE_SEMILATTICE_JOINABLE_1(databases_semilattice_metadata_t, databases);
RDB_MAKE_EQUALITY_COMPARABLE_1(databases_semilattice_metadata_t, databases);

//json adapter concept for databases_semilattice_metadata_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(databases_semilattice_metadata_t *target, const ctx_t &ctx) {
    return json_adapter_with_inserter_t<databases_semilattice_metadata_t::database_map_t, ctx_t>(&target->databases, generate_uuid, ctx).get_subfields(ctx);
}

template <class ctx_t>
cJSON *render_as_json(databases_semilattice_metadata_t *target, const ctx_t &ctx) {
    return render_as_json(&target->databases, ctx);
}

template <class ctx_t>
void apply_json_to(cJSON *change, databases_semilattice_metadata_t *target, const ctx_t &ctx) {
    apply_as_directory(change, &target->databases, ctx);
}

template <class ctx_t>
void on_subfield_change(databases_semilattice_metadata_t *target, const ctx_t &ctx) {
    on_subfield_change(&target->databases, ctx);
}

#endif /* CLUSTERING_ADMINISTRATION_DATABASE_METADATA_HPP_ */

