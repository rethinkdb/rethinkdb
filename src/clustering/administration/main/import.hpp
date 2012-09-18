#ifndef CLUSTERING_ADMINISTRATION_MAIN_IMPORT_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_IMPORT_HPP_

#include <set>
#include <string>

#include "errors.hpp"
#include "extproc/spawner.hpp"

namespace boost {
template <class> class optional;
};

class peer_address_t;
class json_importer_t;

bool run_json_import(extproc::spawner_t::info_t *spawner_info, std::set<peer_address_t> peers, int ports_port, int ports_client_port, std::string db_name, std::string table_name, boost::optional<std::string> primary_key, json_importer_t *importer, signal_t *stop_cond);




#endif  // CLUSTERING_ADMINISTRATION_MAIN_IMPORT_HPP_
