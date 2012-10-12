#ifndef CLUSTERING_ADMINISTRATION_MAIN_IMPORT_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_IMPORT_HPP_

#include <set>
#include <string>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "containers/name_string.hpp"
#include "extproc/spawner.hpp"

class peer_address_set_t;
class json_importer_t;

struct json_import_target_t {
    name_string_t db_name;
    boost::optional<std::string> datacenter_name;
    name_string_t table_name;
    std::string primary_key;
};

bool run_json_import(extproc::spawner_t::info_t *spawner_info, peer_address_set_t peers, int ports_port, int ports_client_port, json_import_target_t import_args, json_importer_t *importer, signal_t *stop_cond);




#endif  // CLUSTERING_ADMINISTRATION_MAIN_IMPORT_HPP_
