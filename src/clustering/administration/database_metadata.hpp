#ifndef CLUSTERING_ADMINISTRATION_DATABASE_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_DATABASE_METADATA_HPP_

#include <map>
#include <string>

// TODO: Some of these includes could be moved to the .cc thanks to deinlining json adapters?
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

void debug_print(append_only_printf_buffer_t *buf, const database_semilattice_metadata_t &x);



json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(database_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
cJSON *with_ctx_render_as_json(database_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
void with_ctx_apply_json_to(cJSON *change, database_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
void with_ctx_on_subfield_change(database_semilattice_metadata_t *, const vclock_ctx_t &);

class databases_semilattice_metadata_t {
public:
    typedef std::map<database_id_t, deletable_t<database_semilattice_metadata_t> > database_map_t;
    database_map_t databases;

    RDB_MAKE_ME_SERIALIZABLE_1(databases);
};

RDB_MAKE_SEMILATTICE_JOINABLE_1(databases_semilattice_metadata_t, databases);
RDB_MAKE_EQUALITY_COMPARABLE_1(databases_semilattice_metadata_t, databases);

//json adapter concept for databases_semilattice_metadata_t
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(databases_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
cJSON *with_ctx_render_as_json(databases_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
void with_ctx_apply_json_to(cJSON *change, databases_semilattice_metadata_t *target, const vclock_ctx_t &ctx);
void with_ctx_on_subfield_change(databases_semilattice_metadata_t *target, const vclock_ctx_t &ctx);

#endif /* CLUSTERING_ADMINISTRATION_DATABASE_METADATA_HPP_ */

