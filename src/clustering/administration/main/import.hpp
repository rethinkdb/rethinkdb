#ifndef CLUSTERING_ADMINISTRATION_MAIN_IMPORT_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_IMPORT_HPP_

#include <set>
#include <string>

#include "errors.hpp"

class io_backender_t;
class peer_address_t;
class json_importer_t;


bool run_json_import(io_backender_t *backender, std::set<peer_address_t> peers, int client_port, std::string db_table, json_importer_t *importer);




#endif  // CLUSTERING_ADMINISTRATION_MAIN_IMPORT_HPP_
